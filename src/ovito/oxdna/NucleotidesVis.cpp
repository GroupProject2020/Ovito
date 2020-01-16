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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/data/VersionedDataObjectRef.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/ParticlePrimitive.h>
#include <ovito/core/rendering/ArrowPrimitive.h>
#include "NucleotidesVis.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(NucleotidesVis);

/******************************************************************************
* Constructor.
******************************************************************************/
NucleotidesVis::NucleotidesVis(DataSet* dataset) : ParticlesVis(dataset)
{
}

/******************************************************************************
* Computes the bounding box of the visual element.
******************************************************************************/
Box3 NucleotidesVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(objectStack.back());
	if(!particles) return {};
	particles->verifyIntegrity();
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	const PropertyObject* nucleotideAxisProperty = particles->getProperty(ParticlesObject::NucleotideAxisProperty);

	// The key type used for caching the computed bounding box:
	using CacheKey = std::tuple<
		VersionedDataObjectRef,	// Position property + revision number
		VersionedDataObjectRef,	// Nucleotide axis property + revision number
		FloatType 				// Default particle radius
	>;

	// Look up the bounding box in the vis cache.
	auto& bbox = dataset()->visCache().get<Box3>(CacheKey(
			positionProperty,
			nucleotideAxisProperty,
			defaultParticleRadius()));

	// Check if the cached bounding box information is still up to date.
	if(bbox.isEmpty()) {

		// If not, recompute bounding box from particle data.
		Box3 innerBox;
		if(ConstPropertyAccess<Point3> positionArray = positionProperty) {
			innerBox.addPoints(positionArray);
			if(ConstPropertyAccess<Vector3> axisArray = nucleotideAxisProperty) {
				const Vector3* axis = axisArray.cbegin();
				for(const Point3& p : positionArray) {
					innerBox.addPoint(p + (*axis++));
				}
			}
		}

		// Extend box to account for radii/shape of particles.
		FloatType maxAtomRadius = defaultParticleRadius();

		// Extend the bounding box by the largest particle radius.
		bbox = innerBox.padBox(std::max(maxAtomRadius * sqrt(FloatType(3)), FloatType(0)));
	}
	return bbox;
}

/******************************************************************************
* Determines the effective rendering colors for the backbone sites of the nucleotides.
******************************************************************************/
void NucleotidesVis::backboneColors(std::vector<ColorA>& output, const ParticlesObject* particles, bool highlightSelection) const
{
	particles->verifyIntegrity();
	OVITO_ASSERT(particles->elementCount() == output.size());

	const PropertyObject* colorProperty = particles->getProperty(ParticlesObject::ColorProperty);
	const PropertyObject* strandProperty = particles->getProperty(ParticlesObject::DNAStrandProperty);
	const PropertyObject* selectionProperty = highlightSelection ? particles->getProperty(ParticlesObject::SelectionProperty) : nullptr;
	const PropertyObject* transparencyProperty = particles->getProperty(ParticlesObject::TransparencyProperty);

	ColorA defaultColor = defaultParticleColor();
	if(colorProperty) {
		// Take particle colors directly from the color property.
		boost::copy(ConstPropertyAccess<Color>(colorProperty), output.begin());
	}
	else if(strandProperty) {
		// Assign colors based on strand.
		// Generate a lookup map for strand colors.
		const std::map<int,Color> colorMap = strandProperty->typeColorMap();
		std::array<ColorA,32> colorArray;
		// Check if all type IDs are within a small, non-negative range.
		// If yes, we can use an array lookup strategy. Otherwise we have to use a dictionary lookup strategy, which is slower.
		if(std::all_of(colorMap.begin(), colorMap.end(), [&colorArray](const auto& i) { return i.first >= 0 && i.first < (int)colorArray.size(); })) {
			colorArray.fill(defaultColor);
			for(const auto& entry : colorMap)
				colorArray[entry.first] = entry.second;
			// Fill color array.
			ConstPropertyAccess<int> typeArray(strandProperty);
			const int* t = typeArray.cbegin();
			for(auto c = output.begin(); c != output.end(); ++c, ++t) {
				if(*t >= 0 && *t < (int)colorArray.size())
					*c = colorArray[*t];
				else
					*c = defaultColor;
			}
		}
		else {
			// Fill color array.
			ConstPropertyAccess<int> typeArray(strandProperty);
			const int* t = typeArray.cbegin();
			for(auto c = output.begin(); c != output.end(); ++c, ++t) {
				auto it = colorMap.find(*t);
				if(it != colorMap.end())
					*c = it->second;
				else
					*c = defaultColor;
			}
		}
	}
	else {
		// Assign a uniform color to all nucleotides.
		boost::fill(output, defaultColor);
	}

	// Set color alpha values based on transparency property.
	if(transparencyProperty) {
		ConstPropertyAccess<FloatType> transparencyArray(transparencyProperty);
		const FloatType* t = transparencyArray.cbegin();
		for(ColorA& c : output) {
			c.a() = qBound(FloatType(0), FloatType(1) - (*t++), FloatType(1));
		}
	}

	// Highlight selected particles.
	if(selectionProperty) {
		const Color selColor = selectionParticleColor();
		ConstPropertyAccess<int> selectionArray(selectionProperty);
		const int* t = selectionArray.cbegin();
		for(auto c = output.begin(); c != output.end(); ++c, ++t) {
			if(*t)
				*c = selColor;
		}
	}
}

/******************************************************************************
* Determines the effective rendering colors for the base sites of the nucleotides.
******************************************************************************/
void NucleotidesVis::nucleobaseColors(std::vector<ColorA>& output, const ParticlesObject* particles, bool highlightSelection) const
{
	particles->verifyIntegrity();
	OVITO_ASSERT(particles->elementCount() == output.size());

	const PropertyObject* baseProperty = particles->getProperty(ParticlesObject::NucleobaseTypeProperty);
	const PropertyObject* selectionProperty = highlightSelection ? particles->getProperty(ParticlesObject::SelectionProperty) : nullptr;
	const PropertyObject* transparencyProperty = particles->getProperty(ParticlesObject::TransparencyProperty);

	ColorA defaultColor = defaultParticleColor();
	if(baseProperty) {
		// Assign colors based on base type.
		// Generate a lookup map for base type colors.
		const std::map<int,Color> colorMap = baseProperty->typeColorMap();
		std::array<ColorA,16> colorArray;
		// Check if all type IDs are within a small, non-negative range.
		// If yes, we can use an array lookup strategy. Otherwise we have to use a dictionary lookup strategy, which is slower.
		if(std::all_of(colorMap.begin(), colorMap.end(), [&colorArray](const auto& i) { return i.first >= 0 && i.first < (int)colorArray.size(); })) {
			colorArray.fill(defaultColor);
			for(const auto& entry : colorMap)
				colorArray[entry.first] = entry.second;
			// Fill color array.
			ConstPropertyAccess<int> typeArray(baseProperty);
			const int* t = typeArray.cbegin();
			for(auto c = output.begin(); c != output.end(); ++c, ++t) {
				if(*t >= 0 && *t < (int)colorArray.size())
					*c = colorArray[*t];
				else
					*c = defaultColor;
			}
		}
		else {
			// Fill color array.
			ConstPropertyAccess<int> typeArray(baseProperty);
			const int* t = typeArray.cbegin();
			for(auto c = output.begin(); c != output.end(); ++c, ++t) {
				auto it = colorMap.find(*t);
				if(it != colorMap.end())
					*c = it->second;
				else
					*c = defaultColor;
			}
		}
	}
	else {
		// Assign a uniform color to all base sites.
		boost::fill(output, defaultColor);
	}

	// Set color alpha values based on transparency property.
	if(transparencyProperty) {
		ConstPropertyAccess<FloatType> transparencyArray(transparencyProperty);
		const FloatType* t = transparencyArray.cbegin();
		for(ColorA& c : output) {
			c.a() = qBound(FloatType(0), FloatType(1) - (*t++), FloatType(1));
		}
	}

	// Highlight selected sites.
	if(selectionProperty) {
		const Color selColor = selectionParticleColor();
		ConstPropertyAccess<int> selectionArray(selectionProperty);
		const int* t = selectionArray.cbegin();
		for(auto c = output.begin(); c != output.end(); ++c, ++t) {
			if(*t)
				*c = selColor;
		}
	}
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
void NucleotidesVis::render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, objectStack, contextNode, flowState, validityInterval));
		return;
	}

	// Get input data.
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(objectStack.back());
	if(!particles) return;
	particles->verifyIntegrity();
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	if(!positionProperty) return;
	const PropertyObject* colorProperty = particles->getProperty(ParticlesObject::ColorProperty);
	const PropertyObject* baseProperty = particles->getProperty(ParticlesObject::NucleobaseTypeProperty);
	const PropertyObject* strandProperty = particles->getProperty(ParticlesObject::DNAStrandProperty);
	const PropertyObject* selectionProperty = renderer->isInteractive() ? particles->getProperty(ParticlesObject::SelectionProperty) : nullptr;
	const PropertyObject* transparencyProperty = particles->getProperty(ParticlesObject::TransparencyProperty);
	const PropertyObject* nucleotideAxisProperty = particles->getProperty(ParticlesObject::NucleotideAxisProperty);
	const PropertyObject* nucleotideNormalProperty = particles->getProperty(ParticlesObject::NucleotideNormalProperty);

	// Make sure we don't exceed our internal limits.
	if(particles->elementCount() > (size_t)std::numeric_limits<int>::max()) {
		qWarning() << "WARNING: Cannot render more than" << std::numeric_limits<int>::max() << "nucleotides.";
		return;
	}

	// The type of lookup key used for caching the rendering primitives:
	using NucleotidesCacheKey = std::tuple<
		CompatibleRendererGroup,	// The scene renderer
		QPointer<PipelineSceneNode>,// The scene node
		VersionedDataObjectRef,		// Position property + revision number
		VersionedDataObjectRef,		// Color property + revision number
		VersionedDataObjectRef,		// Strand property + revision number
		VersionedDataObjectRef,		// Transparency property + revision number
		VersionedDataObjectRef,		// Selection property + revision number
		VersionedDataObjectRef,		// Nucleotide axis property + revision number
		VersionedDataObjectRef,		// Nucleotide normal property + revision number
		FloatType					// Default particle radius
	>;
	
	// The data structure stored in the vis cache.
	struct NucleotidesCacheValue {
		std::shared_ptr<ParticlePrimitive> backbonePrimitive;
		std::shared_ptr<ArrowPrimitive> connectionPrimitive;
		std::shared_ptr<ParticlePrimitive> basePrimitive;
		OORef<ParticlePickInfo> pickInfo;
	};

	// Look up the rendering primitives in the vis cache.
	auto& visCache = dataset()->visCache().get<NucleotidesCacheValue>(NucleotidesCacheKey(
		renderer,
		const_cast<PipelineSceneNode*>(contextNode),
		positionProperty,
		colorProperty,
		strandProperty,
		transparencyProperty,
		selectionProperty,
		nucleotideAxisProperty,
		nucleotideNormalProperty,
		defaultParticleRadius()));

	// Check if we already have valid rendering primitives that are up to date.
	if(!visCache.backbonePrimitive
			|| !visCache.backbonePrimitive->isValid(renderer)
			|| (visCache.connectionPrimitive && !visCache.connectionPrimitive->isValid(renderer))
			|| (visCache.basePrimitive && !visCache.basePrimitive->isValid(renderer))
			|| (transparencyProperty != nullptr) != visCache.backbonePrimitive->translucentParticles()) {

		// Create the rendering primitive for the backbone sites.
		visCache.backbonePrimitive = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::MediumQuality, ParticlePrimitive::SphericalShape, transparencyProperty != nullptr);
		visCache.backbonePrimitive->setSize(particles->elementCount());

		// Fill in the position data.
		visCache.backbonePrimitive->setParticlePositions(ConstPropertyAccess<Point3>(positionProperty).cbegin());

		// Compute the effective color of each particle.
		std::vector<ColorA> colors(particles->elementCount());
		backboneColors(colors, particles, renderer->isInteractive());

		// Fill in backbone color data.
		visCache.backbonePrimitive->setParticleColors(colors.data());

		// Assign a uniform radius to all particles.
		visCache.backbonePrimitive->setParticleRadius(defaultParticleRadius());

		if(nucleotideAxisProperty) {
			// Create the rendering primitive for the base sites.
			visCache.basePrimitive = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::MediumQuality, ParticlePrimitive::EllipsoidShape, transparencyProperty != nullptr);
			visCache.basePrimitive->setSize(particles->elementCount());

			// Fill in the position data for the base sites.
			std::vector<Point3> baseSites(particles->elementCount());
			ConstPropertyAccess<Point3> positionsArray(positionProperty);
			ConstPropertyAccess<Vector3> nucleotideAxisArray(nucleotideAxisProperty);
			for(size_t i = 0; i < baseSites.size(); i++)
				baseSites[i] = positionsArray[i] + (0.8 * nucleotideAxisArray[i]);
			visCache.basePrimitive->setParticlePositions(baseSites.data());

			// Fill in base color data.
			std::vector<ColorA> baseColors(particles->elementCount());
			nucleobaseColors(baseColors, particles, renderer->isInteractive());
			visCache.basePrimitive->setParticleColors(baseColors.data());

			// Fill in aspherical shape values.
			Vector3 asphericalShape(defaultParticleRadius(), defaultParticleRadius(), 0.5 * defaultParticleRadius());
			std::vector<Vector3> asphericalShapes(particles->elementCount(), asphericalShape);
			visCache.basePrimitive->setParticleShapes(asphericalShapes.data());

			// Fill in base orientations.
			if(ConstPropertyAccess<Vector3> nucleotideNormalArray = nucleotideNormalProperty) {
				std::vector<Quaternion> orientations(particles->elementCount());
				for(size_t i = 0; i < orientations.size(); i++) {
					Vector3 an = nucleotideNormalArray[i].safelyNormalized();
					if(an.z() > 1.0 - FLOATTYPE_EPSILON || an.z() < -1.0 + FLOATTYPE_EPSILON || an == Vector3::Zero())
						orientations[i] = Quaternion::Identity();
					else
						orientations[i] = Quaternion(Rotation(Vector3(0,0,1).cross(an), std::acos(an.z())));
				}
				visCache.basePrimitive->setParticleOrientations(orientations.data());	
			}

			// Create the rendering primitive for the connections between backbone and base sites.
			visCache.connectionPrimitive = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, ArrowPrimitive::NormalShading, ArrowPrimitive::HighQuality, transparencyProperty != nullptr);
			visCache.connectionPrimitive->startSetElements(particles->elementCount());
			FloatType cylinderRadius = 0.4 * defaultParticleRadius();
			for(size_t i = 0; i < positionsArray.size(); i++)
				visCache.connectionPrimitive->setElement(i, positionsArray[i], 0.8 * nucleotideAxisArray[i], colors[i], cylinderRadius);
			visCache.connectionPrimitive->endSetElements();
		}
		else {
			visCache.connectionPrimitive.reset();
			visCache.basePrimitive.reset();
		}

		// Create pick info record.
		std::vector<size_t> subobjectToParticleMapping(nucleotideAxisProperty ? (particles->elementCount() * 3) : particles->elementCount());
		std::iota(subobjectToParticleMapping.begin(), subobjectToParticleMapping.begin() + particles->elementCount(), (size_t)0);
		if(nucleotideAxisProperty) {
			std::iota(subobjectToParticleMapping.begin() +   particles->elementCount(), subobjectToParticleMapping.begin() + 2*particles->elementCount(), (size_t)0);
			std::iota(subobjectToParticleMapping.begin() + 2*particles->elementCount(), subobjectToParticleMapping.begin() + 3*particles->elementCount(), (size_t)0);
		}
		visCache.pickInfo = new ParticlePickInfo(this, flowState, std::move(subobjectToParticleMapping));
	}
	else {
		// Update the pipeline state stored in te picking object info.
		visCache.pickInfo->setPipelineState(flowState);
	}

	if(renderer->isPicking())
		renderer->beginPickObject(contextNode, visCache.pickInfo);

	visCache.backbonePrimitive->render(renderer);
	if(visCache.connectionPrimitive)
		visCache.connectionPrimitive->render(renderer);
	if(visCache.basePrimitive)
		visCache.basePrimitive->render(renderer);

	if(renderer->isPicking())
		renderer->endPickObject();
}

}	// End of namespace
}	// End of namespace
