///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <plugins/stdobj/properties/PropertyContainer.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include <plugins/stdobj/series/DataSeriesObject.h>
#include <core/app/Application.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/utilities/units/UnitsManager.h>
#include "SpatialBinningModifier.h"

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(SpatialBinningModifierDelegate);
DEFINE_PROPERTY_FIELD(SpatialBinningModifierDelegate, containerPath);

IMPLEMENT_OVITO_CLASS(SpatialBinningModifier);
DEFINE_PROPERTY_FIELD(SpatialBinningModifier, reductionOperation);
DEFINE_PROPERTY_FIELD(SpatialBinningModifier, firstDerivative);
DEFINE_PROPERTY_FIELD(SpatialBinningModifier, binDirection);
DEFINE_PROPERTY_FIELD(SpatialBinningModifier, numberOfBinsX);
DEFINE_PROPERTY_FIELD(SpatialBinningModifier, numberOfBinsY);
DEFINE_PROPERTY_FIELD(SpatialBinningModifier, numberOfBinsZ);
DEFINE_PROPERTY_FIELD(SpatialBinningModifier, fixPropertyAxisRange);
DEFINE_PROPERTY_FIELD(SpatialBinningModifier, propertyAxisRangeStart);
DEFINE_PROPERTY_FIELD(SpatialBinningModifier, propertyAxisRangeEnd);
DEFINE_PROPERTY_FIELD(SpatialBinningModifier, sourceProperty);
DEFINE_PROPERTY_FIELD(SpatialBinningModifier, onlySelectedElements);
SET_PROPERTY_FIELD_LABEL(SpatialBinningModifier, reductionOperation, "Reduction operation");
SET_PROPERTY_FIELD_LABEL(SpatialBinningModifier, firstDerivative, "Compute first derivative");
SET_PROPERTY_FIELD_LABEL(SpatialBinningModifier, binDirection, "Bin direction");
SET_PROPERTY_FIELD_LABEL(SpatialBinningModifier, numberOfBinsX, "Number of bins");
SET_PROPERTY_FIELD_LABEL(SpatialBinningModifier, numberOfBinsY, "Number of bins");
SET_PROPERTY_FIELD_LABEL(SpatialBinningModifier, numberOfBinsZ, "Number of bins");
SET_PROPERTY_FIELD_LABEL(SpatialBinningModifier, fixPropertyAxisRange, "Fix property axis range");
SET_PROPERTY_FIELD_LABEL(SpatialBinningModifier, propertyAxisRangeStart, "Property axis range start");
SET_PROPERTY_FIELD_LABEL(SpatialBinningModifier, propertyAxisRangeEnd, "Property axis range end");
SET_PROPERTY_FIELD_LABEL(SpatialBinningModifier, sourceProperty, "Source property");
SET_PROPERTY_FIELD_LABEL(SpatialBinningModifier, onlySelectedElements, "Use only selected elements");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(SpatialBinningModifier, numberOfBinsX, IntegerParameterUnit, 1, 100000);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(SpatialBinningModifier, numberOfBinsY, IntegerParameterUnit, 1, 100000);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(SpatialBinningModifier, numberOfBinsZ, IntegerParameterUnit, 1, 100000);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
SpatialBinningModifier::SpatialBinningModifier(DataSet* dataset) : AsynchronousDelegatingModifier(dataset), 
    _reductionOperation(RED_MEAN), 
    _firstDerivative(false),
    _binDirection(CELL_VECTOR_3), 
    _numberOfBinsX(200), 
    _numberOfBinsY(200),
    _numberOfBinsZ(200),
    _fixPropertyAxisRange(false), 
    _propertyAxisRangeStart(0), 
    _propertyAxisRangeEnd(0),
	_onlySelectedElements(false)
{
	// Let this modifier act on particles by default.
	createDefaultModifierDelegate(SpatialBinningModifierDelegate::OOClass(), QStringLiteral("ParticlesSpatialBinningModifierDelegate"));
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void SpatialBinningModifier::initializeModifier(ModifierApplication* modApp)
{
	AsynchronousDelegatingModifier::initializeModifier(modApp);

	// Use the first available property from the input state as data source when the modifier is newly created.
	if(sourceProperty().isNull() && delegate() && !Application::instance()->scriptMode()) {
		const PipelineFlowState& input = modApp->evaluateInputPreliminary();
		if(const PropertyContainer* container = input.getLeafObject(delegate()->subject())) {
			PropertyReference bestProperty;
			for(const PropertyObject* property : container->properties()) {
				bestProperty = PropertyReference(&delegate()->containerClass(), property, (property->componentCount() > 1) ? 0 : -1);
			}
			if(!bestProperty.isNull()) {
				setSourceProperty(bestProperty);
			}
		}
	}
}

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void SpatialBinningModifier::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(AsynchronousDelegatingModifier::delegate) && !dataset()->undoStack().isUndoingOrRedoing() && !isBeingLoaded()) {
		setSourceProperty(sourceProperty().convertToContainerClass(delegate() ? &delegate()->containerClass() : nullptr));
	}
	AsynchronousDelegatingModifier::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the 
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> SpatialBinningModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get the delegate object that will take of the specific details.
	if(!delegate())
		throwException(tr("No delegate set for the binning modifier."));
	if(sourceProperty().isNull())
		throwException(tr("No input property for binning has been selected."));

	// Look up the property container which we will operate on.
	const PropertyContainer* container = input.expectLeafObject(delegate()->subject());
	if(sourceProperty().containerClass() != &container->getOOMetaClass())
		throwException(tr("Property %1 to be binned is not a %2 property.").arg(sourceProperty().name()).arg(container->getOOMetaClass().elementDescriptionName()));

	// Get the number of input elements.
	size_t nelements = container->elementCount();

	// Get selection property.
	ConstPropertyPtr selectionProperty;
	if(onlySelectedElements()) {
		selectionProperty = container->getPropertyStorage(PropertyStorage::GenericSelectionProperty);
		if(!selectionProperty)
			throwException(tr("Binning modifier has been restricted to selected elements, but no selection was previously defined."));
	}

	// Get input property to be binned.
    const PropertyObject* sourcePropertyObj = sourceProperty().findInContainer(container);
    if(!sourcePropertyObj)
        throwException(tr("Source property '%1' not found in the input data.").arg(sourceProperty().nameWithComponent()));
	ConstPropertyPtr sourcePropertyData = sourcePropertyObj->storage();
    size_t vecComponent = (sourceProperty().vectorComponent() != -1) ? sourceProperty().vectorComponent() : 0;
    if(vecComponent >= sourcePropertyData->componentCount())
        throwException(tr("Selected vector component of source property '%1' is out of range.").arg(sourceProperty().nameWithComponent()));

    // Set up the spatial grid.
    Vector3I binCount;
	binCount.x() = std::max(1, numberOfBinsX());
	binCount.y() = std::max(1, numberOfBinsY());
	binCount.z() = std::max(1, numberOfBinsZ());
    if(is1D()) binCount.y() = binCount.z() = 1;
    else if(is2D()) binCount.z() = 1;
    size_t binDataSize = (size_t)binCount[0] * (size_t)binCount[1] * (size_t)binCount[2];
	auto binData = std::make_shared<PropertyStorage>(binDataSize, PropertyStorage::Float, 1, 0, sourceProperty().nameWithComponent(), true, DataSeriesObject::YProperty);

    // Determine coordinate axes (0, 1, 2 -- or 3 if not used).
    Vector3I binDir;
    binDir.x() = binDirectionX(binDirection());
    binDir.y() = binDirectionY(binDirection());
    binDir.z() = binDirectionZ(binDirection());

    // Get the simulation cell information.
	const SimulationCell& cell = input.expectObject<SimulationCellObject>()->data();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return delegate()->createEngine(time, input, cell, 
		binDirection(),
        std::move(sourcePropertyData), 
        vecComponent,
        std::move(selectionProperty), 
        std::move(binData), binCount, binDir, 
        reductionOperation(), firstDerivative());
}

/******************************************************************************
* Compute first derivative using finite differences.
******************************************************************************/
void SpatialBinningModifierDelegate::SpatialBinningEngine::computeGradient()
{
   if(_computeFirstDerivative) {
        FloatType binSpacing = cell().matrix().column(binDir(0)).length() / binCount(0);
        if(binCount(0) > 1 && binSpacing > 0) {
            OVITO_ASSERT(binData()->componentCount() == 1);
        	auto derivativeData = std::make_shared<PropertyStorage>(binData()->size(), PropertyStorage::Float, binData()->componentCount(), 0, binData()->name(), false, binData()->type());
			for(int j = 0; j < binCount(1); j++) {
				for(int i = 0; i < binCount(0); i++) {
					int ndx = 2;
					int i_plus_1 = i + 1;
					int i_minus_1 = i - 1;
					if(i_plus_1 == binCount(0)) {
						if(cell().pbcFlags()[binDir(0)]) i_plus_1 = 0;
						else { i_plus_1 = binCount(0)-1; ndx = 1; }
					}
					if(i_minus_1 == -1) {
						if(cell().pbcFlags()[binDir(0)]) i_minus_1 = binCount(0)-1;
						else { i_minus_1 = 0; ndx = 1; }
					}
                    size_t binIndexOut = (size_t)j*binCount(0) + (size_t)i;
                    size_t binIndexInPlus = (size_t)j*binCount(0) + (size_t)i_plus_1;
                    size_t binIndexInMinus = (size_t)j*binCount(0) + (size_t)i_minus_1;
					derivativeData->setFloat(binIndexOut, (binData()->getFloat(binIndexInPlus) - binData()->getFloat(binIndexInMinus)) / (ndx * binSpacing));
				}
			}
			_binData = std::move(derivativeData);
        }
        else std::fill(binData()->dataFloat(), binData()->dataFloat() + binData()->size(), 0.0);
    }
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState SpatialBinningModifierDelegate::SpatialBinningEngine::emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	SpatialBinningModifier* modifier = static_object_cast<SpatialBinningModifier>(modApp->modifier());
	if(!modifier->delegate())
		modifier->throwException(tr("No delegate set for the binning modifier."));

	PipelineFlowState output = input;

#if 0
	if(SpatialBinningModifier::bin1D((SpatialBinningModifier::BinDirectionType)binningDirection())) {
		DataSeriesObject* seriesObj = poh.outputDataSeries(QStringLiteral("binning"), binData()->name(), binData());
		seriesObj->setIntervalStart(0);
		seriesObj->setIntervalEnd(cell().matrix().column(binDir(0)).length());
		seriesObj->setAxisLabelX(tr("Position"));
	}
#endif

	return output;
}

}	// End of namespace
}	// End of namespace
