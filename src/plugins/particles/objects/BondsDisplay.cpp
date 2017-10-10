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

#include <plugins/particles/Particles.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/rendering/SceneRenderer.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include "BondsDisplay.h"
#include "ParticleDisplay.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(BondsDisplay);	
IMPLEMENT_OVITO_CLASS(BondPickInfo);
DEFINE_PROPERTY_FIELD(BondsDisplay, bondWidth);
DEFINE_PROPERTY_FIELD(BondsDisplay, bondColor);
DEFINE_PROPERTY_FIELD(BondsDisplay, useParticleColors);
DEFINE_PROPERTY_FIELD(BondsDisplay, shadingMode);
DEFINE_PROPERTY_FIELD(BondsDisplay, renderingQuality);
SET_PROPERTY_FIELD_LABEL(BondsDisplay, bondWidth, "Bond width");
SET_PROPERTY_FIELD_LABEL(BondsDisplay, bondColor, "Bond color");
SET_PROPERTY_FIELD_LABEL(BondsDisplay, useParticleColors, "Use particle colors");
SET_PROPERTY_FIELD_LABEL(BondsDisplay, shadingMode, "Shading mode");
SET_PROPERTY_FIELD_LABEL(BondsDisplay, renderingQuality, "RenderingQuality");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(BondsDisplay, bondWidth, WorldParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
BondsDisplay::BondsDisplay(DataSet* dataset) : DisplayObject(dataset),
	_bondWidth(0.4), 
	_bondColor(0.6, 0.6, 0.6), 
	_useParticleColors(true),
	_shadingMode(ArrowPrimitive::NormalShading),
	_renderingQuality(ArrowPrimitive::HighQuality)
{
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 BondsDisplay::boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	BondsObject* bondsObj = dynamic_object_cast<BondsObject>(dataObject);
	ParticleProperty* positionProperty = ParticleProperty::findInState(flowState, ParticleProperty::PositionProperty);
	SimulationCellObject* simulationCell = flowState.findObject<SimulationCellObject>();

	// Detect if the input data has changed since the last time we computed the bounding box.
	if(_boundingBoxCacheHelper.updateState(
			bondsObj,
			positionProperty,
			simulationCell,
			bondWidth())) {

		// Recompute bounding box.
		_cachedBoundingBox.setEmpty();
		if(bondsObj && positionProperty) {

			size_t particleCount = positionProperty->size();
			const Point3* positions = positionProperty->constDataPoint3();
			const AffineTransformation cell = simulationCell ? simulationCell->cellMatrix() : AffineTransformation::Zero();

			for(const Bond& bond : *bondsObj->storage()) {
				if(bond.index1 >= particleCount || bond.index2 >= particleCount)
					continue;

				_cachedBoundingBox.addPoint(positions[bond.index1]);
				_cachedBoundingBox.addPoint(positions[bond.index2]);
				if(bond.pbcShift != Vector_3<int8_t>::Zero()) {
					Vector3 vec = positions[bond.index2] - positions[bond.index1];
					for(size_t k = 0; k < 3; k++)
						if(bond.pbcShift[k] != 0) vec += cell.column(k) * (FloatType)bond.pbcShift[k];
					_cachedBoundingBox.addPoint(positions[bond.index1] + (vec * FloatType(0.5)));
					_cachedBoundingBox.addPoint(positions[bond.index2] - (vec * FloatType(0.5)));
				}
			}

			_cachedBoundingBox = _cachedBoundingBox.padBox(bondWidth() / 2);
		}
	}
	return _cachedBoundingBox;
}

/******************************************************************************
* Lets the display object render the data object.
******************************************************************************/
void BondsDisplay::render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode)
{
	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, dataObject, contextNode, flowState, validityInterval));
		return;
	}
	
	BondsObject* bondsObj = dynamic_object_cast<BondsObject>(dataObject);
	ParticleProperty* positionProperty = ParticleProperty::findInState(flowState, ParticleProperty::PositionProperty);
	SimulationCellObject* simulationCell = flowState.findObject<SimulationCellObject>();
	ParticleProperty* particleColorProperty = ParticleProperty::findInState(flowState, ParticleProperty::ColorProperty);
	ParticleProperty* particleTypeProperty = ParticleProperty::findInState(flowState, ParticleProperty::TypeProperty);
	BondProperty* bondTypeProperty = BondProperty::findInState(flowState, BondProperty::TypeProperty);
	BondProperty* bondColorProperty = BondProperty::findInState(flowState, BondProperty::ColorProperty);
	BondProperty* bondSelectionProperty = BondProperty::findInState(flowState, BondProperty::SelectionProperty);
	if(!useParticleColors()) {
		particleColorProperty = nullptr;
		particleTypeProperty = nullptr;
	}

	// Make sure we don't exceed our internal limits.
	if(bondsObj && bondsObj->storage()->size() * 2 > (size_t)std::numeric_limits<int>::max()) {
		qWarning() << "WARNING: Cannot render more than" << (std::numeric_limits<int>::max()/2) << "bonds.";
		return;
	}	

	if(_geometryCacheHelper.updateState(
			bondsObj,
			positionProperty,
			particleColorProperty,
			particleTypeProperty,
			bondColorProperty,
			bondTypeProperty,
			bondSelectionProperty,
			simulationCell,
			bondWidth(), bondColor(), useParticleColors())
			|| !_buffer	|| !_buffer->isValid(renderer)
			|| !_buffer->setShadingMode(shadingMode())
			|| !_buffer->setRenderingQuality(renderingQuality())) {

		FloatType bondRadius = bondWidth() / 2;
		if(bondsObj && positionProperty && bondRadius > 0) {

			// Create bond geometry buffer.
			_buffer = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, shadingMode(), renderingQuality());
			_buffer->startSetElements((int)bondsObj->storage()->size() * 2);

			// Obtain particle display object.
			ParticleDisplay* particleDisplay = nullptr;
			if(useParticleColors()) {
				for(DisplayObject* displayObj : positionProperty->displayObjects()) {
					particleDisplay = dynamic_object_cast<ParticleDisplay>(displayObj);
					if(particleDisplay) break;
				}
			}

			// Determine half-bond colors.
			std::vector<Color> colors = halfBondColors(positionProperty->size(), bondsObj,
					bondColorProperty, bondTypeProperty, bondSelectionProperty,
					particleDisplay, particleColorProperty, particleTypeProperty);
			OVITO_ASSERT(colors.size() == _buffer->elementCount());

			// Cache some variables.
			size_t particleCount = positionProperty->size();
			const Point3* positions = positionProperty->constDataPoint3();
			const AffineTransformation cell = simulationCell ? simulationCell->cellMatrix() : AffineTransformation::Zero();

			int elementIndex = 0;
			auto color = colors.cbegin();
			for(const Bond& bond : *bondsObj->storage()) {
				if(bond.index1 < particleCount && bond.index2 < particleCount) {
					Vector3 vec = positions[bond.index2] - positions[bond.index1];
					for(size_t k = 0; k < 3; k++)
						if(bond.pbcShift[k] != 0) vec += cell.column(k) * (FloatType)bond.pbcShift[k];
					_buffer->setElement(elementIndex++, positions[bond.index1], vec * FloatType( 0.5), (ColorA)*color++, bondRadius);
					_buffer->setElement(elementIndex++, positions[bond.index2], vec * FloatType(-0.5), (ColorA)*color++, bondRadius);
				}
				else {
					_buffer->setElement(elementIndex++, Point3::Origin(), Vector3::Zero(), (ColorA)*color++, 0);
					_buffer->setElement(elementIndex++, Point3::Origin(), Vector3::Zero(), (ColorA)*color++, 0);
				}
			}

			_buffer->endSetElements();
		}
		else _buffer.reset();
	}

	if(!_buffer)
		return;

	if(renderer->isPicking()) {
		OORef<BondPickInfo> pickInfo(new BondPickInfo(bondsObj, flowState));
		renderer->beginPickObject(contextNode, pickInfo);
	}

	_buffer->render(renderer);

	if(renderer->isPicking()) {
		renderer->endPickObject();
	}
}

/******************************************************************************
* Determines the display colors of half-bonds.
* Returns an array with two colors per full bond, because the two half-bonds 
* may have different colors.
******************************************************************************/
std::vector<Color> BondsDisplay::halfBondColors(size_t particleCount, BondsObject* bondsObject,
		BondProperty* bondColorProperty, BondProperty* bondTypeProperty, BondProperty* bondSelectionProperty,
		ParticleDisplay* particleDisplay, ParticleProperty* particleColorProperty, ParticleProperty* particleTypeProperty)
{
	OVITO_ASSERT(bondColorProperty == nullptr || bondColorProperty->type() == BondProperty::ColorProperty);
	OVITO_ASSERT(bondTypeProperty == nullptr || bondTypeProperty->type() == BondProperty::TypeProperty);
	OVITO_ASSERT(bondSelectionProperty == nullptr || bondSelectionProperty->type() == BondProperty::SelectionProperty);
	
	std::vector<Color> output(bondsObject->size() * 2);
	if(bondColorProperty && bondColorProperty->size() * 2 == output.size()) {
		// Take bond colors directly from the color property.
		auto bc = output.begin();
		for(const Color& c : bondColorProperty->constColorRange()) {
			*bc++ = c;
			*bc++ = c;
		}
	}
	else if(useParticleColors() && particleDisplay != nullptr) {
		// Derive bond colors from particle colors.
		std::vector<Color> particleColors(particleCount);
		particleDisplay->particleColors(particleColors, particleColorProperty, particleTypeProperty, nullptr);
		auto bc = output.begin();
		for(const Bond& bond : *bondsObject->storage()) {
			if(bond.index1 < particleCount && bond.index2 < particleCount) {
				*bc++ = particleColors[bond.index1];
				*bc++ = particleColors[bond.index2];
			}
			else {
				*bc++ = bondColor();
				*bc++ = bondColor();
			}
		}
	}
	else {
		Color defaultColor = bondColor();
		if(bondTypeProperty && bondTypeProperty->size() * 2 == output.size()) {
			// Assign colors based on bond types.
			// Generate a lookup map for bond type colors.
			const std::map<int,Color> colorMap = bondTypeProperty->typeColorMap();
			std::array<Color,16> colorArray;
			// Check if all type IDs are within a small, non-negative range.
			// If yes, we can use an array lookup strategy. Otherwise we have to use a dictionary lookup strategy, which is slower.
			if(std::all_of(colorMap.begin(), colorMap.end(),
					[&colorArray](const std::map<int,Color>::value_type& i) { return i.first >= 0 && i.first < (int)colorArray.size(); })) {
				colorArray.fill(defaultColor);
				for(const auto& entry : colorMap)
					colorArray[entry.first] = entry.second;
				// Fill color array.
				const int* t = bondTypeProperty->constDataInt();
				for(auto c = output.begin(); c != output.end(); ++t) {
					if(*t >= 0 && *t < (int)colorArray.size()) {
						*c++ = colorArray[*t];
						*c++ = colorArray[*t];
					}
					else {
						*c++ = defaultColor;
						*c++ = defaultColor;
					}
				}
			}
			else {
				// Fill color array.
				const int* t = bondTypeProperty->constDataInt();
				for(auto c = output.begin(); c != output.end(); ++t) {
					auto it = colorMap.find(*t);
					if(it != colorMap.end()) {
						*c++ = it->second;
						*c++ = it->second;
					}
					else {
						*c++ = defaultColor;
						*c++ = defaultColor;
					}
				}
			}
		}
		else {
			// Assign a uniform color to all bonds.
			std::fill(output.begin(), output.end(), defaultColor);
		}
	}

	// Highlight selected bonds.
	if(bondSelectionProperty && bondSelectionProperty->size() * 2 == output.size()) {
		const Color selColor = selectionBondColor();
		const int* t = bondSelectionProperty->constDataInt();
		for(auto c = output.begin(); c != output.end(); ++t) {
			if(*t) {
				*c++ = selColor;
				*c++ = selColor;
			}
			else c += 2;
		}
	}

	return output;
}

/******************************************************************************
* Returns a human-readable string describing the picked object,
* which will be displayed in the status bar by OVITO.
******************************************************************************/
QString BondPickInfo::infoString(ObjectNode* objectNode, quint32 subobjectId)
{
	QString str;
	int bondIndex = subobjectId / 2;
	if(_bondsObj && _bondsObj->storage()->size() > bondIndex) {
		str = tr("Bond");
		const Bond& bond = (*_bondsObj->storage())[bondIndex];

		// Bond length
		ParticleProperty* posProperty = ParticleProperty::findInState(pipelineState(), ParticleProperty::PositionProperty);
		if(posProperty && posProperty->size() > bond.index1 && posProperty->size() > bond.index2) {
			const Point3& p1 = posProperty->getPoint3(bond.index1);
			const Point3& p2 = posProperty->getPoint3(bond.index2);
			Vector3 delta = p2 - p1;
			if(SimulationCellObject* simCell = pipelineState().findObject<SimulationCellObject>()) {
				delta += simCell->cellMatrix() * Vector3(bond.pbcShift);
			}
			str += QString(" | Length: %1 | Delta: (%2 %3 %4)").arg(delta.length()).arg(delta.x()).arg(delta.y()).arg(delta.z());
		}

		// Bond properties
		for(DataObject* dataObj : pipelineState().objects()) {
			BondProperty* property = dynamic_object_cast<BondProperty>(dataObj);
			if(!property || property->size() <= bondIndex) continue;
			if(property->type() == BondProperty::SelectionProperty) continue;
			if(property->type() == BondProperty::ColorProperty) continue;
			if(property->dataType() != PropertyStorage::Int && property->dataType() != PropertyStorage::Int64 && property->dataType() != PropertyStorage::Float) continue;
			if(!str.isEmpty()) str += QStringLiteral(" | ");
			str += property->name();
			str += QStringLiteral(" ");
			for(size_t component = 0; component < property->componentCount(); component++) {
				if(component != 0) str += QStringLiteral(", ");
				QString valueString;
				if(property->dataType() == PropertyStorage::Int) {
					str += QString::number(property->getIntComponent(bondIndex, component));
					if(property->elementTypes().empty() == false) {
						if(ElementType* btype = property->elementType(property->getIntComponent(bondIndex, component)))
							str += QString(" (%1)").arg(btype->name());
					}
				}
				else if(property->dataType() == PropertyStorage::Int64)
					str += QString::number(property->getInt64Component(bondIndex, component));
				else if(property->dataType() == PropertyStorage::Float)
					str += QString::number(property->getFloatComponent(bondIndex, component));
			}
		}

		// Pair type info.
		ParticleProperty* typeProperty = ParticleProperty::findInState(pipelineState(), ParticleProperty::TypeProperty);
		if(typeProperty && typeProperty->size() > bond.index1 && typeProperty->size() > bond.index2) {
			ElementType* type1 = typeProperty->elementType(typeProperty->getInt(bond.index1));
			ElementType* type2 = typeProperty->elementType(typeProperty->getInt(bond.index2));
			if(type1 && type2) {
				str += QString(" | Particles: %1 - %2").arg(type1->name(), type2->name());
			}
		}
	}
	return str;
}

}	// End of namespace
}	// End of namespace
