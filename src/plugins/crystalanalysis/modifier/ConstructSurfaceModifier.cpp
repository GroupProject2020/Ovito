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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <plugins/crystalanalysis/util/DelaunayTessellation.h>
#include <plugins/crystalanalysis/util/ManifoldConstructionHelper.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "ConstructSurfaceModifier.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(ConstructSurfaceModifier);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, smoothingLevel);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, probeSphereRadius);
DEFINE_REFERENCE_FIELD(ConstructSurfaceModifier, surfaceMeshDisplay);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, onlySelectedParticles);
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, smoothingLevel, "Smoothing level");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, probeSphereRadius, "Probe sphere radius");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, surfaceMeshDisplay, "Surface mesh display");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, onlySelectedParticles, "Use only selected particles");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ConstructSurfaceModifier, probeSphereRadius, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ConstructSurfaceModifier, smoothingLevel, IntegerParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ConstructSurfaceModifier::ConstructSurfaceModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_smoothingLevel(8), 
	_probeSphereRadius(4), 
	_onlySelectedParticles(false)
{
	// Create the display object.
	setSurfaceMeshDisplay(new SurfaceMeshDisplay(dataset));
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool ConstructSurfaceModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool ConstructSurfaceModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// Do not propagate messages from the attached display object.
	if(source == surfaceMeshDisplay())
		return false;

	return AsynchronousModifier::referenceEvent(source, event);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the 
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> ConstructSurfaceModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get modifier inputs.
	ParticleInputHelper ph(dataset(), input);
	ParticleProperty* posProperty = ph.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);
	ParticleProperty* selProperty = nullptr;
	if(onlySelectedParticles())
		selProperty = ph.expectStandardProperty<ParticleProperty>(ParticleProperty::SelectionProperty);
	SimulationCellObject* simCell = ph.expectSimulationCell();
	if(simCell->is2D())
		throwException(tr("The construct surface mesh modifier does not support 2d simulation cells."));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ConstructSurfaceEngine>(posProperty->storage(),
			selProperty ? selProperty->storage() : nullptr,
			simCell->data(), probeSphereRadius(), smoothingLevel());
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void ConstructSurfaceModifier::ConstructSurfaceEngine::perform()
{
	setProgressText(tr("Constructing surface mesh"));

	if(_radius <= 0)
		throw Exception(tr("Radius parameter must be positive."));

	if(_simCell.volume3D() <= FLOATTYPE_EPSILON*FLOATTYPE_EPSILON*FLOATTYPE_EPSILON)
		throw Exception(tr("Simulation cell is degenerate."));
		
	double alpha = _radius * _radius;
	FloatType ghostLayerSize = _radius * FloatType(3);

	// Check if combination of radius parameter and simulation cell size is valid.
	for(size_t dim = 0; dim < 3; dim++) {
		if(_simCell.pbcFlags()[dim]) {
			int stencilCount = (int)ceil(ghostLayerSize / _simCell.matrix().column(dim).dot(_simCell.cellNormalVector(dim)));
			if(stencilCount > 1)
				throw Exception(tr("Cannot generate Delaunay tessellation. Simulation cell is too small, or radius parameter is too large."));
		}
	}

	// If there are too few particles, don't build Delaunay tessellation.
	// It is going to be invalid anyway.
	size_t numInputParticles = positions()->size();
	if(selection())
		numInputParticles = positions()->size() - std::count(selection()->constDataInt(), selection()->constDataInt() + selection()->size(), 0);
	if(numInputParticles <= 3)
		return;

	// Algorithm is divided into several sub-steps.
	// Assign weights to sub-steps according to estimated runtime.
	beginProgressSubStepsWithWeights({ 20, 1, 6, 1 });

	// Generate Delaunay tessellation.
	DelaunayTessellation tessellation;
	if(!tessellation.generateTessellation(_simCell, positions()->constDataPoint3(), positions()->size(), ghostLayerSize,
			selection() ? selection()->constDataInt() : nullptr, *this))
		return;

	nextProgressSubStep();

	// Determines the region a solid Delaunay cell belongs to.
	// We use this callback function to compute the total volume of the solid region.
	auto tetrahedronRegion = [this, &tessellation](DelaunayTessellation::CellHandle cell) {
		if(tessellation.isGhostCell(cell) == false) {
			Point3 p0 = tessellation.vertexPosition(tessellation.cellVertex(cell, 0));
			Vector3 ad = tessellation.vertexPosition(tessellation.cellVertex(cell, 1)) - p0;
			Vector3 bd = tessellation.vertexPosition(tessellation.cellVertex(cell, 2)) - p0;
			Vector3 cd = tessellation.vertexPosition(tessellation.cellVertex(cell, 3)) - p0;
			_results->addSolidVolume(std::abs(ad.dot(cd.cross(bd))) / FloatType(6));
		}
		return 1;
	};

	ManifoldConstructionHelper<HalfEdgeMesh<>, true> manifoldConstructor(tessellation, *_results->mesh(), alpha, *positions());
	if(!manifoldConstructor.construct(tetrahedronRegion, *this))
		return;
	_results->setIsCompletelySolid(manifoldConstructor.spaceFillingRegion() == 1);

	nextProgressSubStep();

	// Make sure every mesh vertex is only part of one surface manifold.
	_results->mesh()->duplicateSharedVertices();

	nextProgressSubStep();
	if(!SurfaceMesh::smoothMesh(*_results->mesh(), _simCell, _smoothingLevel, *this))
		return;

	// Compute surface area.
	for(const HalfEdgeMesh<>::Face* facet : _results->mesh()->faces()) {
		if(isCanceled()) return;
		Vector3 e1 = _simCell.wrapVector(facet->edges()->vertex1()->pos() - facet->edges()->vertex2()->pos());
		Vector3 e2 = _simCell.wrapVector(facet->edges()->prevFaceEdge()->vertex1()->pos() - facet->edges()->vertex2()->pos());
		_results->addSurfaceArea(e1.cross(e2).length() / 2);
	}

	endProgressSubSteps();

	setResult(std::move(_results));
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState ConstructSurfaceModifier::ConstructSurfaceResults::apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	ConstructSurfaceModifier* modifier = static_object_cast<ConstructSurfaceModifier>(modApp->modifier());
	OVITO_ASSERT(modifier);

	// Create the output data object.
	OORef<SurfaceMesh> meshObj(new SurfaceMesh(modApp->dataset()));
	meshObj->setStorage(mesh());
	meshObj->setIsCompletelySolid(isCompletelySolid());
	meshObj->setDomain(input.findObject<SimulationCellObject>());
	meshObj->addDisplayObject(modifier->surfaceMeshDisplay());

	// Insert output object into the pipeline.
	PipelineFlowState output = input;
	output.addObject(meshObj);
	
	output.attributes().insert(QStringLiteral("ConstructSurfaceMesh.surface_area"), QVariant::fromValue(surfaceArea()));
	output.attributes().insert(QStringLiteral("ConstructSurfaceMesh.solid_volume"), QVariant::fromValue(solidVolume()));

	output.setStatus(PipelineStatus(PipelineStatus::Success, tr("Surface area: %1\nSolid volume: %2\nTotal cell volume: %3\nSolid volume fraction: %4\nSurface area per solid volume: %5\nSurface area per total volume: %6")
			.arg(surfaceArea()).arg(solidVolume()).arg(totalVolume())
			.arg(solidVolume() / totalVolume()).arg(surfaceArea() / solidVolume()).arg(surfaceArea() / totalVolume())));

	return output;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

