///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <plugins/grid/Grid.h>
#include <plugins/grid/objects/VoxelGrid.h>
#include <core/dataset/DataSet.h>
#include <plugins/stdobj/util/InputHelper.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "CreateIsosurfaceModifier.h"
#include "MarchingCubes.h"

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(CreateIsosurfaceModifier);
DEFINE_PROPERTY_FIELD(CreateIsosurfaceModifier, sourceProperty);
DEFINE_REFERENCE_FIELD(CreateIsosurfaceModifier, isolevelController);
DEFINE_REFERENCE_FIELD(CreateIsosurfaceModifier, surfaceMeshVis);
SET_PROPERTY_FIELD_LABEL(CreateIsosurfaceModifier, sourceProperty, "Source property");
SET_PROPERTY_FIELD_LABEL(CreateIsosurfaceModifier, isolevelController, "Isolevel");
SET_PROPERTY_FIELD_LABEL(CreateIsosurfaceModifier, surfaceMeshVis, "Surface mesh vis");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CreateIsosurfaceModifier::CreateIsosurfaceModifier(DataSet* dataset) : AsynchronousModifier(dataset)
{
	setIsolevelController(ControllerManager::createFloatController(dataset));

	// Create the vis element.
	setSurfaceMeshVis(new SurfaceMeshVis(dataset));
	surfaceMeshVis()->setShowCap(false);
	surfaceMeshVis()->setSmoothShading(true);
	surfaceMeshVis()->setObjectTitle(tr("Isosurface"));
}

/******************************************************************************
* Asks the modifier for its validity interval at the given time.
******************************************************************************/
TimeInterval CreateIsosurfaceModifier::modifierValidity(TimePoint time)
{
	TimeInterval interval = AsynchronousModifier::modifierValidity(time);
	if(isolevelController()) interval.intersect(isolevelController()->validityInterval(time));
	return interval;
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool CreateIsosurfaceModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return (input.findObject<VoxelProperty>() != nullptr) && (input.findObject<VoxelGrid>() != nullptr);
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void CreateIsosurfaceModifier::initializeModifier(ModifierApplication* modApp)
{
	AsynchronousModifier::initializeModifier(modApp);

	// Use the first available property from the input state as data source when the modifier is newly created.
	if(sourceProperty().isNull()) {
		PipelineFlowState input = modApp->evaluateInputPreliminary();
		for(DataObject* o : input.objects()) {
			VoxelProperty* property = dynamic_object_cast<VoxelProperty>(o);
			if(property && property->componentCount() <= 1) {
				setSourceProperty(VoxelPropertyReference(property, (property->componentCount() > 1) ? 0 : -1));
			}
		}
	}
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the 
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> CreateIsosurfaceModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	InputHelper ih(dataset(), input);

	// Get modifier inputs.
	VoxelGrid* voxelGrid = input.findObject<VoxelGrid>();
	if(!voxelGrid)
		throwException(tr("Modifier input contains no voxel data grid."));
	OVITO_ASSERT(voxelGrid->domain());
	if(sourceProperty().isNull())
		throwException(tr("Select a field quantity first."));
	VoxelProperty* property = sourceProperty().findInState(input);
	if(!property)
		throwException(tr("The selected voxel property with the name '%1' does not exist.").arg(sourceProperty().name()));
	if(sourceProperty().vectorComponent() >= (int)property->componentCount())
		throwException(tr("The selected vector component is out of range. The property '%1' contains only %2 values per voxel.").arg(sourceProperty().name()).arg(property->componentCount()));

	TimeInterval validityInterval = input.stateValidity();
	FloatType isolevel = isolevelController() ? isolevelController()->getFloatValue(time, validityInterval) : 0;

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ComputeIsosurfaceEngine>(validityInterval, voxelGrid->shape(), property->storage(), 
			sourceProperty().vectorComponent(), voxelGrid->domain()->data(), isolevel);
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CreateIsosurfaceModifier::ComputeIsosurfaceEngine::perform()
{
	setProgressText(tr("Constructing isosurface"));

	if(_gridShape.size() != 3)
		throw Exception(tr("Can construct isosurface only for three-dimensional voxel grids"));
	if(property()->dataType() != PropertyStorage::Float)
		throw Exception(tr("Can construct isosurface only for floating-point data"));
	if(property()->size() != _gridShape[0]*_gridShape[1]*_gridShape[2])
		throw Exception(tr("Input voxel property has wrong dimensions."));

	const FloatType* fieldData = property()->constDataFloat() + std::max(_vectorComponent, 0);

	MarchingCubes mc(_gridShape[0], _gridShape[1], _gridShape[2], _simCell.pbcFlags(), fieldData, property()->componentCount(), *_results->mesh(), false);
	if(!mc.generateIsosurface(_isolevel, *this))
		return;
	_results->setIsCompletelySolid(mc.isCompletelySolid());

	// Determin min/max field values.
	const FloatType* fieldDataEnd = fieldData + _gridShape[0]*_gridShape[1]*_gridShape[2]*property()->componentCount();
	for(; fieldData != fieldDataEnd; fieldData += property()->componentCount()) {
		_results->updateMinMax(*fieldData);
	}

	// Transform mesh vertices from orthogonal grid space to world space.
	const AffineTransformation tm = _simCell.matrix() * Matrix3(
		FloatType(1) / (_gridShape[0] - (_simCell.pbcFlags()[0]?0:1)), 0, 0, 
		0, FloatType(1) / (_gridShape[1] - (_simCell.pbcFlags()[1]?0:1)), 0, 
		0, 0, FloatType(1) / (_gridShape[2] - (_simCell.pbcFlags()[2]?0:1)));
	for(HalfEdgeMesh<>::Vertex* vertex : _results->mesh()->vertices())
		vertex->setPos(tm * vertex->pos());

	// Flip surface orientation if cell matrix is a mirror transformation.
	if(tm.determinant() < 0) {
		_results->mesh()->flipFaces();
	}

	if(isCanceled())
		return;

	if(!_results->mesh()->connectOppositeHalfedges())
		throw Exception(tr("Isosurface mesh is not closed."));

	setResult(std::move(_results));
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState CreateIsosurfaceModifier::ComputeIsosurfaceResults::apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	CreateIsosurfaceModifier* modifier = static_object_cast<CreateIsosurfaceModifier>(modApp->modifier());
	OVITO_ASSERT(modifier);

	// Find the input voxel grid.
	VoxelGrid* voxelGrid = input.findObject<VoxelGrid>();
	if(!voxelGrid) return input;
		
	// Create the output data object.
	OORef<SurfaceMesh> meshObj(new SurfaceMesh(modApp->dataset()));
	meshObj->setStorage(mesh());
	meshObj->setIsCompletelySolid(isCompletelySolid());
	meshObj->setDomain(voxelGrid->domain());
	meshObj->setVisElement(modifier->surfaceMeshVis());

	// Insert data object into the output data collection.
	PipelineFlowState output = input;
	output.addObject(meshObj);
	output.setStatus(PipelineStatus(PipelineStatus::Success, tr("Minimum value: %1\nMaximum value: %2").arg(minValue()).arg(maxValue())));

	return output;
}

}	// End of namespace
}	// End of namespace
