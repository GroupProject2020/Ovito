///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
//  Copyright (2014) Lars Pastewka
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

#include <plugins/particles/Particles.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/app/Application.h>
#include <plugins/stdobj/simcell/SimulationCell.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/utilities/units/UnitsManager.h>
#include "BinningModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(BinningModifier);
DEFINE_PROPERTY_FIELD(BinningModifier, reductionOperation);
DEFINE_PROPERTY_FIELD(BinningModifier, firstDerivative);
DEFINE_PROPERTY_FIELD(BinningModifier, binDirection);
DEFINE_PROPERTY_FIELD(BinningModifier, numberOfBinsX);
DEFINE_PROPERTY_FIELD(BinningModifier, numberOfBinsY);
DEFINE_PROPERTY_FIELD(BinningModifier, fixPropertyAxisRange);
DEFINE_PROPERTY_FIELD(BinningModifier, propertyAxisRangeStart);
DEFINE_PROPERTY_FIELD(BinningModifier, propertyAxisRangeEnd);
DEFINE_PROPERTY_FIELD(BinningModifier, sourceProperty);
DEFINE_PROPERTY_FIELD(BinningModifier, onlySelected);
SET_PROPERTY_FIELD_LABEL(BinningModifier, reductionOperation, "Reduction operation");
SET_PROPERTY_FIELD_LABEL(BinningModifier, firstDerivative, "Compute first derivative");
SET_PROPERTY_FIELD_LABEL(BinningModifier, binDirection, "Bin direction");
SET_PROPERTY_FIELD_LABEL(BinningModifier, numberOfBinsX, "Number of spatial bins");
SET_PROPERTY_FIELD_LABEL(BinningModifier, numberOfBinsY, "Number of spatial bins");
SET_PROPERTY_FIELD_LABEL(BinningModifier, fixPropertyAxisRange, "Fix property axis range");
SET_PROPERTY_FIELD_LABEL(BinningModifier, propertyAxisRangeStart, "Property axis range start");
SET_PROPERTY_FIELD_LABEL(BinningModifier, propertyAxisRangeEnd, "Property axis range end");
SET_PROPERTY_FIELD_LABEL(BinningModifier, sourceProperty, "Source property");
SET_PROPERTY_FIELD_LABEL(BinningModifier, onlySelected, "Use only selected particles");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(BinningModifier, numberOfBinsX, IntegerParameterUnit, 1, 100000);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(BinningModifier, numberOfBinsY, IntegerParameterUnit, 1, 100000);

IMPLEMENT_OVITO_CLASS(BinningModifierApplication);
SET_MODIFIER_APPLICATION_TYPE(BinningModifier, BinningModifierApplication);
DEFINE_PROPERTY_FIELD(BinningModifierApplication, binData);
DEFINE_PROPERTY_FIELD(BinningModifierApplication, range1);
DEFINE_PROPERTY_FIELD(BinningModifierApplication, range2);
SET_PROPERTY_FIELD_CHANGE_EVENT(BinningModifierApplication, binData, ReferenceEvent::ObjectStatusChanged);
SET_PROPERTY_FIELD_CHANGE_EVENT(BinningModifierApplication, range1, ReferenceEvent::ObjectStatusChanged);
SET_PROPERTY_FIELD_CHANGE_EVENT(BinningModifierApplication, range2, ReferenceEvent::ObjectStatusChanged);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
BinningModifier::BinningModifier(DataSet* dataset) : Modifier(dataset), 
    _reductionOperation(RED_MEAN), 
    _firstDerivative(false),
    _binDirection(CELL_VECTOR_3), 
    _numberOfBinsX(200), 
    _numberOfBinsY(200),
    _fixPropertyAxisRange(false), 
    _propertyAxisRangeStart(0), 
    _propertyAxisRangeEnd(0),
	_onlySelected(false)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool BinningModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void BinningModifier::initializeModifier(ModifierApplication* modApp)
{
	Modifier::initializeModifier(modApp);

	// Use the first available particle property from the input state as data source when the modifier is newly created.
	if(sourceProperty().isNull() && Application::instance()->guiMode()) {
		const PipelineFlowState& input = modApp->evaluateInputPreliminary();
		ParticlePropertyReference bestProperty;
		for(DataObject* o : input.objects()) {
			ParticleProperty* property = dynamic_object_cast<ParticleProperty>(o);
			if(property && (property->dataType() == PropertyStorage::Int || property->dataType() == PropertyStorage::Float)) {
				bestProperty = ParticlePropertyReference(property, (property->componentCount() > 1) ? 0 : -1);
			}
		}
		if(!bestProperty.isNull()) {
			setSourceProperty(bestProperty);
		}
	}
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
Future<PipelineFlowState> BinningModifier::evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
    ParticleInputHelper pih(dataset(), input);

	int binDataSizeX = std::max(1, numberOfBinsX());
	int binDataSizeY = std::max(1, numberOfBinsY());
    if(is1D()) binDataSizeY = 1;
    size_t binDataSize = binDataSizeX * binDataSizeY;

	auto binData = std::make_shared<PropertyStorage>(binDataSize, PropertyStorage::Float, 1, 0, sourceProperty().nameWithComponent(), true);

    // Determine coordinate axes (0, 1 or 2).
    int binDirX = binDirectionX(binDirection());
    int binDirY = binDirectionY(binDirection());

    // Number of particles per bin for averaging.
    std::vector<size_t> numberOfParticlesPerBin(binDataSize, 0);

	// Get the source property.
	if(sourceProperty().isNull())
		throwException(tr("Please select an input particle property."));
	ParticleProperty* property = sourceProperty().findInState(input);
	if(!property)
		throwException(tr("The selected particle property with the name '%1' does not exist.").arg(sourceProperty().name()));
	if(sourceProperty().vectorComponent() >= (int)property->componentCount())
		throwException(tr("The selected vector component is out of range. The particle property '%1' contains only %2 values per particle.").arg(sourceProperty().name()).arg(property->componentCount()));

	size_t vecComponent = std::max(0, sourceProperty().vectorComponent());
	size_t vecComponentCount = property->componentCount();

	// Get input selection.
	PropertyStorage* inputSelectionProperty = nullptr;
	if(onlySelected()) {
		inputSelectionProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::SelectionProperty)->storage().get();
		OVITO_ASSERT(inputSelectionProperty->size() == property->size());
	}

    // Get bottom-left and top-right corner of the simulation cell.
	SimulationCell cell = pih.expectSimulationCell()->data();
    AffineTransformation reciprocalCell = cell.inverseMatrix();

    // Get periodic boundary flag.
	auto pbc = cell.pbcFlags();

    // Compute the surface normal vector.
    Vector3 normalX, normalY(1, 1, 1);
    if(binDirection() == CELL_VECTOR_1) {
        normalX = cell.matrix().column(1).cross(cell.matrix().column(2));
    }
    else if(binDirection() == CELL_VECTOR_2) {
        normalX = cell.matrix().column(2).cross(cell.matrix().column(0));
    }
    else if(binDirection() == CELL_VECTOR_3) {
        normalX = cell.matrix().column(0).cross(cell.matrix().column(1));
    }
    else if(binDirection() == CELL_VECTORS_1_2) {
        normalX = cell.matrix().column(1).cross(cell.matrix().column(2));
        normalY = cell.matrix().column(2).cross(cell.matrix().column(0));
    }
    else if(_binDirection == CELL_VECTORS_2_3) {
        normalX = cell.matrix().column(2).cross(cell.matrix().column(0));
        normalY = cell.matrix().column(0).cross(cell.matrix().column(1));
    }
    else if(binDirection() == CELL_VECTORS_1_3) {
        normalX = cell.matrix().column(1).cross(cell.matrix().column(2));
        normalY = cell.matrix().column(0).cross(cell.matrix().column(1));
    }
	if(normalX == Vector3::Zero() || normalY == Vector3::Zero())
		throwException(tr("Simulation cell is degenerate."));

    // Compute the distance of the two cell faces (normal.length() is area of face).
    FloatType cellVolume = cell.volume3D();
    FloatType xAxisRangeStart = cell.matrix().translation().dot(normalX.normalized());
    FloatType xAxisRangeEnd = xAxisRangeStart + cellVolume / normalX.length();
    FloatType yAxisRangeStart = 0;
    FloatType yAxisRangeEnd = 0;
    if(!is1D()) {
        yAxisRangeStart = cell.matrix().translation().dot(normalY.normalized());
		yAxisRangeEnd = yAxisRangeStart + cellVolume / normalY.length();
    }

	// Get the particle positions.
	ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);
	OVITO_ASSERT(posProperty->size() == property->size());

	if(property->size() > 0) {
        const Point3* pos = posProperty->constDataPoint3();
        const Point3* pos_end = pos + posProperty->size();

		if(property->dataType() == PropertyStorage::Float) {
			const FloatType* v = property->constDataFloat() + vecComponent;
			const FloatType* v_end = v + (property->size() * vecComponentCount);
			const int* sel = inputSelectionProperty ? inputSelectionProperty->constDataInt() : nullptr;
            for(; v != v_end; v += vecComponentCount, ++pos) {
				if(sel && !*sel++) continue;
                if(!std::isnan(*v)) {
                    FloatType fractionalPosX = reciprocalCell.prodrow(*pos, binDirX);
                    FloatType fractionalPosY = reciprocalCell.prodrow(*pos, binDirY);
                    int binIndexX = int( fractionalPosX * binDataSizeX );
                    int binIndexY = int( fractionalPosY * binDataSizeY );
                    if(pbc[binDirX]) binIndexX = SimulationCell::modulo(binIndexX, binDataSizeX);
                    if(pbc[binDirY]) binIndexY = SimulationCell::modulo(binIndexY, binDataSizeY);
                    if(binIndexX >= 0 && binIndexX < binDataSizeX && binIndexY >= 0 && binIndexY < binDataSizeY) {
                        size_t binIndex = binIndexY*binDataSizeX+binIndexX;
                        if(reductionOperation() == RED_MEAN || reductionOperation() == RED_SUM || reductionOperation() == RED_SUM_VOL) {
                            binData->setFloat(binIndex, binData->getFloat(binIndex) + (*v));
                        } 
                        else {
                            if(numberOfParticlesPerBin[binIndex] == 0) {
                                binData->setFloat(binIndex, *v);
                            }
                            else {
                                if(reductionOperation() == RED_MAX) {
                                    binData->setFloat(binIndex, std::max(binData->getFloat(binIndex), (FloatType)*v));
                                }
                                else if(reductionOperation() == RED_MIN) {
                                    binData->setFloat(binIndex, std::min(binData->getFloat(binIndex), (FloatType)*v));
                                }
                            }
                        }
                        numberOfParticlesPerBin[binIndex]++;
                    }
                }
            }
		}
		else if(property->dataType() == PropertyStorage::Int) {
			const int* v = property->constDataInt() + vecComponent;
			const int* v_end = v + (property->size() * vecComponentCount);
			const int* sel = inputSelectionProperty ? inputSelectionProperty->constDataInt() : nullptr;
            for(; v != v_end; v += vecComponentCount, ++pos) {
				if(sel && !*sel++) continue;
                FloatType fractionalPosX = reciprocalCell.prodrow(*pos, binDirX);
                FloatType fractionalPosY = reciprocalCell.prodrow(*pos, binDirY);
                int binIndexX = int( fractionalPosX * binDataSizeX );
                int binIndexY = int( fractionalPosY * binDataSizeY );
                if(pbc[binDirX])  binIndexX = SimulationCell::modulo(binIndexX, binDataSizeX);
                if(pbc[binDirY])  binIndexY = SimulationCell::modulo(binIndexY, binDataSizeY);
                if(binIndexX >= 0 && binIndexX < binDataSizeX && binIndexY >= 0 && binIndexY < binDataSizeY) {
                    size_t binIndex = binIndexY*binDataSizeX+binIndexX;
                    if(reductionOperation() == RED_MEAN || reductionOperation() == RED_SUM || reductionOperation() == RED_SUM_VOL) {
                        binData->setFloat(binIndex, binData->getFloat(binIndex) + (*v));
                    }
                    else {
                        if(numberOfParticlesPerBin[binIndex] == 0) {
                            binData->setFloat(binIndex, *v);  
                        }
                        else {
                            if(reductionOperation() == RED_MAX) {
                                binData->setFloat(binIndex, std::max(binData->getFloat(binIndex), (FloatType)*v));
                            }
                            else if(reductionOperation() == RED_MIN) {
                                binData->setFloat(binIndex, std::min(binData->getFloat(binIndex), (FloatType)*v));
                            }
                        }
                    }
                    numberOfParticlesPerBin[binIndex]++;
                }
            }
        }
		else if(property->dataType() == PropertyStorage::Int64) {
			auto v = property->constDataInt64() + vecComponent;
			auto v_end = v + (property->size() * vecComponentCount);
			auto sel = inputSelectionProperty ? inputSelectionProperty->constDataInt() : nullptr;
            for(; v != v_end; v += vecComponentCount, ++pos) {
				if(sel && !*sel++) continue;
                FloatType fractionalPosX = reciprocalCell.prodrow(*pos, binDirX);
                FloatType fractionalPosY = reciprocalCell.prodrow(*pos, binDirY);
                int binIndexX = int( fractionalPosX * binDataSizeX );
                int binIndexY = int( fractionalPosY * binDataSizeY );
                if(pbc[binDirX])  binIndexX = SimulationCell::modulo(binIndexX, binDataSizeX);
                if(pbc[binDirY])  binIndexY = SimulationCell::modulo(binIndexY, binDataSizeY);
                if(binIndexX >= 0 && binIndexX < binDataSizeX && binIndexY >= 0 && binIndexY < binDataSizeY) {
                    size_t binIndex = binIndexY*binDataSizeX+binIndexX;
                    if(reductionOperation() == RED_MEAN || reductionOperation() == RED_SUM || reductionOperation() == RED_SUM_VOL) {
                        binData->setFloat(binIndex, binData->getFloat(binIndex) + (*v));
                    }
                    else {
                        if(numberOfParticlesPerBin[binIndex] == 0) {
                            binData->setFloat(binIndex, *v);  
                        }
                        else {
                            if(reductionOperation() == RED_MAX) {
                                binData->setFloat(binIndex, std::max(binData->getFloat(binIndex), (FloatType)*v));
                            }
                            else if(reductionOperation() == RED_MIN) {
                                binData->setFloat(binIndex, std::min(binData->getFloat(binIndex), (FloatType)*v));
                            }
                        }
                    }
                    numberOfParticlesPerBin[binIndex]++;
                }
            }
        }
        else {
			throwException(tr("The property '%1' has a data type that is not supported by the modifier.").arg(property->name()));            
        }

        if(reductionOperation() == RED_MEAN) {
            // Normalize.
            auto a = binData->dataFloat();
            for(auto n : numberOfParticlesPerBin) {
                if(n != 0) *a /= n;
                ++a;
            }
        }
        else if(reductionOperation() == RED_SUM_VOL) {
            // Divide by bin volume.
            FloatType binVolume = cellVolume / (binDataSizeX*binDataSizeY);
            for(FloatType& v : binData->floatRange())
                v /= binVolume;
        }
	}

	// Compute first derivative using finite differences.
    if(firstDerivative()) {
        FloatType binSpacingX = (xAxisRangeEnd - xAxisRangeStart) / binDataSizeX;
        if(binDataSizeX > 1 && xAxisRangeEnd > xAxisRangeStart) {
        	auto derivativeData = std::make_shared<PropertyStorage>(binDataSize, PropertyStorage::Float, 1, 0, sourceProperty().nameWithComponent(), false);
			for(int j = 0; j < binDataSizeY; j++) {
				for(int i = 0; i < binDataSizeX; i++) {
					int ndx = 2;
					int i_plus_1 = i+1;
					int i_minus_1 = i-1;
					if(i_plus_1 == binDataSizeX) {
						if(pbc[binDirX]) i_plus_1 = 0;
						else { i_plus_1 = binDataSizeX-1; ndx = 1; }
					}
					if(i_minus_1 == -1) {
						if(pbc[binDirX]) i_minus_1 = binDataSizeX-1;
						else { i_minus_1 = 0; ndx = 1; }
					}
					derivativeData->setFloat(j*binDataSizeX + i, (binData->getFloat(j*binDataSizeX + i_plus_1) - binData->getFloat(j*binDataSizeX + i_minus_1)) / (ndx*binSpacingX));
				}
			}
			binData = std::move(derivativeData);
        }
        else std::fill(binData->dataFloat(), binData->dataFloat() + binData->size(), 0.0);
    }

	if(!fixPropertyAxisRange()) {
		auto minmax = std::minmax_element(binData->constDataFloat(), binData->constDataFloat() + binData->size());
        setPropertyAxisRangeStart(*minmax.first);
		setPropertyAxisRangeEnd(*minmax.second);
	}

	static_object_cast<BinningModifierApplication>(modApp)->setBinData(binData);
	static_object_cast<BinningModifierApplication>(modApp)->setRange1({xAxisRangeStart, xAxisRangeEnd});
	static_object_cast<BinningModifierApplication>(modApp)->setRange2({yAxisRangeStart, yAxisRangeEnd});

	// Inform the editor component that the stored data has changed
	// and it should update the display.
	notifyDependents(ReferenceEvent::ObjectStatusChanged);

	return input;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
