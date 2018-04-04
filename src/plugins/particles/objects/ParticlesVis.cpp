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
#include <plugins/particles/objects/ParticleType.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/dataset/data/VersionedDataObjectRef.h>
#include <core/rendering/SceneRenderer.h>
#include <core/rendering/ParticlePrimitive.h>
#include <core/rendering/ArrowPrimitive.h>
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
Box3 ParticlesVis::boundingBox(TimePoint time, DataObject* dataObject, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	ParticleProperty* positionProperty = dynamic_object_cast<ParticleProperty>(dataObject);
	ParticleProperty* radiusProperty = ParticleProperty::findInState(flowState, ParticleProperty::RadiusProperty);
	ParticleProperty* typeProperty = ParticleProperty::findInState(flowState, ParticleProperty::TypeProperty);
	ParticleProperty* shapeProperty = ParticleProperty::findInState(flowState, ParticleProperty::AsphericalShapeProperty);

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
Box3 ParticlesVis::particleBoundingBox(ParticleProperty* positionProperty, ParticleProperty* typeProperty, ParticleProperty* radiusProperty, ParticleProperty* shapeProperty, bool includeParticleRadius)
{
	OVITO_ASSERT(positionProperty == nullptr || positionProperty->type() == ParticleProperty::PositionProperty);
	OVITO_ASSERT(typeProperty == nullptr || typeProperty->type() == ParticleProperty::TypeProperty);
	OVITO_ASSERT(radiusProperty == nullptr || radiusProperty->type() == ParticleProperty::RadiusProperty);
	OVITO_ASSERT(shapeProperty == nullptr || shapeProperty->type() == ParticleProperty::AsphericalShapeProperty);
	if(particleShape() != Sphere && particleShape() != Box && particleShape() != Cylinder && particleShape() != Spherocylinder)
		shapeProperty = nullptr;

	Box3 bbox;
	if(positionProperty) {
		bbox.addPoints(positionProperty->constDataPoint3(), positionProperty->size());
	}
	if(!includeParticleRadius)
		return bbox;

	// Extend box to account for radii/shape of particles.
	FloatType maxAtomRadius = 0;
	if(typeProperty) {
		for(const auto& it : ParticleType::typeRadiusMap(typeProperty)) {
			maxAtomRadius = std::max(maxAtomRadius, (it.second != 0) ? it.second : defaultParticleRadius());
		}
	}
	if(maxAtomRadius == 0)
		maxAtomRadius = defaultParticleRadius();
	if(shapeProperty) {
		for(const Vector3& s : shapeProperty->constVector3Range())
			maxAtomRadius = std::max(maxAtomRadius, std::max(s.x(), std::max(s.y(), s.z())));
		if(particleShape() == Spherocylinder)
			maxAtomRadius *= 2;
	}
	if(radiusProperty && radiusProperty->size() > 0) {
		auto minmax = std::minmax_element(radiusProperty->constDataFloat(), radiusProperty->constDataFloat() + radiusProperty->size());
		if(*minmax.first <= 0)
			maxAtomRadius = std::max(maxAtomRadius, *minmax.second);
		else
			maxAtomRadius = *minmax.second;
	}

	// Extend the bounding box by the largest particle radius.
	return bbox.padBox(std::max(maxAtomRadius * sqrt(FloatType(3)), FloatType(0)));
}

/******************************************************************************
* Determines the display particle colors.
******************************************************************************/
void ParticlesVis::particleColors(std::vector<Color>& output, ParticleProperty* colorProperty, ParticleProperty* typeProperty, ParticleProperty* selectionProperty)
{
	OVITO_ASSERT(colorProperty == nullptr || colorProperty->type() == ParticleProperty::ColorProperty);
	OVITO_ASSERT(typeProperty == nullptr || typeProperty->type() == ParticleProperty::TypeProperty);
	OVITO_ASSERT(selectionProperty == nullptr || selectionProperty->type() == ParticleProperty::SelectionProperty);

	Color defaultColor = defaultParticleColor();
	if(colorProperty && colorProperty->size() == output.size()) {
		// Take particle colors directly from the color property.
		std::copy(colorProperty->constDataColor(), colorProperty->constDataColor() + output.size(), output.begin());
	}
	else if(typeProperty && typeProperty->size() == output.size()) {
		// Assign colors based on particle types.
		// Generate a lookup map for particle type colors.
		const std::map<int,Color> colorMap = typeProperty->typeColorMap();
		std::array<Color,16> colorArray;
		// Check if all type IDs are within a small, non-negative range.
		// If yes, we can use an array lookup strategy. Otherwise we have to use a dictionary lookup strategy, which is slower.
		if(std::all_of(colorMap.begin(), colorMap.end(),
				[&colorArray](const std::map<int,Color>::value_type& i) { return i.first >= 0 && i.first < (int)colorArray.size(); })) {
			colorArray.fill(defaultColor);
			for(const auto& entry : colorMap)
				colorArray[entry.first] = entry.second;
			// Fill color array.
			const int* t = typeProperty->constDataInt();
			for(auto c = output.begin(); c != output.end(); ++c, ++t) {
				if(*t >= 0 && *t < (int)colorArray.size())
					*c = colorArray[*t];
				else
					*c = defaultColor;
			}
		}
		else {
			// Fill color array.
			const int* t = typeProperty->constDataInt();
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
		std::fill(output.begin(), output.end(), defaultColor);
	}

	// Highlight selected particles.
	if(selectionProperty && selectionProperty->size() == output.size()) {
		const Color selColor = selectionParticleColor();
		const int* t = selectionProperty->constDataInt();
		for(auto c = output.begin(); c != output.end(); ++c, ++t) {
			if(*t)
				*c = selColor;
		}
	}
}

/******************************************************************************
* Determines the display particle radii.
******************************************************************************/
void ParticlesVis::particleRadii(std::vector<FloatType>& output, ParticleProperty* radiusProperty, ParticleProperty* typeProperty)
{
	OVITO_ASSERT(radiusProperty == nullptr || radiusProperty->type() == ParticleProperty::RadiusProperty);
	OVITO_ASSERT(typeProperty == nullptr || typeProperty->type() == ParticleProperty::TypeProperty);

	FloatType defaultRadius = defaultParticleRadius();
	if(radiusProperty && radiusProperty->size() == output.size()) {
		// Take particle radii directly from the radius property.
		std::transform(radiusProperty->constDataFloat(), radiusProperty->constDataFloat() + output.size(), 
			output.begin(), [defaultRadius](FloatType r) { return r > 0 ? r : defaultRadius; } );
	}
	else if(typeProperty && typeProperty->size() == output.size()) {
		// Assign radii based on particle types.
		// Build a lookup map for particle type radii.
		const std::map<int,FloatType> radiusMap = ParticleType::typeRadiusMap(typeProperty);
		// Skip the following loop if all per-type radii are zero. In this case, simply use the default radius for all particles.
		if(std::any_of(radiusMap.cbegin(), radiusMap.cend(), [](const std::pair<int,FloatType>& it) { return it.second != 0; })) {
			// Fill radius array.
			const int* t = typeProperty->constDataInt();
			for(auto c = output.begin(); c != output.end(); ++c, ++t) {
				auto it = radiusMap.find(*t);
				// Set particle radius only if the type's radius is non-zero.
				if(it != radiusMap.end() && it->second != 0)
					*c = it->second;
				else
					*c = defaultRadius;
			}
		}
		else {
			// Assign a uniform radius to all particles.
			std::fill(output.begin(), output.end(), defaultRadius);
		}
	}
	else {
		// Assign a uniform radius to all particles.
		std::fill(output.begin(), output.end(), defaultRadius);
	}
}

/******************************************************************************
* Determines the display radius of a single particle.
******************************************************************************/
FloatType ParticlesVis::particleRadius(size_t particleIndex, ParticleProperty* radiusProperty, ParticleProperty* typeProperty)
{
	OVITO_ASSERT(radiusProperty == nullptr || radiusProperty->type() == ParticleProperty::RadiusProperty);
	OVITO_ASSERT(typeProperty == nullptr || typeProperty->type() == ParticleProperty::TypeProperty);

	if(radiusProperty && radiusProperty->size() > particleIndex) {
		// Take particle radius directly from the radius property.
		FloatType r = radiusProperty->getFloat(particleIndex);
		if(r > 0) return r;
	}
	else if(typeProperty && typeProperty->size() > particleIndex) {
		// Assign radius based on particle types.
		ParticleType* ptype = static_object_cast<ParticleType>(typeProperty->elementType(typeProperty->getInt(particleIndex)));
		if(ptype && ptype->radius() > 0)
			return ptype->radius();
	}

	return defaultParticleRadius();
}

/******************************************************************************
* Determines the display color of a single particle.
******************************************************************************/
ColorA ParticlesVis::particleColor(size_t particleIndex, ParticleProperty* colorProperty, ParticleProperty* typeProperty, ParticleProperty* selectionProperty, ParticleProperty* transparencyProperty)
{
	OVITO_ASSERT(colorProperty == nullptr || colorProperty->type() == ParticleProperty::ColorProperty);
	OVITO_ASSERT(typeProperty == nullptr || typeProperty->type() == ParticleProperty::TypeProperty);
	OVITO_ASSERT(selectionProperty == nullptr || selectionProperty->type() == ParticleProperty::SelectionProperty);
	OVITO_ASSERT(transparencyProperty == nullptr || transparencyProperty->type() == ParticleProperty::TransparencyProperty);

	// Check if particle is selected.
	if(selectionProperty && selectionProperty->size() > particleIndex) {
		if(selectionProperty->getInt(particleIndex))
			return selectionParticleColor();
	}

	ColorA c = defaultParticleColor();
	if(colorProperty && colorProperty->size() > particleIndex) {
		// Take particle color directly from the color property.
		c = colorProperty->getColor(particleIndex);
	}
	else if(typeProperty && typeProperty->size() > particleIndex) {
		// Return color based on particle types.
		ElementType* ptype = typeProperty->elementType(typeProperty->getInt(particleIndex));
		if(ptype)
			c = ptype->color();
	}

	// Apply alpha component.
	if(transparencyProperty && transparencyProperty->size() > particleIndex) {
		c.a() = FloatType(1) - transparencyProperty->getFloat(particleIndex);
	}

	return c;
}

/******************************************************************************
* Returns the actual rendering quality used to render the particles.
******************************************************************************/
ParticlePrimitive::RenderingQuality ParticlesVis::effectiveRenderingQuality(SceneRenderer* renderer, ParticleProperty* positionProperty) const
{
	ParticlePrimitive::RenderingQuality renderQuality = renderingQuality();
	if(renderQuality == ParticlePrimitive::AutoQuality) {
		if(!positionProperty) return ParticlePrimitive::HighQuality;
		size_t particleCount = positionProperty->size();
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
ParticlePrimitive::ParticleShape ParticlesVis::effectiveParticleShape(ParticleProperty* shapeProperty, ParticleProperty* orientationProperty) const
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
void ParticlesVis::render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode)
{
	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, dataObject, contextNode, flowState, validityInterval));
		return;
	}

	// Get input data.
	ParticleProperty* positionProperty = dynamic_object_cast<ParticleProperty>(dataObject);
	ParticleProperty* radiusProperty = ParticleProperty::findInState(flowState, ParticleProperty::RadiusProperty);
	ParticleProperty* colorProperty = ParticleProperty::findInState(flowState, ParticleProperty::ColorProperty);
	ParticleProperty* typeProperty = ParticleProperty::findInState(flowState, ParticleProperty::TypeProperty);
	ParticleProperty* selectionProperty = renderer->isInteractive() ? ParticleProperty::findInState(flowState, ParticleProperty::SelectionProperty) : nullptr;
	ParticleProperty* transparencyProperty = ParticleProperty::findInState(flowState, ParticleProperty::TransparencyProperty);
	ParticleProperty* shapeProperty = ParticleProperty::findInState(flowState, ParticleProperty::AsphericalShapeProperty);
	ParticleProperty* orientationProperty = ParticleProperty::findInState(flowState, ParticleProperty::OrientationProperty);
	if(particleShape() != Sphere && particleShape() != Box && particleShape() != Cylinder && particleShape() != Spherocylinder) {
		shapeProperty = nullptr;
		orientationProperty = nullptr;
	}
	if(particleShape() == Sphere && shapeProperty == nullptr)
		orientationProperty = nullptr;

	// Make sure we don't exceed our internal limits.
	if(positionProperty && positionProperty->size() > (size_t)std::numeric_limits<int>::max()) {
		qWarning() << "WARNING: Cannot render more than" << std::numeric_limits<int>::max() << "particles.";
		return;
	}

	// Get number of particles.
	int particleCount = positionProperty ? (int)positionProperty->size() : 0;

	if(particleShape() != Cylinder && particleShape() != Spherocylinder) {

		// The key type used for caching the rendering primitive:
		using ParticleCacheKey = std::tuple<
			CompatibleRendererGroup,	// The scene renderer
			QPointer<PipelineSceneNode>,// The scene node
			VersionedDataObjectRef,		// The 'Position' particle property
			VersionedDataObjectRef,		// Shape property + revision number
			VersionedDataObjectRef		// Orientation property + revision number
		>;

		// If rendering quality is set to automatic, pick quality level based on number of particles.
		ParticlePrimitive::RenderingQuality renderQuality = effectiveRenderingQuality(renderer, positionProperty);

		// Determine primitive particle shape and shading mode.
		ParticlePrimitive::ParticleShape primitiveParticleShape = effectiveParticleShape(shapeProperty, orientationProperty);
		ParticlePrimitive::ShadingMode primitiveShadingMode = ParticlePrimitive::NormalShading;
		if(particleShape() == Circle || particleShape() == Square)
			primitiveShadingMode = ParticlePrimitive::FlatShading;

		// Look up the rendering primitive in the vis cache.
		auto& particlePrimitive = dataset()->visCache().get<std::shared_ptr<ParticlePrimitive>>(ParticleCacheKey(
			renderer, 
			contextNode,
			positionProperty,
			shapeProperty, 
			orientationProperty));

		// Check if we already have a valid rendering primitive that is up to date.
		if(!particlePrimitive 
				|| !particlePrimitive->isValid(renderer) 
				|| particlePrimitive->particleCount() != particleCount
				|| !particlePrimitive->setShadingMode(primitiveShadingMode)
				|| !particlePrimitive->setRenderingQuality(renderQuality)
				|| !particlePrimitive->setParticleShape(primitiveParticleShape)
				|| (transparencyProperty != nullptr) != particlePrimitive->translucentParticles()) {

			// Recreate the rendering primitive for the particles.
			particlePrimitive = renderer->createParticlePrimitive(primitiveShadingMode, renderQuality, primitiveParticleShape, transparencyProperty != nullptr);
			particlePrimitive->setSize(particleCount);
			// Fill in the position data.
			particlePrimitive->setParticlePositions(positionProperty->constDataPoint3());

			// Fill in shape data.
			if(shapeProperty && shapeProperty->size() == particleCount)
				particlePrimitive->setParticleShapes(shapeProperty->constDataVector3());
			// Fill in orientation data.
			if(orientationProperty && orientationProperty->size() == particleCount)
				particlePrimitive->setParticleOrientations(orientationProperty->constDataQuaternion());			
		}

		// The key type used for caching the particle radii:
		using RadiiCacheKey = std::tuple<
			std::shared_ptr<ParticlePrimitive>,	// The rendering primitive
			FloatType,							// Default particle radius
			VersionedDataObjectRef,				// Radius property + revision number
			VersionedDataObjectRef				// Type property + revision number
		>;
		bool& radiiUpToDate = dataset()->visCache().get<bool>(RadiiCacheKey(
			particlePrimitive, 
			defaultParticleRadius(),
			radiusProperty,
			typeProperty));

		// Make sure that the particle radii stored in the rendering primitive are up to date.
		if(!radiiUpToDate) {
			radiiUpToDate = true;

			// Fill in radius data.
			if(radiusProperty && radiusProperty->size() == particleCount) {
				// Allocate memory buffer.
				std::vector<FloatType> particleRadii(particleCount);
				FloatType defaultRadius = defaultParticleRadius();
				std::transform(radiusProperty->constDataFloat(), radiusProperty->constDataFloat() + particleCount, 
					particleRadii.begin(), [defaultRadius](FloatType r) { return r > 0 ? r : defaultRadius; } );
				particlePrimitive->setParticleRadii(particleRadii.data());
			}
			else if(typeProperty && typeProperty->size() == particleCount) {
				// Assign radii based on particle types.
				// Build a lookup map for particle type radii.
				const std::map<int,FloatType> radiusMap = ParticleType::typeRadiusMap(typeProperty);
				// Skip the following loop if all per-type radii are zero. In this case, simply use the default radius for all particles.
				if(std::any_of(radiusMap.cbegin(), radiusMap.cend(), [](const std::pair<int,FloatType>& it) { return it.second != 0; })) {
					// Allocate memory buffer.
					std::vector<FloatType> particleRadii(particleCount, defaultParticleRadius());
					// Fill radius array.
					const int* t = typeProperty->constDataInt();
					for(auto c = particleRadii.begin(); c != particleRadii.end(); ++c, ++t) {
						auto it = radiusMap.find(*t);
						// Set particle radius only if the type's radius is non-zero.
						if(it != radiusMap.end() && it->second != 0)
							*c = it->second;
					}
					particlePrimitive->setParticleRadii(particleRadii.data());
				}
				else {
					// Assign a constant radius to all particles.
					particlePrimitive->setParticleRadius(defaultParticleRadius());
				}
			}
			else {
				// Assign a constant radius to all particles.
				particlePrimitive->setParticleRadius(defaultParticleRadius());
			}
		}

		// The key type used for caching the particle colors:
		using ColorCacheKey = std::tuple<
			std::shared_ptr<ParticlePrimitive>,	// The rendering primitive
			VersionedDataObjectRef,		// Type property + revision number
			VersionedDataObjectRef,		// Color property + revision number
			VersionedDataObjectRef,		// Selection property + revision number
			VersionedDataObjectRef		// Transparency property + revision number
		>;
		bool& colorsUpToDate = dataset()->visCache().get<bool>(ColorCacheKey(
			particlePrimitive, 
			typeProperty,
			colorProperty,
			selectionProperty,
			transparencyProperty));

		// Make sure that the particle colors stored in the rendering primitive are up to date.
		if(!colorsUpToDate) {
			colorsUpToDate = true;

			// Fill in color data.			
			if(colorProperty && !selectionProperty && !transparencyProperty && colorProperty->size() == particleCount) {
				// Direct particle colors.
				particlePrimitive->setParticleColors(colorProperty->constDataColor());
			}
			else {
				std::vector<Color> colors(particleCount);
				particleColors(colors, colorProperty, typeProperty, selectionProperty);
				if(!transparencyProperty || transparencyProperty->size() != particleCount) {
					particlePrimitive->setParticleColors(colors.data());
				}
				else {
					// Add alpha channel based on transparency particle property.
					std::vector<ColorA> colorsWithAlpha(particleCount);
					const FloatType* t = transparencyProperty->constDataFloat();
					auto c_in = colors.cbegin();
					for(auto c_out = colorsWithAlpha.begin(); c_out != colorsWithAlpha.end(); ++c_out, ++c_in, ++t) {
						c_out->r() = c_in->r();
						c_out->g() = c_in->g();
						c_out->b() = c_in->b();
						c_out->a() = FloatType(1) - (*t);
					}
					particlePrimitive->setParticleColors(colorsWithAlpha.data());
				}
			}
		}
			
		if(renderer->isPicking()) {
			OORef<ParticlePickInfo> pickInfo(new ParticlePickInfo(this, flowState, particleCount));
			renderer->beginPickObject(contextNode, pickInfo);
		}

		particlePrimitive->render(renderer);

		if(renderer->isPicking()) {
			renderer->endPickObject();
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
			VersionedDataObjectRef,		// Shape property + revision number
			VersionedDataObjectRef,		// Orientation property + revision number
			FloatType					// Default particle radius
		>;

		// The key type used for caching the rendering primitives:
		using CylindersCacheKey = std::tuple<
			CompatibleRendererGroup,			// The scene renderer
			std::shared_ptr<ParticlePrimitive>,	// The sphere rendering primitive
			VersionedDataObjectRef,		// Position property + revision number
			VersionedDataObjectRef,		// Type property + revision number
			VersionedDataObjectRef,		// Selection property + revision number
			VersionedDataObjectRef,		// Color property + revision number
			VersionedDataObjectRef,		// Shape property + revision number
			VersionedDataObjectRef,		// Orientation property + revision number
			FloatType					// Default particle radius
		>;

		std::shared_ptr<ParticlePrimitive> spheresPrimitive;
		if(particleShape() == Spherocylinder) {

			// Look up the rendering primitive for the spheres in the vis cache.
			auto& cachedSpheresPrimitive = dataset()->visCache().get<std::shared_ptr<ParticlePrimitive>>(SpherocylinderCacheKey(
				renderer, 
				positionProperty, 
				typeProperty, 
				selectionProperty,
				colorProperty, 
				shapeProperty, 
				orientationProperty,
				defaultParticleRadius()));
			// Check if we already have a valid rendering primitive for the spheres that is up to date.
			if(!cachedSpheresPrimitive
					|| !cachedSpheresPrimitive->isValid(renderer) 
					|| (cachedSpheresPrimitive->particleCount() != particleCount * 2)) {

				// Recreate the rendering primitive for the spheres.
				cachedSpheresPrimitive = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::HighQuality, ParticlePrimitive::SphericalShape, false);
				cachedSpheresPrimitive->setSize(particleCount * 2);
			}
			spheresPrimitive = cachedSpheresPrimitive;
		}

		// Look up the rendering primitive for the cylinders in the vis cache.
		auto& cylinderPrimitive = dataset()->visCache().get<std::shared_ptr<ArrowPrimitive>>(CylindersCacheKey(
			renderer, 
			spheresPrimitive,
			positionProperty, 
			typeProperty, 
			selectionProperty,
			colorProperty, 
			shapeProperty, 
			orientationProperty,
			defaultParticleRadius()));
		// Check if we already have a valid rendering primitive for the cylinders that is up to date.
		if(!cylinderPrimitive 
				|| !cylinderPrimitive->isValid(renderer) 
				|| cylinderPrimitive->elementCount() != particleCount
				|| !cylinderPrimitive->setShadingMode(ArrowPrimitive::NormalShading)
				|| !cylinderPrimitive->setRenderingQuality(ArrowPrimitive::HighQuality)
				|| cylinderPrimitive->shape() != ArrowPrimitive::CylinderShape) {

			// Recreate the rendering primitive for the cylinders.
			cylinderPrimitive = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, ArrowPrimitive::NormalShading, ArrowPrimitive::HighQuality);

			// Determine cylinder colors.
			std::vector<Color> colors(particleCount);
			particleColors(colors, colorProperty, typeProperty, selectionProperty);

			std::vector<Point3> sphereCapPositions;
			std::vector<FloatType> sphereRadii;
			std::vector<Color> sphereColors;
			if(spheresPrimitive) {
				sphereCapPositions.resize(particleCount * 2);
				sphereRadii.resize(particleCount * 2);
				sphereColors.resize(particleCount * 2);
			}

			// Fill cylinder buffer.
			cylinderPrimitive->startSetElements(particleCount);
			for(int index = 0; index < particleCount; index++) {
				const Point3& center = positionProperty->getPoint3(index);
				FloatType radius, length;
				if(shapeProperty) {
					radius = std::abs(shapeProperty->getVector3(index).x());
					length = shapeProperty->getVector3(index).z();
				}
				else {
					radius = defaultParticleRadius();
					length = radius * 2;
				}
				Vector3 dir = Vector3(0, 0, length);
				if(orientationProperty) {
					const Quaternion& q = orientationProperty->getQuaternion(index);
					dir = q * dir;
				}
				Point3 p = center - (dir * FloatType(0.5));
				if(spheresPrimitive) {
					sphereCapPositions[index*2] = p;
					sphereCapPositions[index*2+1] = p + dir;
					sphereRadii[index*2] = sphereRadii[index*2+1] = radius;
					sphereColors[index*2] = sphereColors[index*2+1] = colors[index];
				}
				cylinderPrimitive->setElement(index, p, dir, (ColorA)colors[index], radius);
			}
			cylinderPrimitive->endSetElements();

			// Fill geometry buffer for spherical caps of spherocylinders.
			if(spheresPrimitive) {
				spheresPrimitive->setSize(particleCount * 2);
				spheresPrimitive->setParticlePositions(sphereCapPositions.data());
				spheresPrimitive->setParticleRadii(sphereRadii.data());
				spheresPrimitive->setParticleColors(sphereColors.data());
			}
		}

		if(renderer->isPicking()) {
			OORef<ParticlePickInfo> pickInfo(new ParticlePickInfo(this, flowState, particleCount));
			renderer->beginPickObject(contextNode, pickInfo);
		}
		cylinderPrimitive->render(renderer);
		if(spheresPrimitive)
			spheresPrimitive->render(renderer);
		if(renderer->isPicking()) {
			renderer->endPickObject();
		}
	}
}

/******************************************************************************
* Render a marker around a particle to highlight it in the viewports.
******************************************************************************/
void ParticlesVis::highlightParticle(size_t particleIndex, const PipelineFlowState& flowState, SceneRenderer* renderer)
{
	if(renderer->isBoundingBoxPass()) {
		renderer->addToLocalBoundingBox(highlightParticleBoundingBox(particleIndex, flowState, renderer->worldTransform(), renderer->viewport()));
		return;
	}

	// Fetch properties of selected particle which are needed to render the overlay.
	ParticleProperty* posProperty = nullptr;
	ParticleProperty* radiusProperty = nullptr;
	ParticleProperty* colorProperty = nullptr;
	ParticleProperty* selectionProperty = nullptr;
	ParticleProperty* transparencyProperty = nullptr;
	ParticleProperty* shapeProperty = nullptr;
	ParticleProperty* orientationProperty = nullptr;
	ParticleProperty* typeProperty = nullptr;
	for(DataObject* dataObj : flowState.objects()) {
		ParticleProperty* property = dynamic_object_cast<ParticleProperty>(dataObj);
		if(!property) continue;
		if(property->type() == ParticleProperty::PositionProperty && property->size() >= particleIndex)
			posProperty = property;
		else if(property->type() == ParticleProperty::RadiusProperty && property->size() >= particleIndex)
			radiusProperty = property;
		else if(property->type() == ParticleProperty::TypeProperty && property->size() >= particleIndex)
			typeProperty = property;
		else if(property->type() == ParticleProperty::ColorProperty && property->size() >= particleIndex)
			colorProperty = property;
		else if(property->type() == ParticleProperty::SelectionProperty && property->size() >= particleIndex)
			selectionProperty = property;
		else if(property->type() == ParticleProperty::TransparencyProperty && property->size() >= particleIndex)
			transparencyProperty = property;
		else if(property->type() == ParticleProperty::AsphericalShapeProperty && property->size() >= particleIndex)
			shapeProperty = property;
		else if(property->type() == ParticleProperty::OrientationProperty && property->size() >= particleIndex)
			orientationProperty = property;
	}
	if(!posProperty || particleIndex >= posProperty->size())
		return;

	// Determine position of selected particle.
	Point3 pos = posProperty->getPoint3(particleIndex);

	// Determine radius of selected particle.
	FloatType radius = particleRadius(particleIndex, radiusProperty, typeProperty);

	// Determine the display color of selected particle.
	ColorA color = particleColor(particleIndex, colorProperty, typeProperty, selectionProperty, transparencyProperty);
	ColorA highlightColor = selectionParticleColor();
	color = color * FloatType(0.5) + highlightColor * FloatType(0.5);

	// Determine rendering quality used to render the particles.
	ParticlePrimitive::RenderingQuality renderQuality = effectiveRenderingQuality(renderer, posProperty);

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
			particleBuffer->setParticleShapes(shapeProperty->constDataVector3() + particleIndex);
		if(orientationProperty)
			particleBuffer->setParticleOrientations(orientationProperty->constDataQuaternion() + particleIndex);

		// Prepare marker geometry buffer.
		highlightParticleBuffer = renderer->createParticlePrimitive(primitiveShadingMode, renderQuality, primitiveParticleShape, false);
		highlightParticleBuffer->setSize(1);
		highlightParticleBuffer->setParticleColor(highlightColor);
		highlightParticleBuffer->setParticlePositions(&pos);
		highlightParticleBuffer->setParticleRadius(radius + renderer->viewport()->nonScalingSize(renderer->worldTransform() * pos) * FloatType(1e-1));
		if(shapeProperty) {
			Vector3 shape = shapeProperty->getVector3(particleIndex);
			shape += Vector3(renderer->viewport()->nonScalingSize(renderer->worldTransform() * pos) * FloatType(1e-1));
			highlightParticleBuffer->setParticleShapes(&shape);
		}
		if(orientationProperty)
			highlightParticleBuffer->setParticleOrientations(orientationProperty->constDataQuaternion() + particleIndex);
	}
	else {
		FloatType radius, length;
		if(shapeProperty) {
			radius = std::abs(shapeProperty->getVector3(particleIndex).x());
			length = shapeProperty->getVector3(particleIndex).z();
		}
		else {
			radius = defaultParticleRadius();
			length = radius * 2;
		}
		Vector3 dir = Vector3(0, 0, length);
		if(orientationProperty) {
			const Quaternion& q = orientationProperty->getQuaternion(particleIndex);
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

/******************************************************************************
* Compute the (local) bounding box of the marker around a particle used to 
* highlight it in the viewports.
******************************************************************************/
Box3 ParticlesVis::highlightParticleBoundingBox(size_t particleIndex, const PipelineFlowState& flowState, const AffineTransformation& tm, Viewport* viewport)
{
	// Fetch properties of selected particle needed to compute the bounding box.
	ParticleProperty* posProperty = nullptr;
	ParticleProperty* radiusProperty = nullptr;
	ParticleProperty* shapeProperty = nullptr;
	ParticleProperty* typeProperty = nullptr;
	for(DataObject* dataObj : flowState.objects()) {
		ParticleProperty* property = dynamic_object_cast<ParticleProperty>(dataObj);
		if(!property) continue;
		if(property->type() == ParticleProperty::PositionProperty && property->size() >= particleIndex)
			posProperty = property;
		else if(property->type() == ParticleProperty::RadiusProperty && property->size() >= particleIndex)
			radiusProperty = property;
		else if(property->type() == ParticleProperty::AsphericalShapeProperty && property->size() >= particleIndex)
			shapeProperty = property;
		else if(property->type() == ParticleProperty::TypeProperty && property->size() >= particleIndex)
			typeProperty = property;
	}
	if(!posProperty)
		return Box3();

	// Determine position of selected particle.
	Point3 pos = posProperty->getPoint3(particleIndex);

	// Determine radius of selected particle.
	FloatType radius = particleRadius(particleIndex, radiusProperty, typeProperty);
	if(shapeProperty) {
		radius = std::max(radius, shapeProperty->getVector3(particleIndex).x());
		radius = std::max(radius, shapeProperty->getVector3(particleIndex).y());
		radius = std::max(radius, shapeProperty->getVector3(particleIndex).z());
		radius *= 2;
	}

	if(radius <= 0 || !viewport)
		return Box3();

	return Box3(pos, radius + viewport->nonScalingSize(tm * pos) * FloatType(1e-1));
}

/******************************************************************************
* Given an sub-object ID returned by the Viewport::pick() method, looks up the
* corresponding particle index.
******************************************************************************/
qlonglong ParticlePickInfo::particleIndexFromSubObjectID(quint32 subobjID) const
{
	if(_visElement->particleShape() != ParticlesVis::Cylinder
			&& _visElement->particleShape() != ParticlesVis::Spherocylinder) {
		return subobjID;
	}
	else {
		if(subobjID < (quint32)_particleCount)
			return subobjID;
		else
			return (subobjID - _particleCount) / 2;
	}
}

/******************************************************************************
* Returns a human-readable string describing the picked object,
* which will be displayed in the status bar by OVITO.
******************************************************************************/
QString ParticlePickInfo::infoString(PipelineSceneNode* objectNode, quint32 subobjectId)
{
	qlonglong particleIndex = particleIndexFromSubObjectID(subobjectId);
	if(particleIndex < 0) return QString();
	return particleInfoString(pipelineState(), particleIndex);
}

/******************************************************************************
* Builds the info string for a particle to be displayed in the status bar.
******************************************************************************/
QString ParticlePickInfo::particleInfoString(const PipelineFlowState& pipelineState, size_t particleIndex)
{
	QString str;
	for(DataObject* dataObj : pipelineState.objects()) {
		ParticleProperty* property = dynamic_object_cast<ParticleProperty>(dataObj);
		if(!property || property->size() <= particleIndex) continue;
		if(property->type() == ParticleProperty::SelectionProperty) continue;
		if(property->type() == ParticleProperty::ColorProperty) continue;
		if(property->dataType() != PropertyStorage::Int && property->dataType() != PropertyStorage::Int64 && property->dataType() != PropertyStorage::Float) continue;
		if(!str.isEmpty()) str += QStringLiteral(" | ");
		str += property->name();
		str += QStringLiteral(" ");
		for(size_t component = 0; component < property->componentCount(); component++) {
			if(component != 0) str += QStringLiteral(", ");
			if(property->dataType() == PropertyStorage::Int) {
				str += QString::number(property->getIntComponent(particleIndex, component));
				if(property->elementTypes().empty() == false) {
					if(ElementType* ptype = property->elementType(property->getIntComponent(particleIndex, component)))
						str += QString(" (%1)").arg(ptype->name());
				}
			}
			else if(property->dataType() == PropertyStorage::Int64) {
				str += QString::number(property->getInt64Component(particleIndex, component));
			}
			else if(property->dataType() == PropertyStorage::Float) {
				str += QString::number(property->getFloatComponent(particleIndex, component));
			}
		}
	}
	return str;
}

}	// End of namespace
}	// End of namespace
