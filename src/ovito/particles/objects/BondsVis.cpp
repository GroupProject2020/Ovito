////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/data/VersionedDataObjectRef.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/ArrowPrimitive.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
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

			ConstPropertyAccess<ParticleIndexPair> bondTopology(bondTopologyProperty);
			ConstPropertyAccess<Vector3I> bondPeriodicImages(bondPeriodicImageProperty);
			ConstPropertyAccess<Point3> positions(positionProperty);

			size_t particleCount = positions.size();
			const AffineTransformation cell = simulationCell ? simulationCell->cellMatrix() : AffineTransformation::Zero();

			for(size_t bondIndex = 0; bondIndex < bondTopology.size(); bondIndex++) {
				size_t index1 = bondTopology[bondIndex][0];
				size_t index2 = bondTopology[bondIndex][1];
				if(index1 >= particleCount || index2 >= particleCount)
					continue;

				bbox.addPoint(positions[index1]);
				bbox.addPoint(positions[index2]);
				if(bondPeriodicImages && bondPeriodicImages[bondIndex] != Vector3I::Zero()) {
					Vector3 vec = positions[index2] - positions[index1];
					const Vector3I& pbcShift = bondPeriodicImages[bondIndex];
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
	std::cout << "RENDER BOND VIS\n";
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
	const PropertyObject* bondTypeProperty = bonds->getProperty(BondsObject::TypeProperty);
	const PropertyObject* bondColorProperty = bonds->getProperty(BondsObject::ColorProperty);
	const PropertyObject* bondSelectionProperty = renderer->isInteractive() ? bonds->getProperty(BondsObject::SelectionProperty) : nullptr;
	const PropertyObject* transparencyProperty = bonds->getProperty(BondsObject::TransparencyProperty);

	// Obtain particle-related properties and the vis element.
	const ParticlesVis* particleVis = particles->visElement<ParticlesVis>();
	const PropertyObject* particleRadiusProperty = particles->getProperty(ParticlesObject::RadiusProperty);
	const PropertyObject* particleColorProperty = nullptr;
	const PropertyObject* particleTypeProperty = nullptr;
	if(useParticleColors() && particleVis) {
		particleColorProperty = particles->getProperty(ParticlesObject::ColorProperty);
		particleTypeProperty = particleVis->getParticleTypeColorProperty(particles);
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
		VersionedDataObjectRef,		// Particle radius property + revision number
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
			particleRadiusProperty,
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

			// Cache some values.
			ConstPropertyAccess<Point3> positions(positionProperty);
			size_t particleCount = positions.size();
			const AffineTransformation cell = simulationCell ? simulationCell->cellMatrix() : AffineTransformation::Zero();

			// Compute the radii of the particles.
			std::vector<FloatType> particleRadii;
			if(particleVis)
				particleRadii = particleVis->particleRadii(particles);

			if(!useParticleColors())
				particleVis = nullptr;

			// Determine half-bond colors.
			std::vector<ColorA> colors = halfBondColors(particles, renderer->isInteractive(), useParticleColors(), false);
			OVITO_ASSERT(colors.size() == arrowPrimitive->elementCount());

			// Make sure the particle radius array has the correct length.
			if(particleRadii.size() != particleCount) particleRadii.clear();

			int elementIndex = 0;
			auto color = colors.cbegin();
			ConstPropertyAccess<ParticleIndexPair> bonds(bondTopologyProperty);
			ConstPropertyAccess<Vector3I> bondPeriodicImages(bondPeriodicImageProperty);
			for(size_t bondIndex = 0; bondIndex < bonds.size(); bondIndex++) {
				size_t particleIndex1 = bonds[bondIndex][0];
				size_t particleIndex2 = bonds[bondIndex][1];
				if(particleIndex1 < particleCount && particleIndex2 < particleCount) {
					Vector3 vec = positions[particleIndex2] - positions[particleIndex1];
					if(bondPeriodicImageProperty) {
						for(size_t k = 0; k < 3; k++)
							if(int d = bondPeriodicImages[bondIndex][k]) vec += cell.column(k) * (FloatType)d;
					}
					FloatType t = 0.5;
					FloatType blen = vec.length() * FloatType(2);
					if(!particleRadii.empty() && blen != 0) {
						// This calculation determines the point where to split the bond into the two half-bonds
						// such that the border appears halfway between the two particles, which may have two different sizes.
						t = FloatType(0.5) + std::min(FloatType(0.5), particleRadii[particleIndex1]/blen) - std::min(FloatType(0.5), particleRadii[particleIndex2]/blen);
					}
					arrowPrimitive->setElement(elementIndex++, positions[particleIndex1], vec * t, *color++, bondRadius);
					arrowPrimitive->setElement(elementIndex++, positions[particleIndex2], vec * (t-FloatType(1)), *color++, bondRadius);
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
std::vector<ColorA> BondsVis::halfBondColors(const ParticlesObject* particles, bool highlightSelection, bool useParticleColors, bool ignoreBondColorProperty) const
{
	OVITO_ASSERT(particles != nullptr);
	particles->verifyIntegrity();
	const BondsObject* bonds = particles->bonds();
	if(!bonds) return {};
	bonds->verifyIntegrity();

	// Get bond-related properties which determine the bond coloring.
	ConstPropertyAccess<ParticleIndexPair> topologyProperty = bonds->getProperty(BondsObject::TopologyProperty);
	ConstPropertyAccess<Color> bondColorProperty = !ignoreBondColorProperty ? bonds->getProperty(BondsObject::ColorProperty) : nullptr;
	const PropertyObject* bondTypeProperty = bonds->getProperty(BondsObject::TypeProperty);
	ConstPropertyAccess<int> bondSelectionProperty = highlightSelection ? bonds->getProperty(BondsObject::SelectionProperty) : nullptr;
	ConstPropertyAccess<FloatType> transparencyProperty = bonds->getProperty(BondsObject::TransparencyProperty);

	// Get particle-related properties and the vis element.
	const ParticlesVis* particleVis = particles->visElement<ParticlesVis>();
	ConstPropertyAccess<Color> particleColorProperty;
	const PropertyObject* particleTypeProperty = nullptr;
	if(useParticleColors && particleVis) {
		particleColorProperty = particles->getProperty(ParticlesObject::ColorProperty);
		particleTypeProperty = particleVis->getParticleTypeColorProperty(particles);
	}

	std::vector<ColorA> output(bonds->elementCount() * 2);
	ColorA defaultColor = (ColorA)bondColor();
	if(bondColorProperty && bondColorProperty.size() * 2 == output.size()) {
		// Take bond colors directly from the color property.
		auto bc = output.begin();
		for(const Color& c : bondColorProperty) {
			*bc++ = c;
			*bc++ = c;
		}
	}
	else if(useParticleColors && particleVis) {
		// Derive bond colors from particle colors.
		size_t particleCount = particles->elementCount();
		std::vector<ColorA> particleColors = particleVis->particleColors(particles, false, false);
		auto bc = output.begin();
		for(const auto& bond : topologyProperty) {
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
			if(boost::algorithm::all_of(colorMap,
					[&colorArray](const std::map<int, ColorA>::value_type& i) { return i.first >= 0 && i.first < (int)colorArray.size(); })) {
				colorArray.fill(defaultColor);
				for(const auto& entry : colorMap)
					colorArray[entry.first] = entry.second;
				// Fill color array.
				ConstPropertyAccess<int> bondTypeData(bondTypeProperty);
				const int* t = bondTypeData.cbegin();
				for(auto c = output.begin(); c != output.end(); ++t) {
					if(*t >= 0 && *t < (int)colorArray.size()) {
						//Modif
						colorArray[*t].a() = 0.5;
						*c++ = colorArray[*t];
						*c++ = colorArray[*t];
					}
					else {
						//Modif
						defaultColor.a() = 0.5;
						*c++ = defaultColor;
						*c++ = defaultColor;
					}
				}
			}
			else {
				// Fill color array.
				ConstPropertyAccess<int> bondTypeData(bondTypeProperty);
				const int* t = bondTypeData.cbegin();
				for(auto c = output.begin(); c != output.end(); ++t) {
					auto it = colorMap.find(*t);
					if(it != colorMap.end()) {
						//Modif
						//
						*c++ = it->second;
						*c++ = it->second;
					}
					else {
						defaultColor.a() = 0.5;
						*c++ = defaultColor;
						*c++ = defaultColor;
					}
				}
			}
		}
		else {
			// Assign a uniform color to all bonds.
			defaultColor.a() = 0.5;
			boost::fill(output, defaultColor);
		}
	}

	// Apply transparency values.
	// TRY TO MODIFY THIS

	for(auto c=output.begin(); c!=output.end(); ++c){
		c->a() = 0.5;
	}
	
	/*if(transparencyProperty && transparencyProperty.size() * 2 == output.size()) {
		auto c = output.begin();
		for(FloatType t : transparencyProperty) {
			//Modif
			//FloatType alpha = qBound(FloatType(0), FloatType(1)-t, FloatType(1));
			FloatType alpha = 0.5;
			c->a() = alpha; ++c;
			c->a() = alpha; ++c;
		}
	}
*/
	// Highlight selected bonds.
/*	if(bondSelectionProperty && bondSelectionProperty.size() * 2 == output.size()) {
		const ColorA selColor = selectionBondColor();
		const int* t = bondSelectionProperty.cbegin();
		for(auto c = output.begin(); c != output.end(); ++t) {
			if(*t) {
				*c++ = selColor;
				*c++ = selColor;
			}
			else c += 2;
		}
	}
*/
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
			ConstPropertyAccess<ParticleIndexPair> topologyProperty = particles->bonds()->getTopology();
			if(topologyProperty && topologyProperty.size() > bondIndex) {
				size_t index1 = topologyProperty[bondIndex][0];
				size_t index2 = topologyProperty[bondIndex][1];
				str = tr("Bond");

				// Bond length
				ConstPropertyAccess<Point3> posProperty = particles->getProperty(ParticlesObject::PositionProperty);
				if(posProperty && posProperty.size() > index1 && posProperty.size() > index2) {
					const Point3& p1 = posProperty[index1];
					const Point3& p2 = posProperty[index2];
					Vector3 delta = p2 - p1;
					if(ConstPropertyAccess<Vector3I> periodicImageProperty = particles->bonds()->getProperty(BondsObject::PeriodicImageProperty)) {
						if(const SimulationCellObject* simCell = pipelineState().getObject<SimulationCellObject>()) {
							delta += simCell->cellMatrix() * Vector3(periodicImageProperty[bondIndex]);
						}
					}
					str += QString(" | Length: %1 | Delta: (%2 %3 %4)").arg(delta.length()).arg(delta.x()).arg(delta.y()).arg(delta.z());
				}

				// Bond properties
				for(const PropertyObject* property : particles->bonds()->properties()) {
					if(property->size() <= bondIndex) continue;
					if(property->type() == BondsObject::SelectionProperty) continue;
					if(property->type() == BondsObject::ColorProperty) continue;
					if(!str.isEmpty()) str += QStringLiteral(" | ");
					str += property->name();
					str += QStringLiteral(" ");
					if(property->dataType() == PropertyStorage::Int) {
						ConstPropertyAccess<int, true> data(property);
						for(size_t component = 0; component < data.componentCount(); component++) {
							if(component != 0) str += QStringLiteral(", ");
							str += QString::number(data.get(bondIndex, component));
							if(property->elementTypes().empty() == false) {
								if(const ElementType* ptype = property->elementType(data.get(bondIndex, component))) {
									if(!ptype->name().isEmpty())
										str += QString(" (%1)").arg(ptype->name());
								}
							}
						}
					}
					else if(property->dataType() == PropertyStorage::Int64) {
						ConstPropertyAccess<qlonglong, true> data(property);
						for(size_t component = 0; component < property->componentCount(); component++) {
							if(component != 0) str += QStringLiteral(", ");
							str += QString::number(data.get(bondIndex, component));
						}
					}
					else if(property->dataType() == PropertyStorage::Float) {
						ConstPropertyAccess<FloatType, true> data(property);
						for(size_t component = 0; component < property->componentCount(); component++) {
							if(component != 0) str += QStringLiteral(", ");
							str += QString::number(data.get(bondIndex, component));
						}
					}
					else {
						str += QStringLiteral("<%1>").arg(QMetaType::typeName(property->dataType()) ? QMetaType::typeName(property->dataType()) : "unknown");
					}
				}

				// Pair type info.
				const PropertyObject* typeProperty = particles->getProperty(ParticlesObject::TypeProperty);
				if(typeProperty && typeProperty->size() > index1 && typeProperty->size() > index2) {
					ConstPropertyAccess<int> typeData(typeProperty);
					const ElementType* type1 = typeProperty->elementType(typeData[index1]);
					const ElementType* type2 = typeProperty->elementType(typeData[index2]);
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
