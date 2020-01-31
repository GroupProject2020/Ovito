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
#include <ovito/particles/objects/ParticleType.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/data/VersionedDataObjectRef.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/ParticlePrimitive.h>
#include <ovito/core/rendering/ArrowPrimitive.h>
#include "ParticlesVis.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(ParticlesVis);
IMPLEMENT_OVITO_CLASS(ParticlePickInfo);
DEFINE_PROPERTY_FIELD(ParticlesVis, defaultParticleRadius);
DEFINE_PROPERTY_FIELD(ParticlesVis, renderingQuality);
DEFINE_PROPERTY_FIELD(ParticlesVis, particleShape);
SET_PROPERTY_FIELD_LABEL(ParticlesVis, defaultParticleRadius, "Default particle radius");
SET_PROPERTY_FIELD_LABEL(ParticlesVis, renderingQuality, "Rendering quality");
SET_PROPERTY_FIELD_LABEL(ParticlesVis, particleShape, "Shape");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ParticlesVis, defaultParticleRadius, WorldParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
ParticlesVis::ParticlesVis(DataSet* dataset) : DataVis(dataset),
	_defaultParticleRadius(1.2),
	_renderingQuality(ParticlePrimitive::AutoQuality),
	_particleShape(Sphere)
{
}

/******************************************************************************
* Computes the bounding box of the visual element.
******************************************************************************/
Box3 ParticlesVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(objectStack.back());
	if(!particles) return {};
	particles->verifyIntegrity();
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	const PropertyObject* radiusProperty = particles->getProperty(ParticlesObject::RadiusProperty);
	const PropertyObject* typeProperty = particles->getProperty(ParticlesObject::TypeProperty);
	const PropertyObject* shapeProperty = particles->getProperty(ParticlesObject::AsphericalShapeProperty);

	// The key type used for caching the computed bounding box:
	using CacheKey = std::tuple<
		VersionedDataObjectRef,	// Position property + revision number
		VersionedDataObjectRef,	// Radius property + revision number
		VersionedDataObjectRef,	// Type property + revision number
		VersionedDataObjectRef,	// Aspherical shape property + revision number
		FloatType 				// Default particle radius
	>;

	// Look up the bounding box in the vis cache.
	auto& bbox = dataset()->visCache().get<Box3>(CacheKey(
			positionProperty,
			radiusProperty,
			typeProperty,
			shapeProperty,
			defaultParticleRadius()));

	// Check if the cached bounding box information is still up to date.
	if(bbox.isEmpty()) {
		// If not, recompute bounding box from particle data.
		bbox = particleBoundingBox(positionProperty, typeProperty, radiusProperty, shapeProperty, true);
	}
	return bbox;
}

/******************************************************************************
* Computes the bounding box of the particles.
******************************************************************************/
Box3 ParticlesVis::particleBoundingBox(ConstPropertyAccess<Point3> positionProperty, const PropertyObject* typeProperty, ConstPropertyAccess<FloatType> radiusProperty, ConstPropertyAccess<Vector3> shapeProperty, bool includeParticleRadius) const
{
	OVITO_ASSERT(typeProperty == nullptr || typeProperty->type() == ParticlesObject::TypeProperty);
	if(particleShape() != Sphere && particleShape() != Box && particleShape() != Cylinder && particleShape() != Spherocylinder)
		shapeProperty = nullptr;

	Box3 bbox;
	if(positionProperty) {
		bbox.addPoints(positionProperty);
	}
	if(!includeParticleRadius)
		return bbox;

	// Check if any of the particle types have a user-defined mesh geometry assigned.
	std::vector<std::pair<int,FloatType>> userShapeParticleTypes;
	if(typeProperty) {
		for(const ElementType* etype : typeProperty->elementTypes()) {
			if(const ParticleType* ptype = dynamic_object_cast<ParticleType>(etype)) {
				if(ptype->shapeMesh() && ptype->shapeMesh()->mesh() && ptype->shapeMesh()->mesh()->faceCount() != 0) {
					// Compute the maximum extent of the user-defined shape mesh.
					const Box3& bbox = ptype->shapeMesh()->mesh()->boundingBox();
					FloatType extent = std::max((bbox.minc - Point3::Origin()).length(), (bbox.maxc - Point3::Origin()).length());
					userShapeParticleTypes.emplace_back(ptype->numericId(), extent);
				}
			}
		}
	}

	// Extend box to account for radii/shape of particles.
	FloatType maxAtomRadius = 0;

	if(userShapeParticleTypes.empty()) {
		// Standard case - no user-defined particle shapes assigned:
		if(typeProperty) {
			for(const auto& it : ParticleType::typeRadiusMap(typeProperty)) {
				maxAtomRadius = std::max(maxAtomRadius, (it.second != 0) ? it.second : defaultParticleRadius());
			}
		}
		if(maxAtomRadius == 0)
			maxAtomRadius = defaultParticleRadius();
		if(shapeProperty) {
			for(const Vector3& s : shapeProperty)
				maxAtomRadius = std::max(maxAtomRadius, std::max(s.x(), std::max(s.y(), s.z())));
			if(particleShape() == Spherocylinder)
				maxAtomRadius *= 2;
		}
		if(radiusProperty && radiusProperty.size() != 0) {
			auto minmax = std::minmax_element(radiusProperty.cbegin(), radiusProperty.cend());
			if(*minmax.first <= 0)
				maxAtomRadius = std::max(maxAtomRadius, *minmax.second);
			else
				maxAtomRadius = *minmax.second;
		}
	}
	else {
		// Non-standard case - at least one user-defined particle shape assigned:
		std::map<int,FloatType> typeRadiusMap = ParticleType::typeRadiusMap(typeProperty);
		if(radiusProperty && radiusProperty.size() == typeProperty->size()) {
			const FloatType* r = radiusProperty.cbegin();
			ConstPropertyAccess<int> typeData(typeProperty);
			for(int t : typeData) {
				// Determine effective radius of the current particle.
				FloatType radius = *r++;
				if(radius <= 0) radius = typeRadiusMap[t];
				if(radius <= 0) radius = defaultParticleRadius();
				// Effective radius is multiplied with the extent of the user-defined shape mesh.
				bool foundMeshExtent = false;
				for(const auto& entry : userShapeParticleTypes) {
					if(entry.first == t) {
						maxAtomRadius = std::max(maxAtomRadius, radius * entry.second);
						foundMeshExtent = true;
						break;
					}
				}
				// If this particle type has no user-defined shape assigned, simply use radius.
				if(!foundMeshExtent)
					maxAtomRadius = std::max(maxAtomRadius, radius);
			}
		}
		else {
			for(const auto& it : typeRadiusMap) {
				FloatType typeRadius = (it.second != 0) ? it.second : defaultParticleRadius();
				bool foundMeshExtent = false;
				for(const auto& entry : userShapeParticleTypes) {
					if(entry.first == it.first) {
						maxAtomRadius = std::max(maxAtomRadius, typeRadius * entry.second);
						foundMeshExtent = true;
						break;
					}
				}
				// If this particle type has no user-defined shape assigned, simply use radius.
				if(!foundMeshExtent)
					maxAtomRadius = std::max(maxAtomRadius, typeRadius);
			}
		}
	}

	// Extend the bounding box by the largest particle radius.
	return bbox.padBox(std::max(maxAtomRadius * sqrt(FloatType(3)), FloatType(0)));
}

/******************************************************************************
* Returns the typed particle property used to determine the rendering colors 
* of particles (if no per-particle colors are defined).
******************************************************************************/
const PropertyObject* ParticlesVis::getParticleTypeColorProperty(const ParticlesObject* particles) const
{
	return particles->getProperty(ParticlesObject::TypeProperty);
}

/******************************************************************************
* Determines the display particle colors.
******************************************************************************/
std::vector<ColorA> ParticlesVis::particleColors(const ParticlesObject* particles, bool highlightSelection, bool includeTransparency) const
{
	OVITO_ASSERT(particles);
	particles->verifyIntegrity();

	// Get all relevant particle properties which determine the particle rendering color.
	ConstPropertyAccess<Color> colorProperty = particles->getProperty(ParticlesObject::ColorProperty);
	const PropertyObject* typeProperty = getParticleTypeColorProperty(particles);
	ConstPropertyAccess<int> selectionProperty = highlightSelection ? particles->getProperty(ParticlesObject::SelectionProperty) : nullptr;
	ConstPropertyAccess<FloatType> transparencyProperty = includeTransparency ? particles->getProperty(ParticlesObject::TransparencyProperty) : nullptr;

	// Allocate output array.
	std::vector<ColorA> output(particles->elementCount());

	ColorA defaultColor = defaultParticleColor();
	if(colorProperty && colorProperty.size() == output.size()) {
		// Take particle colors directly from the color property.
		boost::copy(colorProperty, output.begin());
	}
	else if(typeProperty && typeProperty->size() == output.size()) {
		// Assign colors based on particle types.
		// Generate a lookup map for particle type colors.
		const std::map<int,Color> colorMap = typeProperty->typeColorMap();
		std::array<ColorA,16> colorArray;
		// Check if all type IDs are within a small, non-negative range.
		// If yes, we can use an array lookup strategy. Otherwise we have to use a dictionary lookup strategy, which is slower.
		if(std::all_of(colorMap.begin(), colorMap.end(),
				[&colorArray](const std::map<int,ColorA>::value_type& i) { return i.first >= 0 && i.first < (int)colorArray.size(); })) {
			colorArray.fill(defaultColor);
			for(const auto& entry : colorMap)
				colorArray[entry.first] = entry.second;
			// Fill color array.
			ConstPropertyAccess<int> typeData(typeProperty);
			const int* t = typeData.cbegin();
			for(auto c = output.begin(); c != output.end(); ++c, ++t) {
				if(*t >= 0 && *t < (int)colorArray.size())
					*c = colorArray[*t];
				else
					*c = defaultColor;
			}
		}
		else {
			// Fill color array.
			ConstPropertyAccess<int> typeData(typeProperty);
			const int* t = typeData.cbegin();
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
		// Assign a uniform color to all particles.
		boost::fill(output, defaultColor);
	}

	// Set color alpha values based on transparency particle property.
	if(transparencyProperty && transparencyProperty.size() == output.size()) {
		const FloatType* t = transparencyProperty.cbegin();
		for(ColorA& c : output) {
			c.a() = qBound(FloatType(0), FloatType(1) - (*t++), FloatType(1));
		}
	}

	// Highlight selected particles.
	if(selectionProperty && selectionProperty.size() == output.size()) {
		const Color selColor = selectionParticleColor();
		const int* t = selectionProperty.cbegin();
		for(auto c = output.begin(); c != output.end(); ++c, ++t) {
			if(*t)
				*c = selColor;
		}
	}

	return output;
}

/******************************************************************************
* Returns the typed particle property used to determine the rendering radii 
* of particles (if no per-particle radii are defined).
******************************************************************************/
const PropertyObject* ParticlesVis::getParticleTypeRadiusProperty(const ParticlesObject* particles) const
{
	return particles->getProperty(ParticlesObject::TypeProperty);
}

/******************************************************************************
* Determines the display particle radii.
******************************************************************************/
std::vector<FloatType> ParticlesVis::particleRadii(const ParticlesObject* particles) const
{
	particles->verifyIntegrity();

	// Get particle properties that determine the rendering size of particles.
	ConstPropertyAccess<FloatType> radiusProperty = particles->getProperty(ParticlesObject::RadiusProperty);
	const PropertyObject* typeProperty = getParticleTypeRadiusProperty(particles);

	// Allocate output array.
	std::vector<FloatType> output(particles->elementCount());

	FloatType defaultRadius = defaultParticleRadius();
	if(radiusProperty) {
		// Take particle radii directly from the radius property.
		boost::transform(radiusProperty, output.begin(), [defaultRadius](FloatType r) { return r > 0 ? r : defaultRadius; } );
	}
	else if(typeProperty) {
		// Assign radii based on particle types.
		// Build a lookup map for particle type radii.
		const std::map<int,FloatType> radiusMap = ParticleType::typeRadiusMap(typeProperty);
		// Skip the following loop if all per-type radii are zero. In this case, simply use the default radius for all particles.
		if(boost::algorithm::any_of(radiusMap, [](const std::pair<int,FloatType>& it) { return it.second != 0; })) {
			// Fill radius array.
			ConstPropertyAccess<int> typeData(typeProperty);
			boost::transform(typeData, output.begin(), [&](int t) {
				auto it = radiusMap.find(t);
				// Set particle radius only if the type's radius is non-zero.
				if(it != radiusMap.end() && it->second != 0)
					return it->second;
				else
					return defaultRadius;
			});
		}
		else {
			// Assign a uniform radius to all particles.
			boost::fill(output, defaultRadius);
		}
	}
	else {
		// Assign a uniform radius to all particles.
		boost::fill(output, defaultRadius);
	}

	return output;
}

/******************************************************************************
* Determines the display radius of a single particle.
******************************************************************************/
FloatType ParticlesVis::particleRadius(size_t particleIndex, ConstPropertyAccess<FloatType> radiusProperty, const PropertyObject* typeProperty) const
{
	OVITO_ASSERT(typeProperty == nullptr || typeProperty->type() == ParticlesObject::TypeProperty);

	if(radiusProperty && radiusProperty.size() > particleIndex) {
		// Take particle radius directly from the radius property.
		FloatType r = radiusProperty[particleIndex];
		if(r > 0) return r;
	}
	else if(typeProperty && typeProperty->size() > particleIndex) {
		// Assign radius based on particle types.
		ConstPropertyAccess<int> typeData(typeProperty);
		const ParticleType* ptype = static_object_cast<ParticleType>(typeProperty->elementType(typeData[particleIndex]));
		if(ptype && ptype->radius() > 0)
			return ptype->radius();
	}

	return defaultParticleRadius();
}

/******************************************************************************
* Determines the display color of a single particle.
******************************************************************************/
ColorA ParticlesVis::particleColor(size_t particleIndex, ConstPropertyAccess<Color> colorProperty, const PropertyObject* typeProperty, ConstPropertyAccess<int> selectionProperty, ConstPropertyAccess<FloatType> transparencyProperty) const
{
	// Check if particle is selected.
	if(selectionProperty && selectionProperty.size() > particleIndex) {
		if(selectionProperty[particleIndex])
			return selectionParticleColor();
	}

	ColorA c = defaultParticleColor();
	if(colorProperty && colorProperty.size() > particleIndex) {
		// Take particle color directly from the color property.
		c = colorProperty[particleIndex];
	}
	else if(typeProperty && typeProperty->size() > particleIndex) {
		// Return color based on particle types.
		ConstPropertyAccess<int> typeData(typeProperty);
		const ElementType* ptype = typeProperty->elementType(typeData[particleIndex]);
		if(ptype)
			c = ptype->color();
	}

	// Apply alpha component.
	if(transparencyProperty && transparencyProperty.size() > particleIndex) {
		c.a() = qBound(FloatType(0), FloatType(1) - transparencyProperty[particleIndex], FloatType(1));
	}

	return c;
}

/******************************************************************************
* Returns the actual rendering quality used to render the particles.
******************************************************************************/
ParticlePrimitive::RenderingQuality ParticlesVis::effectiveRenderingQuality(SceneRenderer* renderer, const ParticlesObject* particles) const
{
	ParticlePrimitive::RenderingQuality renderQuality = renderingQuality();
	if(renderQuality == ParticlePrimitive::AutoQuality) {
		if(!particles) return ParticlePrimitive::HighQuality;
		size_t particleCount = particles->elementCount();
		if(particleCount < 4000 || renderer->isInteractive() == false)
			renderQuality = ParticlePrimitive::HighQuality;
		else if(particleCount < 400000)
			renderQuality = ParticlePrimitive::MediumQuality;
		else
			renderQuality = ParticlePrimitive::LowQuality;
	}
	return renderQuality;
}

/******************************************************************************
* Returns the actual particle shape used to render the particles.
******************************************************************************/
ParticlePrimitive::ParticleShape ParticlesVis::effectiveParticleShape(const PropertyObject* shapeProperty, const PropertyObject* orientationProperty) const
{
	if(particleShape() == Sphere) {
		if(shapeProperty != nullptr) return ParticlePrimitive::EllipsoidShape;
		else return ParticlePrimitive::SphericalShape;
	}
	else if(particleShape() == Box) {
		if(shapeProperty != nullptr || orientationProperty != nullptr) return ParticlePrimitive::BoxShape;
		else return ParticlePrimitive::SquareCubicShape;
	}
	else if(particleShape() == Circle) {
		return ParticlePrimitive::SphericalShape;
	}
	else if(particleShape() == Square) {
		return ParticlePrimitive::SquareCubicShape;
	}
	else {
		OVITO_ASSERT(false);
		return ParticlePrimitive::SphericalShape;
	}
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
void ParticlesVis::render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, objectStack, contextNode, flowState, validityInterval));
		return;
	}

	// Get input data.
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(objectStack.back());
	if(!particles) return;
	qDebug() << "ParticlesVis::render: time" << time << particles;
	particles->verifyIntegrity();
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	const PropertyObject* radiusProperty = particles->getProperty(ParticlesObject::RadiusProperty);
	const PropertyObject* colorProperty = particles->getProperty(ParticlesObject::ColorProperty);
	const PropertyObject* typeProperty = getParticleTypeColorProperty(particles);
	const PropertyObject* typeRadiusProperty = getParticleTypeRadiusProperty(particles);
	const PropertyObject* selectionProperty = renderer->isInteractive() ? particles->getProperty(ParticlesObject::SelectionProperty) : nullptr;
	const PropertyObject* transparencyProperty = particles->getProperty(ParticlesObject::TransparencyProperty);
	const PropertyObject* asphericalShapeProperty = particles->getProperty(ParticlesObject::AsphericalShapeProperty);
	const PropertyObject* orientationProperty = particles->getProperty(ParticlesObject::OrientationProperty);

	// Check if any of the particle types have a user-defined mesh geometry assigned.
	std::vector<int> userShapeParticleTypes;
	if(typeProperty) {
		for(const ElementType* etype : typeProperty->elementTypes()) {
			if(const ParticleType* ptype = dynamic_object_cast<ParticleType>(etype)) {
				if(ptype->shapeMesh() && ptype->shapeMesh()->mesh() && ptype->shapeMesh()->mesh()->faceCount() != 0) {
					userShapeParticleTypes.push_back(ptype->numericId());
				}
			}
		}
	}

	if(particleShape() != Sphere && particleShape() != Box && particleShape() != Cylinder && particleShape() != Spherocylinder && userShapeParticleTypes.empty()) {
		asphericalShapeProperty = nullptr;
		orientationProperty = nullptr;
	}
	if(particleShape() == Sphere && asphericalShapeProperty == nullptr && userShapeParticleTypes.empty())
		orientationProperty = nullptr;

	// Make sure we don't exceed our internal limits.
	if(particles->elementCount() > (size_t)std::numeric_limits<int>::max()) {
		qWarning() << "WARNING: Cannot render more than" << std::numeric_limits<int>::max() << "particles.";
		return;
	}

	ConstPropertyPtr positionStorage = positionProperty ? positionProperty->storage() : nullptr;
	ConstPropertyPtr radiusStorage = radiusProperty ? radiusProperty->storage() : nullptr;
	ConstPropertyPtr colorStorage = colorProperty ? colorProperty->storage() : nullptr;
	ConstPropertyPtr asphericalShapeStorage = asphericalShapeProperty ? asphericalShapeProperty->storage() : nullptr;
	ConstPropertyPtr orientationStorage = orientationProperty ? orientationProperty->storage() : nullptr;

	// Get total number of particles.
	int particleCount = particles->elementCount();

	if(particleShape() != Cylinder && particleShape() != Spherocylinder) {

		// If rendering quality is set to automatic, pick quality level based on current number of particles.
		ParticlePrimitive::RenderingQuality renderQuality = effectiveRenderingQuality(renderer, particles);

		// Determine primitive particle shape and shading mode.
		ParticlePrimitive::ParticleShape primitiveParticleShape = effectiveParticleShape(asphericalShapeProperty, orientationProperty);
		ParticlePrimitive::ShadingMode primitiveShadingMode = ParticlePrimitive::NormalShading;
		if(particleShape() == Circle || particleShape() == Square)
			primitiveShadingMode = ParticlePrimitive::FlatShading;

		// The type of lookup key for caching the rendering primitive:
		using ParticleCacheKey = std::tuple<
			CompatibleRendererGroup,	// The scene renderer
			QPointer<PipelineSceneNode>,// The scene node
			VersionedDataObjectRef,		// Position property + revision number
			VersionedDataObjectRef		// Particle type property + revision number
		>;
		// The data structure stored in the vis cache.
		struct ParticleCacheValue {
			std::shared_ptr<ParticlePrimitive> particlePrimitive;
			OORef<ParticlePickInfo> pickInfo;
		};
		// Look up the rendering primitive in the vis cache.
		auto& visCache = dataset()->visCache().get<ParticleCacheValue>(ParticleCacheKey(
			renderer,
			const_cast<PipelineSceneNode*>(contextNode),
			positionProperty,
			typeProperty));

		// Check if we already have a valid rendering primitive that is up to date.
		if(!visCache.particlePrimitive
				|| !visCache.particlePrimitive->isValid(renderer)
				|| !visCache.particlePrimitive->setShadingMode(primitiveShadingMode)
				|| !visCache.particlePrimitive->setRenderingQuality(renderQuality)
				|| !visCache.particlePrimitive->setParticleShape(primitiveParticleShape)
				|| (transparencyProperty != nullptr) != visCache.particlePrimitive->translucentParticles()) {
			// Create the particle rendering primitive.
			visCache.particlePrimitive = renderer->createParticlePrimitive(primitiveShadingMode, renderQuality, primitiveParticleShape, transparencyProperty != nullptr);
		}

		// The type of lookup key used for caching the particle positions, orientations and shapes:
		using PositionCacheKey = std::tuple<
			std::shared_ptr<ParticlePrimitive>,	// The rendering primitive
			VersionedDataObjectRef,		// Position property + revision number
			VersionedDataObjectRef,		// Aspherical shape property + revision number
			VersionedDataObjectRef		// Orientation property + revision number
		>;
		bool& positionsUpToDate = dataset()->visCache().get<bool>(PositionCacheKey(
			visCache.particlePrimitive,
			positionProperty,
			asphericalShapeProperty,
			orientationProperty));

		// The type of lookup key used for caching the particle radii:
		using RadiiCacheKey = std::tuple<
			std::shared_ptr<ParticlePrimitive>,	// The rendering primitive
			FloatType,							// Default particle radius
			VersionedDataObjectRef,				// Radius property + revision number
			VersionedDataObjectRef				// Type property + revision number
		>;
		bool& radiiUpToDate = dataset()->visCache().get<bool>(RadiiCacheKey(
			visCache.particlePrimitive,
			defaultParticleRadius(),
			radiusProperty,
			typeRadiusProperty));

		// The type of lookup key used for caching the particle colors:
		using ColorCacheKey = std::tuple<
			std::shared_ptr<ParticlePrimitive>,	// The rendering primitive
			VersionedDataObjectRef,		// Type property + revision number
			VersionedDataObjectRef,		// Color property + revision number
			VersionedDataObjectRef,		// Selection property + revision number
			VersionedDataObjectRef		// Transparency property + revision number
		>;
		bool& colorsUpToDate = dataset()->visCache().get<bool>(ColorCacheKey(
			visCache.particlePrimitive,
			typeProperty,
			colorProperty,
			selectionProperty,
			transparencyProperty));

		// The type of lookup key used for caching the mesh rendering primitives:
		using ShapeMeshCacheKey = std::tuple<
			CompatibleRendererGroup,	// The scene renderer
			QPointer<PipelineSceneNode>,// The scene node
			VersionedDataObjectRef		// Particle type property + revision number
		>;
		// The data structure stored in the vis cache.
		struct ShapeMeshCacheValue {
			std::vector<std::shared_ptr<MeshPrimitive>> shapeMeshPrimitives;
			std::vector<bool> shapeUseMeshColor;
			std::vector<OORef<ObjectPickInfo>> pickInfos;
		};
		// Look up the rendering primitive in the vis cache.
		ShapeMeshCacheValue* meshVisCache = nullptr;
		if(!userShapeParticleTypes.empty()) {
			meshVisCache = &dataset()->visCache().get<ShapeMeshCacheValue>(ShapeMeshCacheKey(
				renderer,
				const_cast<PipelineSceneNode*>(contextNode),
				typeProperty));

			// Check if we already have a valid rendering primitive that is up to date.
			if(meshVisCache->shapeMeshPrimitives.empty() || !meshVisCache->shapeMeshPrimitives.front()->isValid(renderer)) {
				// Create the mesh rendering primitives.
				meshVisCache->shapeMeshPrimitives.clear();
				meshVisCache->shapeUseMeshColor.clear();
				for(int t : userShapeParticleTypes) {
					const ParticleType* ptype = static_object_cast<ParticleType>(typeProperty->elementType(t));
					OVITO_ASSERT(ptype->shapeMesh() && ptype->shapeMesh()->mesh());
					meshVisCache->shapeMeshPrimitives.push_back(renderer->createMeshPrimitive());
					meshVisCache->shapeMeshPrimitives.back()->setMesh(*ptype->shapeMesh()->mesh(), ColorA(1,1,1,1), ptype->highlightShapeEdges());
					meshVisCache->shapeMeshPrimitives.back()->setCullFaces(ptype->shapeBackfaceCullingEnabled());
					meshVisCache->shapeUseMeshColor.push_back(ptype->shapeUseMeshColor());
				}
			}

			// The type of lookup key used for caching the particle colors and orientations:
			using ParticleInfoCacheKey = std::tuple<
				std::shared_ptr<MeshPrimitive>,	// The rendering primitive
				FloatType,						// Default particle radius
				VersionedDataObjectRef,		// Position property + revision number
				VersionedDataObjectRef,		// Orientation property + revision number
				VersionedDataObjectRef,		// Color property + revision number
				VersionedDataObjectRef,		// Selection property + revision number
				VersionedDataObjectRef,		// Transparency property + revision number
				VersionedDataObjectRef		// Radius property + revision number
			>;
			bool& particleInfoUpToDate = dataset()->visCache().get<bool>(ParticleInfoCacheKey(
				meshVisCache->shapeMeshPrimitives.front(),
				defaultParticleRadius(),
				positionProperty,
				orientationProperty,
				colorProperty,
				selectionProperty,
				transparencyProperty,
				radiusProperty));

			// Update the cached per-particle information if necessary.
			if(!particleInfoUpToDate) {
				particleInfoUpToDate = true;

				// For each particle type with a user-defined shape, we build a list of transformation matrices and colors
				// of all particles to render.
				std::vector<std::vector<AffineTransformation>> shapeParticleTMs(meshVisCache->shapeMeshPrimitives.size());
				std::vector<std::vector<ColorA>> shapeParticleColors(meshVisCache->shapeMeshPrimitives.size());
				std::vector<std::vector<size_t>> shapeParticleIndices(meshVisCache->shapeMeshPrimitives.size());
				std::vector<ColorA> colors = particleColors(particles, renderer->isInteractive(), true);
				std::vector<FloatType> radii = particleRadii(particles);
				ConstPropertyAccess<int> typeArray(typeProperty);
				ConstPropertyAccess<Point3> positionArray(positionStorage);
				ConstPropertyAccess<Quaternion> orientationArray(orientationStorage);
				for(size_t i = 0; i < particleCount; i++) {
					auto iter = boost::find(userShapeParticleTypes, typeArray[i]);
					if(iter == userShapeParticleTypes.end()) continue;
					if(radii[i] <= 0) continue;
					size_t typeIndex = iter - userShapeParticleTypes.begin();
					AffineTransformation tm = AffineTransformation::scaling(radii[i]);
					if(positionArray)
						tm.translation() = positionArray[i] - Point3::Origin();
					if(orientationArray) {
						Quaternion quat = orientationArray[i];
						// Normalize quaternion.
						FloatType c = sqrt(quat.dot(quat));
						if(c <= FLOATTYPE_EPSILON)
							quat.setIdentity();
						else
							quat /= c;
						tm = tm * Matrix3::rotation(quat);
					}
					shapeParticleTMs[typeIndex].push_back(tm);
					shapeParticleIndices[typeIndex].push_back(i);
					shapeParticleColors[typeIndex].push_back(colors[i]);
				}

				// Store the per-particle data in the mesh rendering primitives.
				meshVisCache->pickInfos.clear();
				for(size_t i = 0; i < meshVisCache->shapeMeshPrimitives.size(); i++) {
					if(meshVisCache->shapeUseMeshColor[i])
						shapeParticleColors[i].clear();
					meshVisCache->shapeMeshPrimitives[i]->setInstancedRendering(std::move(shapeParticleTMs[i]), std::move(shapeParticleColors[i]));
					meshVisCache->pickInfos.emplace_back(new ParticlePickInfo(this, flowState, std::move(shapeParticleIndices[i])));
				}
			}
		}

		// Determine which particles must be rendered using the built-in rendering primitives and
		// which are rendered using more general triangle meshes.
		boost::dynamic_bitset<> hiddenParticlesMask;
		int visibleStandardParticles = particleCount;
		if(!positionsUpToDate || !radiiUpToDate || !colorsUpToDate) {
			std::vector<size_t> visibleParticleIndices;
			if(!userShapeParticleTypes.empty()) {

				// Create a bitmask that indicates which particles must be rendered with user-defined shapes
				// instead of the built-in primitives.
				hiddenParticlesMask.resize(typeProperty->size());
				size_t index = 0;
				for(int t : ConstPropertyAccess<int>(typeProperty)) {
					if(boost::find(userShapeParticleTypes, t) != userShapeParticleTypes.cend()) {
						hiddenParticlesMask.set(index);
						visibleStandardParticles--;
					}
					else {
						visibleParticleIndices.push_back(index);
					}
					index++;
				}
			}
			if(visibleStandardParticles == 0) {
				// All particles are using user-defined shape meshes for rendering.
				// No particles need to be rendered using the built-in primitives.
				visCache.particlePrimitive->setSize(0);
				positionsUpToDate = true;
				radiiUpToDate = true;
				colorsUpToDate = true;
			}
			visCache.pickInfo = new ParticlePickInfo(this, flowState, std::move(visibleParticleIndices));
		}
		else {
			// Update the pipeline state stored in te picking object info.
			visCache.pickInfo->setPipelineState(flowState);
		}

		// Make sure that the particle positions, orientations and aspherical shapes stored in the rendering primitive are up to date.
		if(!positionsUpToDate) {
			positionsUpToDate = true;

			visCache.particlePrimitive->setSize(visibleStandardParticles);
			if(positionStorage) {
				// Filter the property array to include only the visible particles.
				if(visibleStandardParticles != particleCount)
					positionStorage = positionStorage->filterCopy(hiddenParticlesMask);
				// Fill in the position data.
				visCache.particlePrimitive->setParticlePositions(ConstPropertyAccess<Point3>(positionStorage).cbegin());
			}
			if(asphericalShapeStorage) {
				// Filter the property array to include only the visible particles.
				if(visibleStandardParticles != particleCount)
					asphericalShapeStorage = asphericalShapeStorage->filterCopy(hiddenParticlesMask);
				// Fill in aspherical shape data.
				visCache.particlePrimitive->setParticleShapes(ConstPropertyAccess<Vector3>(asphericalShapeStorage).cbegin());
			}
			if(orientationStorage) {
				// Filter the property array to include only the visible particles.
				if(visibleStandardParticles != particleCount)
					orientationStorage = orientationStorage->filterCopy(hiddenParticlesMask);
				// Fill in orientation data.
				visCache.particlePrimitive->setParticleOrientations(ConstPropertyAccess<Quaternion>(orientationStorage).cbegin());
			}
		}

		// Make sure that the particle radii stored in the rendering primitive are up to date.
		if(!radiiUpToDate) {
			radiiUpToDate = true;

			if(radiusStorage) {
				// Use per-particle radius information.
				// Filter the property array to include only the visible particles.
				PropertyPtr positiveRadiusStorage;
				if(visibleStandardParticles != particleCount)
					positiveRadiusStorage = radiusStorage->filterCopy(hiddenParticlesMask);
				else
					positiveRadiusStorage = std::make_shared<PropertyStorage>(*radiusStorage);
				// Replace null entries in the per-particle radius array with the default radius.
				FloatType defaultRadius = defaultParticleRadius();
				for(FloatType& r : PropertyAccess<FloatType>(positiveRadiusStorage))
					if(r <= 0) r = defaultRadius;
				// Fill in radius data.
				visCache.particlePrimitive->setParticleRadii(ConstPropertyAccess<FloatType>(positiveRadiusStorage).cbegin());
			}
			else if(typeProperty) {
				// Assign radii based on particle types.
				// Build a lookup map for particle type radii.
				const std::map<int,FloatType> radiusMap = ParticleType::typeRadiusMap(typeProperty);
				// Skip the following loop if all per-type radii are zero. In this case, simply use the default radius for all particles.
				if(boost::algorithm::any_of(radiusMap, [](const std::pair<int,FloatType>& it) { return it.second != 0; })) {
					// Allocate value buffer.
					std::vector<FloatType> particleRadii(visibleStandardParticles, defaultParticleRadius());
					// Fill radius array.
					auto c = particleRadii.begin();
					size_t index = 0;
					for(int t : ConstPropertyAccess<int>(typeProperty)) {
						if(hiddenParticlesMask.empty() || !hiddenParticlesMask.test(index)) {
							auto it = radiusMap.find(t);
							// Set particle radius only if the type's radius is non-zero.
							if(it != radiusMap.end() && it->second != 0)
								*c = it->second;
							++c;
						}
						index++;
					}
					OVITO_ASSERT(c == particleRadii.end());
					visCache.particlePrimitive->setParticleRadii(particleRadii.data());
				}
				else {
					// Assign a uniform radius to all particles.
					visCache.particlePrimitive->setParticleRadius(defaultParticleRadius());
				}
			}
			else {
				// Assign a uniform radius to all particles.
				visCache.particlePrimitive->setParticleRadius(defaultParticleRadius());
			}
		}

		// Make sure that the particle colors stored in the rendering primitive are up to date.
		if(!colorsUpToDate) {
			colorsUpToDate = true;

			// Fill in color data.
			if(colorStorage && !selectionProperty && !transparencyProperty) {
				// Filter the property array to include only the visible particles.
				if(visibleStandardParticles != particleCount)
					colorStorage = colorStorage->filterCopy(hiddenParticlesMask);
				// Directly use particle colors.
				visCache.particlePrimitive->setParticleColors(ConstPropertyAccess<Color>(colorStorage).cbegin());
			}
			else {
				std::vector<ColorA> colors = particleColors(particles, renderer->isInteractive(), true);
				// Filter the color array to include only the visible particles.
				if(visibleStandardParticles != particleCount) {
					auto c = colors.begin();
					for(size_t i = 0; i < particleCount; i++)
						if(!hiddenParticlesMask.test(i))
							*c++ = colors[i];
				}
				visCache.particlePrimitive->setParticleColors(colors.data());
			}
		}

		if(renderer->isPicking())
			renderer->beginPickObject(contextNode, visCache.pickInfo);

		visCache.particlePrimitive->render(renderer);

		if(renderer->isPicking())
			renderer->endPickObject();

		if(meshVisCache) {
			auto pickInfo = meshVisCache->pickInfos.cbegin();
			for(const auto& primitive : meshVisCache->shapeMeshPrimitives) {
				if(renderer->isPicking())
					renderer->beginPickObject(contextNode, *pickInfo++);
				primitive->render(renderer);
				if(renderer->isPicking())
					renderer->endPickObject();
			}
		}
	}
	else {
		// Rendering cylindrical and spherocylindrical particles.

		// The key type used for caching the sphere rendering primitive:
		using SpherocylinderCacheKey = std::tuple<
			CompatibleRendererGroup,	// The scene renderer
			VersionedDataObjectRef,		// Position property + revision number
			VersionedDataObjectRef,		// Type property + revision number
			VersionedDataObjectRef,		// Selection property + revision number
			VersionedDataObjectRef,		// Color property + revision number
			VersionedDataObjectRef,		// Apherical shape property + revision number
			VersionedDataObjectRef,		// Orientation property + revision number
			FloatType,					// Default particle radius
			ParticleShape				// Display shape
		>;
		// The data structure stored in the vis cache.
		struct CacheValue {
			std::shared_ptr<ParticlePrimitive> spheresPrimitive;
			std::shared_ptr<ArrowPrimitive> cylinderPrimitive;
			OORef<ObjectPickInfo> pickInfo;
		};

		// Look up the existing rendering primitives in the vis cache.
		auto& visCache = dataset()->visCache().get<CacheValue>(SpherocylinderCacheKey(
			renderer,
			positionProperty,
			typeProperty,
			selectionProperty,
			colorProperty,
			asphericalShapeProperty,
			orientationProperty,
			defaultParticleRadius(),
			particleShape()));

		if(particleShape() == Spherocylinder) {
			// Check if we already have a valid rendering primitive for the spheres that is up to date.
			if(!visCache.spheresPrimitive
					|| !visCache.spheresPrimitive->isValid(renderer)
					|| (visCache.spheresPrimitive->particleCount() != particleCount * 2)) {

				// Recreate the rendering primitive for the spheres.
				visCache.spheresPrimitive = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::HighQuality, ParticlePrimitive::SphericalShape, false);
				visCache.spheresPrimitive->setSize(particleCount * 2);
			}
		}
		else visCache.spheresPrimitive.reset();

		// Check if we already have a valid rendering primitive for the cylinders that is up to date.
		if(!visCache.cylinderPrimitive
				|| !visCache.cylinderPrimitive->isValid(renderer)
				|| visCache.cylinderPrimitive->elementCount() != particleCount
				|| !visCache.cylinderPrimitive->setShadingMode(ArrowPrimitive::NormalShading)
				|| !visCache.cylinderPrimitive->setRenderingQuality(ArrowPrimitive::HighQuality)
				|| visCache.cylinderPrimitive->shape() != ArrowPrimitive::CylinderShape) {

			// Recreate the rendering primitive for the cylinders.
			visCache.cylinderPrimitive = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, ArrowPrimitive::NormalShading, ArrowPrimitive::HighQuality);

			// Determine cylinder colors.
			std::vector<ColorA> colors = particleColors(particles, renderer->isInteractive(), true);

			std::vector<Point3> sphereCapPositions;
			std::vector<FloatType> sphereRadii;
			std::vector<ColorA> sphereColors;
			if(visCache.spheresPrimitive) {
				sphereCapPositions.resize(particleCount * 2);
				sphereRadii.resize(particleCount * 2);
				sphereColors.resize(particleCount * 2);
			}

			// Fill cylinder buffer.
			visCache.cylinderPrimitive->startSetElements(particleCount);
			ConstPropertyAccess<Point3> positionArray(positionStorage);
			ConstPropertyAccess<Vector3> asphericalShapeArray(asphericalShapeStorage);
			ConstPropertyAccess<Quaternion> orientationArray(orientationStorage);
			for(int index = 0; index < particleCount; index++) {
				const Point3& center = positionArray[index];
				FloatType radius, length;
				if(asphericalShapeArray) {
					radius = std::abs(asphericalShapeArray[index].x());
					length = asphericalShapeArray[index].z();
				}
				else {
					radius = defaultParticleRadius();
					length = radius * 2;
				}
				Vector3 dir = Vector3(0, 0, length);
				if(orientationArray) {
					const Quaternion& q = orientationArray[index];
					dir = q * dir;
				}
				Point3 p = center - (dir * FloatType(0.5));
				if(visCache.spheresPrimitive) {
					sphereCapPositions[index*2] = p;
					sphereCapPositions[index*2+1] = p + dir;
					sphereRadii[index*2] = sphereRadii[index*2+1] = radius;
					sphereColors[index*2] = sphereColors[index*2+1] = colors[index];
				}
				visCache.cylinderPrimitive->setElement(index, p, dir, colors[index], radius);
			}
			visCache.cylinderPrimitive->endSetElements();

			// Fill geometry buffer for spherical caps of spherocylinders.
			if(visCache.spheresPrimitive) {
				visCache.spheresPrimitive->setSize(particleCount * 2);
				visCache.spheresPrimitive->setParticlePositions(sphereCapPositions.data());
				visCache.spheresPrimitive->setParticleRadii(sphereRadii.data());
				visCache.spheresPrimitive->setParticleColors(sphereColors.data());
			}
		}

		if(renderer->isPicking()) {
			if(!visCache.pickInfo) {
				std::vector<size_t> subobjectMapping;
				if(visCache.spheresPrimitive) {
					subobjectMapping.resize(particleCount * 3);
					auto iter = subobjectMapping.begin();
					for(size_t i = 0; i < particleCount; i++) {
						*iter++ = i;
					}
					for(size_t i = 0; i < particleCount; i++) {
						*iter++ = i; *iter++ = i;
					}
				}
				visCache.pickInfo = new ParticlePickInfo(this, flowState, std::move(subobjectMapping));
			}
			renderer->beginPickObject(contextNode, visCache.pickInfo);
		}
		visCache.cylinderPrimitive->render(renderer);
		if(visCache.spheresPrimitive)
			visCache.spheresPrimitive->render(renderer);
		if(renderer->isPicking()) {
			renderer->endPickObject();
		}
	}
}

/******************************************************************************
* Render a marker around a particle to highlight it in the viewports.
******************************************************************************/
void ParticlesVis::highlightParticle(size_t particleIndex, const ParticlesObject* particles, SceneRenderer* renderer) const
{
	if(!renderer->isBoundingBoxPass()) {

		// Fetch properties of selected particle which are needed to render the overlay.
		const PropertyObject* posProperty = nullptr;
		const PropertyObject* radiusProperty = nullptr;
		const PropertyObject* colorProperty = nullptr;
		const PropertyObject* selectionProperty = nullptr;
		const PropertyObject* transparencyProperty = nullptr;
		const PropertyObject* shapeProperty = nullptr;
		const PropertyObject* orientationProperty = nullptr;
		const PropertyObject* typeProperty = nullptr;
		for(const PropertyObject* property : particles->properties()) {
			if(property->type() == ParticlesObject::PositionProperty && property->size() >= particleIndex)
				posProperty = property;
			else if(property->type() == ParticlesObject::RadiusProperty && property->size() >= particleIndex)
				radiusProperty = property;
			else if(property->type() == ParticlesObject::TypeProperty && property->size() >= particleIndex)
				typeProperty = property;
			else if(property->type() == ParticlesObject::ColorProperty && property->size() >= particleIndex)
				colorProperty = property;
			else if(property->type() == ParticlesObject::SelectionProperty && property->size() >= particleIndex)
				selectionProperty = property;
			else if(property->type() == ParticlesObject::TransparencyProperty && property->size() >= particleIndex)
				transparencyProperty = property;
			else if(property->type() == ParticlesObject::AsphericalShapeProperty && property->size() >= particleIndex)
				shapeProperty = property;
			else if(property->type() == ParticlesObject::OrientationProperty && property->size() >= particleIndex)
				orientationProperty = property;
		}
		if(!posProperty || particleIndex >= posProperty->size())
			return;

		// Check if the particle must be rendered using a custom shape.
		if(typeProperty && particleIndex < typeProperty->size()) {
			ConstPropertyAccess<int> typeArray(typeProperty);
			if(ParticleType* ptype = dynamic_object_cast<ParticleType>(typeProperty->elementType(typeArray[particleIndex]))) {
				if(ptype->shapeMesh())
					return;	// Note: Highlighting of particles with user-defined shapes is not implemented yet.
			}
		}

		// Determine position of selected particle.
		Point3 pos = ConstPropertyAccess<Point3>(posProperty)[particleIndex];

		// Determine radius of selected particle.
		FloatType radius = particleRadius(particleIndex, radiusProperty, typeProperty);

		// Determine the display color of selected particle.
		ColorA color = particleColor(particleIndex, colorProperty, typeProperty, selectionProperty, transparencyProperty);
		ColorA highlightColor = selectionParticleColor();
		color = color * FloatType(0.5) + highlightColor * FloatType(0.5);

		// Determine rendering quality used to render the particles.
		ParticlePrimitive::RenderingQuality renderQuality = effectiveRenderingQuality(renderer, particles);

		std::shared_ptr<ParticlePrimitive> particleBuffer;
		std::shared_ptr<ParticlePrimitive> highlightParticleBuffer;
		std::shared_ptr<ArrowPrimitive> cylinderBuffer;
		std::shared_ptr<ArrowPrimitive> highlightCylinderBuffer;
		if(particleShape() != Cylinder && particleShape() != Spherocylinder) {
			// Determine effective particle shape and shading mode.
			ParticlePrimitive::ParticleShape primitiveParticleShape = effectiveParticleShape(shapeProperty, orientationProperty);
			ParticlePrimitive::ShadingMode primitiveShadingMode = ParticlePrimitive::NormalShading;
			if(particleShape() == ParticlesVis::Circle || particleShape() == ParticlesVis::Square)
				primitiveShadingMode = ParticlePrimitive::FlatShading;

			particleBuffer = renderer->createParticlePrimitive(primitiveShadingMode, renderQuality, primitiveParticleShape, false);
			particleBuffer->setSize(1);
			particleBuffer->setParticleColor(color);
			particleBuffer->setParticlePositions(&pos);
			particleBuffer->setParticleRadius(radius);
			if(shapeProperty)
				particleBuffer->setParticleShapes(ConstPropertyAccess<Vector3>(shapeProperty).cbegin() + particleIndex);
			if(orientationProperty)
				particleBuffer->setParticleOrientations(ConstPropertyAccess<Quaternion>(orientationProperty).cbegin() + particleIndex);

			// Prepare marker geometry buffer.
			highlightParticleBuffer = renderer->createParticlePrimitive(primitiveShadingMode, renderQuality, primitiveParticleShape, false);
			highlightParticleBuffer->setSize(1);
			highlightParticleBuffer->setParticleColor(highlightColor);
			highlightParticleBuffer->setParticlePositions(&pos);
			highlightParticleBuffer->setParticleRadius(radius + renderer->viewport()->nonScalingSize(renderer->worldTransform() * pos) * FloatType(1e-1));
			if(shapeProperty) {
				Vector3 shape = ConstPropertyAccess<Vector3>(shapeProperty)[particleIndex];
				shape += Vector3(renderer->viewport()->nonScalingSize(renderer->worldTransform() * pos) * FloatType(1e-1));
				highlightParticleBuffer->setParticleShapes(&shape);
			}
			if(orientationProperty)
				highlightParticleBuffer->setParticleOrientations(ConstPropertyAccess<Quaternion>(orientationProperty).cbegin() + particleIndex);
		}
		else {
			FloatType radius, length;
			if(shapeProperty) {
				Vector3 shape = ConstPropertyAccess<Vector3>(shapeProperty)[particleIndex];
				radius = std::abs(shape.x());
				length = shape.z();
			}
			else {
				radius = defaultParticleRadius();
				length = radius * 2;
			}
			Vector3 dir = Vector3(0, 0, length);
			if(orientationProperty) {
				Quaternion q = ConstPropertyAccess<Quaternion>(orientationProperty)[particleIndex];
				dir = q * dir;
			}
			Point3 p = pos - (dir * FloatType(0.5));
			cylinderBuffer = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, ArrowPrimitive::NormalShading, ArrowPrimitive::HighQuality);
			highlightCylinderBuffer = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, ArrowPrimitive::NormalShading, ArrowPrimitive::HighQuality);
			cylinderBuffer->startSetElements(1);
			cylinderBuffer->setElement(0, p, dir, (ColorA)color, radius);
			cylinderBuffer->endSetElements();
			FloatType padding = renderer->viewport()->nonScalingSize(renderer->worldTransform() * pos) * FloatType(1e-1);
			highlightCylinderBuffer->startSetElements(1);
			highlightCylinderBuffer->setElement(0, p, dir, highlightColor, radius + padding);
			highlightCylinderBuffer->endSetElements();
			if(particleShape() == Spherocylinder) {
				particleBuffer = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::HighQuality, ParticlePrimitive::SphericalShape, false);
				particleBuffer->setSize(2);
				highlightParticleBuffer = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::HighQuality, ParticlePrimitive::SphericalShape, false);
				highlightParticleBuffer->setSize(2);
				Point3 sphereCapPositions[2] = {p, p + dir};
				FloatType sphereRadii[2] = {radius, radius};
				FloatType sphereHighlightRadii[2] = {radius + padding, radius + padding};
				Color sphereColors[2] = {(Color)color, (Color)color};
				particleBuffer->setParticlePositions(sphereCapPositions);
				particleBuffer->setParticleRadii(sphereRadii);
				particleBuffer->setParticleColors(sphereColors);
				highlightParticleBuffer->setParticlePositions(sphereCapPositions);
				highlightParticleBuffer->setParticleRadii(sphereHighlightRadii);
				highlightParticleBuffer->setParticleColor(highlightColor);
			}
		}

		renderer->setHighlightMode(1);
		if(particleBuffer)
			particleBuffer->render(renderer);
		if(cylinderBuffer)
			cylinderBuffer->render(renderer);
		renderer->setHighlightMode(2);
		if(highlightParticleBuffer)
			highlightParticleBuffer->render(renderer);
		if(highlightCylinderBuffer)
			highlightCylinderBuffer->render(renderer);
		renderer->setHighlightMode(0);
	}
	else {
		// Fetch properties of selected particle needed to compute the bounding box.
		const PropertyObject* posProperty = nullptr;
		const PropertyObject* radiusProperty = nullptr;
		const PropertyObject* shapeProperty = nullptr;
		const PropertyObject* typeProperty = nullptr;
		for(const PropertyObject* property : particles->properties()) {
			if(property->type() == ParticlesObject::PositionProperty && property->size() >= particleIndex)
				posProperty = property;
			else if(property->type() == ParticlesObject::RadiusProperty && property->size() >= particleIndex)
				radiusProperty = property;
			else if(property->type() == ParticlesObject::AsphericalShapeProperty && property->size() >= particleIndex)
				shapeProperty = property;
			else if(property->type() == ParticlesObject::TypeProperty && property->size() >= particleIndex)
				typeProperty = property;
		}
		if(!posProperty)
			return;

		// Determine position of selected particle.
		Point3 pos = ConstPropertyAccess<Point3>(posProperty)[particleIndex];

		// Determine radius of selected particle.
		FloatType radius = particleRadius(particleIndex, radiusProperty, typeProperty);
		if(shapeProperty) {
			Vector3 shape = ConstPropertyAccess<Vector3>(shapeProperty)[particleIndex];
			radius = std::max(radius, shape.x());
			radius = std::max(radius, shape.y());
			radius = std::max(radius, shape.z());
			radius *= 2;
		}

		if(radius <= 0 || !renderer->viewport())
			return;

		const AffineTransformation& tm = renderer->worldTransform();
		renderer->addToLocalBoundingBox(Box3(pos, radius + renderer->viewport()->nonScalingSize(tm * pos) * FloatType(1e-1)));
	}
}

/******************************************************************************
* Given an sub-object ID returned by the Viewport::pick() method, looks up the
* corresponding particle index.
******************************************************************************/
size_t ParticlePickInfo::particleIndexFromSubObjectID(quint32 subobjID) const
{
	if(subobjID < _subobjectToParticleMapping.size())
		return _subobjectToParticleMapping[subobjID];
	return subobjID;
}

/******************************************************************************
* Returns a human-readable string describing the picked object,
* which will be displayed in the status bar by OVITO.
******************************************************************************/
QString ParticlePickInfo::infoString(PipelineSceneNode* objectNode, quint32 subobjectId)
{
	size_t particleIndex = particleIndexFromSubObjectID(subobjectId);
	return particleInfoString(pipelineState(), particleIndex);
}

/******************************************************************************
* Builds the info string for a particle to be displayed in the status bar.
******************************************************************************/
QString ParticlePickInfo::particleInfoString(const PipelineFlowState& pipelineState, size_t particleIndex)
{
	QString str;
	if(const ParticlesObject* particles = pipelineState.getObject<ParticlesObject>()) {
		for(const PropertyObject* property : particles->properties()) {
			if(property->size() <= particleIndex) continue;
			if(property->type() == ParticlesObject::SelectionProperty) continue;
			if(property->type() == ParticlesObject::ColorProperty) continue;
			if(!str.isEmpty()) str += QStringLiteral(" | ");
			str += property->name();
			str += QStringLiteral(" ");
			if(property->dataType() == PropertyStorage::Int) {
				ConstPropertyAccess<int, true> data(property);
				for(size_t component = 0; component < data.componentCount(); component++) {
					if(component != 0) str += QStringLiteral(", ");
					str += QString::number(data.get(particleIndex, component));
					if(property->elementTypes().empty() == false) {
						if(const ElementType* ptype = property->elementType(data.get(particleIndex, component))) {
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
					str += QString::number(data.get(particleIndex, component));
				}
			}
			else if(property->dataType() == PropertyStorage::Float) {
				ConstPropertyAccess<FloatType, true> data(property);
				for(size_t component = 0; component < property->componentCount(); component++) {
					if(component != 0) str += QStringLiteral(", ");
					str += QString::number(data.get(particleIndex, component));
				}
			}
			else {
				str += QStringLiteral("<%1>").arg(QMetaType::typeName(property->dataType()) ? QMetaType::typeName(property->dataType()) : "unknown");
			}
		}
	}
	return str;
}

}	// End of namespace
}	// End of namespace
