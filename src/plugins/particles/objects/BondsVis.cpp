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
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/dataset/DataSet.h>
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
SET_PROPERTY_FIELD_LABEL(BondsVis, bondWidth, "Default bond width");
SET_PROPERTY_FIELD_LABEL(BondsVis, bondColor, "Default bond color");
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
Box3 BondsVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	if(objectStack.size() < 2) return {};
	const BondsObject* bonds = dynamic_object_cast<BondsObject>(objectStack.back());
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(objectStack[objectStack.size()-2]);
	if(!bonds || !particles) return {};
	particles->verifyIntegrity();
	bonds->verifyIntegrity();
	const PropertyObject* bondTopologyProperty = bonds->getProperty(BondsObject::TopologyProperty);
	const PropertyObject* bondPeriodicImageProperty = bonds->getProperty(BondsObject::PeriodicImageProperty);
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	const SimulationCellObject* simulationCell = flowState.getObject<SimulationCellObject>();

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
void BondsVis::render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, objectStack, contextNode, flowState, validityInterval));
		return;
	}

	if(objectStack.size() < 2) return;
	const BondsObject* bonds = dynamic_object_cast<BondsObject>(objectStack.back());
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(objectStack[objectStack.size()-2]);
	if(!bonds || !particles) return;
	particles->verifyIntegrity();
	bonds->verifyIntegrity();
	const PropertyObject* bondTopologyProperty = bonds->getProperty(BondsObject::TopologyProperty);
	const PropertyObject* bondPeriodicImageProperty = bonds->getProperty(BondsObject::PeriodicImageProperty);
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	const SimulationCellObject* simulationCell = flowState.getObject<SimulationCellObject>();
	const PropertyObject* particleColorProperty = particles->getProperty(ParticlesObject::ColorProperty);
	const PropertyObject* particleTypeProperty = particles->getProperty(ParticlesObject::TypeProperty);
	const PropertyObject* bondTypeProperty = bonds->getProperty(BondsObject::TypeProperty);
	const PropertyObject* bondColorProperty = bonds->getProperty(BondsObject::ColorProperty);
	const PropertyObject* bondSelectionProperty = bonds->getProperty(BondsObject::SelectionProperty);
	const PropertyObject* transparencyProperty = bonds->getProperty(BondsObject::TransparencyProperty);
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
		VersionedDataObjectRef,		// Bond transparency + revision number
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
			transparencyProperty,
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
			arrowPrimitive = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, shadingMode(), renderingQuality(), transparencyProperty != nullptr);
			arrowPrimitive->startSetElements((int)bondTopologyProperty->size() * 2);

			// Obtain particles vis element.
			ParticlesVis* particleVis = useParticleColors() ? particles->visElement<ParticlesVis>() : nullptr;

			// Determine half-bond colors.
			std::vector<ColorA> colors = halfBondColors(positionProperty->size(), bondTopologyProperty,
					bondColorProperty, bondTypeProperty, bondSelectionProperty, transparencyProperty,
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
					arrowPrimitive->setElement(elementIndex++, positions[index1], vec * FloatType( 0.5), *color++, bondRadius);
					arrowPrimitive->setElement(elementIndex++, positions[index2], vec * FloatType(-0.5), *color++, bondRadius);
				}
				else {
					arrowPrimitive->setElement(elementIndex++, Point3::Origin(), Vector3::Zero(), *color++, 0);
					arrowPrimitive->setElement(elementIndex++, Point3::Origin(), Vector3::Zero(), *color++, 0);
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
std::vector<ColorA> BondsVis::halfBondColors(size_t particleCount, const PropertyObject* topologyProperty,
		const PropertyObject* bondColorProperty, const PropertyObject* bondTypeProperty, const PropertyObject* bondSelectionProperty, const PropertyObject* transparencyProperty,
		const ParticlesVis* particleVis, const PropertyObject* particleColorProperty, const PropertyObject* particleTypeProperty) const
{
	OVITO_ASSERT(topologyProperty != nullptr && topologyProperty->type() == BondsObject::TopologyProperty);
	OVITO_ASSERT(bondColorProperty == nullptr || bondColorProperty->type() == BondsObject::ColorProperty);
	OVITO_ASSERT(bondTypeProperty == nullptr || bondTypeProperty->type() == BondsObject::TypeProperty);
	OVITO_ASSERT(bondSelectionProperty == nullptr || bondSelectionProperty->type() == BondsObject::SelectionProperty);
	OVITO_ASSERT(transparencyProperty == nullptr || transparencyProperty->type() == BondsObject::TransparencyProperty);

	std::vector<ColorA> output(topologyProperty->size() * 2);
	ColorA defaultColor = (ColorA)bondColor();
	if(bondColorProperty && bondColorProperty->size() * 2 == output.size()) {
		// Take bond colors directly from the color property.
		auto bc = output.begin();
		for(const Color& c : bondColorProperty->constColorRange()) {
			*bc++ = (ColorA)c;
			*bc++ = (ColorA)c;
		}
	}
	else if(useParticleColors() && particleVis != nullptr) {
		// Derive bond colors from particle colors.
		std::vector<ColorA> particleColors(particleCount);
		particleVis->particleColors(particleColors, particleColorProperty, particleTypeProperty, nullptr);
		auto bond = topologyProperty->constDataInt64();
		for(auto bc = output.begin(); bc != output.end(); bond += 2) {
			if(bond[0] < particleCount && bond[1] < particleCount) {
				*bc++ = particleColors[bond[0]];
				*bc++ = particleColors[bond[1]];
			}
			else {
				*bc++ = defaultColor;
				*bc++ = defaultColor;
			}
		}
	}
	else {
		if(bondTypeProperty && bondTypeProperty->size() * 2 == output.size()) {
			// Assign colors based on bond types.
			// Generate a lookup map for bond type colors.
			const std::map<int, Color>& colorMap = bondTypeProperty->typeColorMap();
			std::array<ColorA,16> colorArray;
			// Check if all type IDs are within a small, non-negative range.
			// If yes, we can use an array lookup strategy. Otherwise we have to use a dictionary lookup strategy, which is slower.
			if(std::all_of(colorMap.begin(), colorMap.end(),
					[&colorArray](const std::map<int, ColorA>::value_type& i) { return i.first >= 0 && i.first < (int)colorArray.size(); })) {
				colorArray.fill(defaultColor);
				for(const auto& entry : colorMap)
					colorArray[entry.first] = (ColorA)entry.second;
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
						*c++ = (ColorA)it->second;
						*c++ = (ColorA)it->second;
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

	// Apply transparency values.
	if(transparencyProperty && transparencyProperty->size() * 2 == output.size()) {
		auto c = output.begin();
		for(FloatType t : transparencyProperty->constFloatRange()) {
			FloatType alpha = qBound(FloatType(0), FloatType(1)-t, FloatType(1));
			c->a() = alpha; ++c;
			c->a() = alpha; ++c;
		}
	}

	// Highlight selected bonds.
	if(bondSelectionProperty && bondSelectionProperty->size() * 2 == output.size()) {
		const ColorA selColor = (ColorA)selectionBondColor();
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
	if(const ParticlesObject* particles = pipelineState().getObject<ParticlesObject>()) {
		if(particles->bonds()) {
			const PropertyObject* topologyProperty = particles->bonds()->getTopology();
			if(topologyProperty && topologyProperty->size() > bondIndex) {
				size_t index1 = topologyProperty->getInt64Component(bondIndex, 0);
				size_t index2 = topologyProperty->getInt64Component(bondIndex, 1);
				str = tr("Bond");

				// Bond length
				const PropertyObject* posProperty = particles->getProperty(ParticlesObject::PositionProperty);
				if(posProperty && posProperty->size() > index1 && posProperty->size() > index2) {
					const Point3& p1 = posProperty->getPoint3(index1);
					const Point3& p2 = posProperty->getPoint3(index2);
					Vector3 delta = p2 - p1;
					if(const PropertyObject* periodicImageProperty = particles->bonds()->getProperty(BondsObject::PeriodicImageProperty)) {
						if(const SimulationCellObject* simCell = pipelineState().getObject<SimulationCellObject>()) {
							delta += simCell->cellMatrix() * Vector3(periodicImageProperty->getVector3I(bondIndex));
						}
					}
					str += QString(" | Length: %1 | Delta: (%2 %3 %4)").arg(delta.length()).arg(delta.x()).arg(delta.y()).arg(delta.z());
				}

				// Bond properties
				for(const PropertyObject* property : particles->bonds()->properties()) {
					if(property->size() <= bondIndex) continue;
					if(property->type() == BondsObject::SelectionProperty) continue;
					if(property->type() == BondsObject::ColorProperty) continue;
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
								if(const ElementType* btype = property->elementType(property->getIntComponent(bondIndex, component))) {
									if(!btype->name().isEmpty())
										str += QString(" (%1)").arg(btype->name());
								}
							}
						}
						else if(property->dataType() == PropertyStorage::Int64)
							str += QString::number(property->getInt64Component(bondIndex, component));
						else if(property->dataType() == PropertyStorage::Float)
							str += QString::number(property->getFloatComponent(bondIndex, component));
					}
				}

				// Pair type info.
				const PropertyObject* typeProperty = particles->getProperty(ParticlesObject::TypeProperty);
				if(typeProperty && typeProperty->size() > index1 && typeProperty->size() > index2) {
					const ElementType* type1 = typeProperty->elementType(typeProperty->getInt(index1));
					const ElementType* type2 = typeProperty->elementType(typeProperty->getInt(index2));
					if(type1 && type2) {
						str += QString(" | Particles: %1 - %2").arg(type1->nameOrNumericId(), type2->nameOrNumericId());
					}
				}
			}
		}
	}
	return str;
}

}	// End of namespace
}	// End of namespace
