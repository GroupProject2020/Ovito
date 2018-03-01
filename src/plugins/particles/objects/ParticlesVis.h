///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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

#pragma once


#include <plugins/particles/Particles.h>
#include <core/dataset/data/DataVis.h>
#include <core/dataset/data/VersionedDataObjectRef.h>
#include <core/dataset/data/CacheStateHelper.h>
#include <core/rendering/ParticlePrimitive.h>
#include <core/rendering/ArrowPrimitive.h>
#include <core/rendering/SceneRenderer.h>
#include "ParticleProperty.h"

namespace Ovito { namespace Particles {

/**
 * \brief A visualization element for rendering particles.
 */
class OVITO_PARTICLES_EXPORT ParticlesVis : public DataVis
{
	Q_OBJECT
	OVITO_CLASS(ParticlesVis)
	Q_CLASSINFO("DisplayName", "Particles");
	
public:

	/// The shapes supported by the particle vis element.
	enum ParticleShape {
		Sphere,
		Box,
		Circle,
		Square,
		Cylinder,
		Spherocylinder
	};
	Q_ENUMS(ParticleShape);

public:

	/// \brief Constructor.
	Q_INVOKABLE ParticlesVis(DataSet* dataset);

	/// \brief Lets the visualization element render the data object.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// \brief Returns the default display color for particles.
	Color defaultParticleColor() const { return Color(1,1,1); }

	/// \brief Returns the display color used for selected particles.
	Color selectionParticleColor() const { return Color(1,0,0); }

	/// \brief Returns the actual particle shape used to render the particles.
	ParticlePrimitive::ParticleShape effectiveParticleShape(ParticleProperty* shapeProperty, ParticleProperty* orientationProperty) const;

	/// \brief Returns the actual rendering quality used to render the particles.
	ParticlePrimitive::RenderingQuality effectiveRenderingQuality(SceneRenderer* renderer, ParticleProperty* positionProperty) const;

	/// \brief Determines the display particle colors.
	void particleColors(std::vector<Color>& output, ParticleProperty* colorProperty, ParticleProperty* typeProperty, ParticleProperty* selectionProperty = nullptr);

	/// \brief Determines the display particle radii.
	void particleRadii(std::vector<FloatType>& output, ParticleProperty* radiusProperty, ParticleProperty* typeProperty);

	/// \brief Determines the display radius of a single particle.
	FloatType particleRadius(size_t particleIndex, ParticleProperty* radiusProperty, ParticleProperty* typeProperty);

	/// \brief Determines the display color of a single particle.
	ColorA particleColor(size_t particleIndex, ParticleProperty* colorProperty, ParticleProperty* typeProperty, ParticleProperty* selectionProperty, ParticleProperty* transparencyProperty);

	/// \brief Computes the bounding box of the particles.
	Box3 particleBoundingBox(ParticleProperty* positionProperty, ParticleProperty* typeProperty, ParticleProperty* radiusProperty, ParticleProperty* shapeProperty, bool includeParticleRadius = true);

	/// \brief Render a marker around a particle to highlight it in the viewports.
	void highlightParticle(size_t particleIndex, const PipelineFlowState& flowState, SceneRenderer* renderer);

	/// \brief Compute the (local) bounding box of the marker around a particle used to highlight it in the viewports.
	Box3 highlightParticleBoundingBox(size_t particleIndex, const PipelineFlowState& flowState, const AffineTransformation& tm, Viewport* viewport);

public:

    Q_PROPERTY(Ovito::ParticlePrimitive::RenderingQuality renderingQuality READ renderingQuality WRITE setRenderingQuality);
    Q_PROPERTY(Ovito::Particles::ParticlesVis::ParticleShape particleShape READ particleShape WRITE setParticleShape);
	
private:

	/// Controls the default display radius of atomic particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, defaultParticleRadius, setDefaultParticleRadius, PROPERTY_FIELD_MEMORIZE);

	/// Controls the rendering quality mode for particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePrimitive::RenderingQuality, renderingQuality, setRenderingQuality);

	/// Controls the display shape of particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticleShape, particleShape, setParticleShape);

	/// The buffered particle geometry used to render the particles.
	std::shared_ptr<ParticlePrimitive> _particleBuffer;

	/// The buffered particle geometry used to render particles with cylindrical shape.
	std::shared_ptr<ArrowPrimitive> _cylinderBuffer;

	/// The buffered particle geometry used to render spherocylinder particles.
	std::shared_ptr<ParticlePrimitive> _spherocylinderBuffer;

	/// This helper structure is used to detect any changes in the particle positions
	/// that require updating the particle position buffer.
	CacheStateHelper<
		VersionedDataObjectRef
		> _positionsCacheHelper;

	/// This helper structure is used to detect any changes in the particle radii
	/// that require updating the particle radius buffer.
	CacheStateHelper<
		VersionedDataObjectRef,		// Radius property + revision number
		VersionedDataObjectRef,		// Type property + revision number
		FloatType					// Default radius
		> _radiiCacheHelper;

	/// This helper structure is used to detect any changes in the particle shapes
	/// that require updating the particle shape buffer.
	CacheStateHelper<
		VersionedDataObjectRef,		// Shape property + revision number
		VersionedDataObjectRef		// Orientation property + revision number
		> _shapesCacheHelper;

	/// This helper structure is used to detect any changes in the particle colors
	/// that require updating the particle color buffer.
	CacheStateHelper<
		VersionedDataObjectRef,		// Color property + revision number
		VersionedDataObjectRef,		// Type property + revision number
		VersionedDataObjectRef,		// Selection property + revision number
		VersionedDataObjectRef,		// Transparency property + revision number
		VersionedDataObjectRef		// Position property + revision number
		> _colorsCacheHelper;

	/// This helper structure is used to detect any changes in the particle properties
	/// that require updating the cylinder geometry buffer.
	CacheStateHelper<
		VersionedDataObjectRef,		// Position property + revision number
		VersionedDataObjectRef,		// Type property + revision number
		VersionedDataObjectRef,		// Selection property + revision number
		VersionedDataObjectRef,		// Color property + revision number
		VersionedDataObjectRef,		// Shape property + revision number
		VersionedDataObjectRef,		// Orientation property + revision number
		FloatType										// Default particle radius
		> _cylinderCacheHelper;

	/// The bounding box that includes all particles.
	Box3 _cachedBoundingBox;

	/// This helper structure is used to detect changes in the input objects
	/// that require rebuilding the bounding box.
	CacheStateHelper<
		VersionedDataObjectRef,	// Position property + revision number
		VersionedDataObjectRef,	// Radius property + revision number
		VersionedDataObjectRef,	// Type property + revision number
		VersionedDataObjectRef,	// Aspherical shape property + revision number
		FloatType> 				// Default particle radius
		_boundingBoxCacheHelper;
};

/**
 * \brief This information record is attached to the particles by the ParticlesVis when rendering
 * them in the viewports. It facilitates the picking of particles with the mouse.
 */
class OVITO_PARTICLES_EXPORT ParticlePickInfo : public ObjectPickInfo
{
	Q_OBJECT
	OVITO_CLASS(ParticlePickInfo)

public:

	/// Constructor.
	ParticlePickInfo(ParticlesVis* visElement, const PipelineFlowState& pipelineState, int particleCount) :
		_visElement(visElement), _pipelineState(pipelineState), _particleCount(particleCount) {}

	/// The pipeline flow state containing the particle properties.
	const PipelineFlowState& pipelineState() const { return _pipelineState; }

	/// Returns a human-readable string describing the picked object, which will be displayed in the status bar by OVITO.
	virtual QString infoString(PipelineSceneNode* objectNode, quint32 subobjectId) override;

	/// Given an sub-object ID returned by the Viewport::pick() method, looks up the
	/// corresponding particle index.
	qlonglong particleIndexFromSubObjectID(quint32 subobjID) const;

	/// Builds the info string for a particle to be displayed in the status bar.
	static QString particleInfoString(const PipelineFlowState& pipelineState, size_t particleIndex);

private:

	/// The pipeline flow state containing the particle properties.
	PipelineFlowState _pipelineState;

	/// The vis element that rendered the particles.
	OORef<ParticlesVis> _visElement;

	/// The number of rendered particles;
	qlonglong _particleCount;
};

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::ParticlesVis::ParticleShape);
Q_DECLARE_TYPEINFO(Ovito::Particles::ParticlesVis::ParticleShape, Q_PRIMITIVE_TYPE);


