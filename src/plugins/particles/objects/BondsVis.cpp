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

#include <plugins/particles/Particles.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/dataset/data/VersionedDataObjectRef.h>
#include <core/rendering/SceneRenderer.h>
#include <core/rendering/ArrowPrimitive.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include "BondsVis.h"
#include "ParticlesVis.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(BondsVis);	
IMPLEMENT_OVITO_CLASS(BondPickInfo);
DEFINE_PROPERTY_FIELD(BondsVis, bondWidth);
DEFINE_PROPERTY_FIELD(BondsVis, bondColor);
DEFINE_PROPERTY_FIELD(BondsVis, useParticleColors);
DEFINE_PROPERTY_FIELD(BondsVis, shadingMode);
DEFINE_PROPERTY_FIELD(BondsVis, renderingQuality);
SET_PROPERTY_FIELD_LABEL(BondsVis, bondWidth, "Bond width");
SET_PROPERTY_FIELD_LABEL(BondsVis, bondColor, "Bond color");
SET_PROPERTY_FIELD_LABEL(BondsVis, useParticleColors, "Use particle colors");
SET_PROPERTY_FIELD_LABEL(BondsVis, shadingMode, "Shading mode");
SET_PROPERTY_FIELD_LABEL(BondsVis, renderingQuality, "RenderingQuality");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(BondsVis, bondWidth, WorldParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
BondsVis::BondsVis(DataSet* dataset) : DataVis(dataset),
	_bondWidth(0.4), 
	_bondColor(0.6, 0.6, 0.6), 
	_useParticleColors(true),
	_shadingMode(ArrowPrimitive::NormalShading),
	_renderingQuality(ArrowPrimitive::HighQuality)
{
}

/******************************************************************************
* Computes the bounding box of the visual element.
******************************************************************************/
Box3 BondsVis::boundingBox(TimePoint time, DataObject* dataObject, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	BondProperty* bondTopologyProperty = dynamic_object_cast<BondProperty>(dataObject);
	if(bondTopologyProperty->type() != BondProperty::TopologyProperty) bondTopologyProperty = nullptr;
	BondProperty* bondPeriodicImageProperty = BondProperty::findInState(flowState, BondProperty::PeriodicImageProperty);
	ParticleProperty* positionProperty = ParticleProperty::findInState(flowState, ParticleProperty::PositionProperty);
	SimulationCellObject* simulationCell = flowState.findObject<SimulationCellObject>();

	// The key type used for caching the computed bounding box:
	using CacheKey = std::tuple<
		VersionedDataObjectRef,		// Bond topology property + revision number
		VersionedDataObjectRef,		// Bond PBC vector property + revision number
		VersionedDataObjectRef,		// Particle position property + revision number
		VersionedDataObjectRef,		// Simulation cell + revision number
		FloatType					// Bond width
	>;

	// Look up the bounding box in the vis cache.
	auto& bbox = dataset()->visCache().get<Box3>(CacheKey(
			bondTopologyProperty,
			bondPeriodicImageProperty,
			positionProperty,
			simulationCell,
			bondWidth()));

	// Check if the cached bounding box information is still up to date.
	if(bbox.isEmpty()) {

		// If not, recompute bounding box from bond data.
		if(bondTopologyProperty && positionProperty) {

			size_t particleCount = positionProperty->size();
			const Point3* positions = positionProperty->constDataPoint3();
			const AffineTransformation cell = simulationCell ? simulationCell->cellMatrix() : AffineTransformation::Zero();

			for(size_t bondIndex = 0; bondIndex < bondTopologyProperty->size(); bondIndex++) {
				size_t index1 = bondTopologyProperty->getInt64Component(bondIndex, 0);
				size_t index2 = bondTopologyProperty->getInt64Component(bondIndex, 1);
				if(index1 >= particleCount || index2 >= particleCount)
					continue;

				bbox.addPoint(positions[index1]);
				bbox.addPoint(positions[index2]);
				if(bondPeriodicImageProperty && bondPeriodicImageProperty->getVector3I(bondIndex) != Vector3I::Zero()) {
					Vector3 vec = positions[index2] - positions[index1];
					const Vector3I& pbcShift = bondPeriodicImageProperty->getVector3I(bondIndex);
					for(size_t k = 0; k < 3; k++) {
						if(pbcShift[k] != 0) vec += cell.column(k) * (FloatType)pbcShift[k];
					}
					bbox.addPoint(positions[index1] + (vec * FloatType(0.5)));
					bbox.addPoint(positions[index2] - (vec * FloatType(0.5)));
				}
			}

			bbox = bbox.padBox(bondWidth() / 2);
		}
	}
	return bbox;
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
void BondsVis::render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode)
{
	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, dataObject, contextNode, flowState, validityInterval));
		return;
	}
	
	BondProperty* bondTopologyProperty = dynamic_object_cast<BondProperty>(dataObject);
	if(bondTopologyProperty->type() != BondProperty::TopologyProperty) bondTopologyProperty = nullptr;
	BondProperty* bondPeriodicImageProperty = BondProperty::findInState(flowState, BondProperty::PeriodicImageProperty);
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
	if(bondTopologyProperty && bondTopologyProperty->size() * 2 > (size_t)std::numeric_limits<int>::max()) {
		qWarning() << "WARNING: Cannot render more than" << (std::numeric_limits<int>::max()/2) << "bonds.";
		return;
	}

	// The key type used for caching the rendering primitive:
	using CacheKey = std::tuple<
		CompatibleRendererGroup,	// The scene renderer
		VersionedDataObjectRef,		// Bond topology property + revision number
		VersionedDataObjectRef,		// Bond PBC vector property + revision number
		VersionedDataObjectRef,		// Particle position property + revision number
		VersionedDataObjectRef,		// Particle color property + revision number
		VersionedDataObjectRef,		// Particle type property + revision number
		VersionedDataObjectRef,		// Bond color property + revision number
		VersionedDataObjectRef,		// Bond type property + revision number
		VersionedDataObjectRef,		// Bond selection property + revision number
		VersionedDataObjectRef,		// Simulation cell + revision number
		FloatType,					// Bond width
		Color,						// Bond color
		bool						// Use particle colors
	>;

	// Lookup the rendering primitive in the vis cache.
	auto& arrowPrimitive = dataset()->visCache().get<std::shared_ptr<ArrowPrimitive>>(CacheKey(
			renderer,
			bondTopologyProperty,
			bondPeriodicImageProperty,
			positionProperty,
			particleColorProperty,
			particleTypeProperty,
			bondColorProperty,
			bondTypeProperty,
			bondSelectionProperty,
			simulationCell,
			bondWidth(), 
			bondColor(), 
			useParticleColors()));

	// Check if we already have a valid rendering primitive that is up to date.
	if(!arrowPrimitive 
			|| !arrowPrimitive->isValid(renderer)
			|| !arrowPrimitive->setShadingMode(shadingMode())
			|| !arrowPrimitive->setRenderingQuality(renderingQuality())) {

		FloatType bondRadius = bondWidth() / 2;
		if(bondTopologyProperty && positionProperty && bondRadius > 0) {

			// Create bond geometry buffer.
			arrowPrimitive = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, shadingMode(), renderingQuality());
			arrowPrimitive->startSetElements((int)bondTopologyProperty->size() * 2);

			// Obtain particle vis element.
			ParticlesVis* particleVis = nullptr;
			if(useParticleColors()) {
				for(DataVis* vis : positionProperty->visElements()) {
					if((particleVis = dynamic_object_cast<ParticlesVis>(vis)))
						break;
				}
			}

			// Determine half-bond colors.
			std::vector<Color> colors = halfBondColors(positionProperty->size(), bondTopologyProperty,
					bondColorProperty, bondTypeProperty, bondSelectionProperty,
					particleVis, particleColorProperty, particleTypeProperty);
			OVITO_ASSERT(colors.size() == arrowPrimitive->elementCount());

			// Cache some variables.
			size_t particleCount = positionProperty->size();
			const Point3* positions = positionProperty->constDataPoint3();
			const AffineTransformation cell = simulationCell ? simulationCell->cellMatrix() : AffineTransformation::Zero();

			int elementIndex = 0;
			auto color = colors.cbegin();
			for(size_t bondIndex = 0; bondIndex < bondTopologyProperty->size(); bondIndex++) {
				size_t index1 = bondTopologyProperty->getInt64Component(bondIndex, 0);
				size_t index2 = bondTopologyProperty->getInt64Component(bondIndex, 1);
				if(index1 < particleCount && index2 < particleCount) {
					Vector3 vec = positions[index2] - positions[index1];
					if(bondPeriodicImageProperty) {
						for(size_t k = 0; k < 3; k++)
							if(int d = bondPeriodicImageProperty->getIntComponent(bondIndex, k)) vec += cell.column(k) * (FloatType)d;
					}
					arrowPrimitive->setElement(elementIndex++, positions[index1], vec * FloatType( 0.5), (ColorA)*color++, bondRadius);
					arrowPrimitive->setElement(elementIndex++, positions[index2], vec * FloatType(-0.5), (ColorA)*color++, bondRadius);
				}
				else {
					arrowPrimitive->setElement(elementIndex++, Point3::Origin(), Vector3::Zero(), (ColorA)*color++, 0);
					arrowPrimitive->setElement(elementIndex++, Point3::Origin(), Vector3::Zero(), (ColorA)*color++, 0);
				}
			}

			arrowPrimitive->endSetElements();
		}
		else arrowPrimitive.reset();
	}

	if(!arrowPrimitive)
		return;

	if(renderer->isPicking()) {
		OORef<BondPickInfo> pickInfo(new BondPickInfo(flowState));
		renderer->beginPickObject(contextNode, pickInfo);
	}

	arrowPrimitive->render(renderer);

	if(renderer->isPicking()) {
		renderer->endPickObject();
	}
}

/******************************************************************************
* Determines the display colors of half-bonds.
* Returns an array with two colors per full bond, because the two half-bonds 
* may have different colors.
******************************************************************************/
std::vector<Color> BondsVis::halfBondColors(size_t particleCount, BondProperty* topologyProperty,
		BondProperty* bondColorProperty, BondProperty* bondTypeProperty, BondProperty* bondSelectionProperty,
		ParticlesVis* particleDisplay, ParticleProperty* particleColorProperty, ParticleProperty* particleTypeProperty)
{
	OVITO_ASSERT(topologyProperty != nullptr && topologyProperty->type() == BondProperty::TopologyProperty);
	OVITO_ASSERT(bondColorProperty == nullptr || bondColorProperty->type() == BondProperty::ColorProperty);
	OVITO_ASSERT(bondTypeProperty == nullptr || bondTypeProperty->type() == BondProperty::TypeProperty);
	OVITO_ASSERT(bondSelectionProperty == nullptr || bondSelectionProperty->type() == BondProperty::SelectionProperty);
	
	std::vector<Color> output(topologyProperty->size() * 2);
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
		auto bond = topologyProperty->constDataInt64();
		for(auto bc = output.begin(); bc != output.end(); bond += 2) {
			if(bond[0] < particleCount && bond[1] < particleCount) {
				*bc++ = particleColors[bond[0]];
				*bc++ = particleColors[bond[1]];
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
QString BondPickInfo::infoString(PipelineSceneNode* objectNode, quint32 subobjectId)
{
	QString str;
	size_t bondIndex = subobjectId / 2;
	BondProperty* topologyProperty = BondProperty::findInState(pipelineState(), BondProperty::TopologyProperty);
	if(topologyProperty && topologyProperty->size() > bondIndex) {
		size_t index1 = topologyProperty->getInt64Component(bondIndex, 0);
		size_t index2 = topologyProperty->getInt64Component(bondIndex, 1);
		str = tr("Bond");

		// Bond length
		ParticleProperty* posProperty = ParticleProperty::findInState(pipelineState(), ParticleProperty::PositionProperty);
		if(posProperty && posProperty->size() > index1 && posProperty->size() > index2) {
			const Point3& p1 = posProperty->getPoint3(index1);
			const Point3& p2 = posProperty->getPoint3(index2);
			Vector3 delta = p2 - p1;
			if(BondProperty* periodicImageProperty = BondProperty::findInState(pipelineState(), BondProperty::PeriodicImageProperty)) {
				if(SimulationCellObject* simCell = pipelineState().findObject<SimulationCellObject>()) {
					delta += simCell->cellMatrix() * Vector3(periodicImageProperty->getVector3I(bondIndex));
				}
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
		if(typeProperty && typeProperty->size() > index1 && typeProperty->size() > index2) {
			ElementType* type1 = typeProperty->elementType(typeProperty->getInt(index1));
			ElementType* type2 = typeProperty->elementType(typeProperty->getInt(index2));
			if(type1 && type2) {
				str += QString(" | Particles: %1 - %2").arg(type1->name(), type2->name());
			}
		}
	}
	return str;
}

}	// End of namespace
}	// End of namespace
