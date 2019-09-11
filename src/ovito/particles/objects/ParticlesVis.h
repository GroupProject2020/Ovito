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

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/ParticlePrimitive.h>
#include <ovito/core/dataset/data/DataVis.h>

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

	/// Constructor.
	Q_INVOKABLE ParticlesVis(DataSet* dataset);

	/// Renders the visual element.
	virtual void render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode) override;

	/// Computes the bounding box of the visual element.
	virtual Box3 boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// Returns the default display color for particles.
	Color defaultParticleColor() const { return Color(1,1,1); }

	/// Returns the display color used for selected particles.
	Color selectionParticleColor() const { return Color(1,0,0); }

	/// Returns the actual particle shape used to render the particles.
	ParticlePrimitive::ParticleShape effectiveParticleShape(const PropertyObject* shapeProperty, const PropertyObject* orientationProperty) const;

	/// Returns the actual rendering quality used to render the particles.
	ParticlePrimitive::RenderingQuality effectiveRenderingQuality(SceneRenderer* renderer, const ParticlesObject* particles) const;

	/// Determines the display particle colors.
	void particleColors(std::vector<ColorA>& output, const PropertyObject* colorProperty, const PropertyObject* typeProperty, const PropertyObject* selectionProperty = nullptr, const PropertyObject* transparencyProperty = nullptr) const;

	/// Determines the display particle radii.
	void particleRadii(std::vector<FloatType>& output, const PropertyObject* radiusProperty, const PropertyObject* typeProperty) const;

	/// Determines the display radius of a single particle.
	FloatType particleRadius(size_t particleIndex, const PropertyObject* radiusProperty, const PropertyObject* typeProperty) const;

	/// s the display color of a single particle.
	ColorA particleColor(size_t particleIndex, const PropertyObject* colorProperty, const PropertyObject* typeProperty, const PropertyObject* selectionProperty, const PropertyObject* transparencyProperty) const;

	/// Computes the bounding box of the particles.
	Box3 particleBoundingBox(const PropertyObject* positionProperty, const PropertyObject* typeProperty, const PropertyObject* radiusProperty, const PropertyObject* shapeProperty, bool includeParticleRadius) const;

	/// Render a marker around a particle to highlight it in the viewports.
	void highlightParticle(size_t particleIndex, const ParticlesObject* particles, SceneRenderer* renderer) const;

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
	ParticlePickInfo(ParticlesVis* visElement, const PipelineFlowState& pipelineState, std::vector<size_t> subobjectToParticleMapping = {}) :
		_visElement(visElement), _pipelineState(pipelineState), _subobjectToParticleMapping(std::move(subobjectToParticleMapping)) {}

	/// The pipeline flow state containing the particle properties.
	const PipelineFlowState& pipelineState() const { return _pipelineState; }

	/// Returns a human-readable string describing the picked object, which will be displayed in the status bar by OVITO.
	virtual QString infoString(PipelineSceneNode* objectNode, quint32 subobjectId) override;

	/// Given an sub-object ID returned by the Viewport::pick() method, looks up the
	/// corresponding particle index.
	size_t particleIndexFromSubObjectID(quint32 subobjID) const;

	/// Builds the info string for a particle to be displayed in the status bar.
	static QString particleInfoString(const PipelineFlowState& pipelineState, size_t particleIndex);

private:

	/// The pipeline flow state containing the particle properties.
	PipelineFlowState _pipelineState;

	/// The vis element that rendered the particles.
	OORef<ParticlesVis> _visElement;

	/// Stores the index of the particle that is associated with a rendering primitive sub-object ID.
	std::vector<size_t> _subobjectToParticleMapping;
};

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::ParticlesVis::ParticleShape);
Q_DECLARE_TYPEINFO(Ovito::Particles::ParticlesVis::ParticleShape, Q_PRIMITIVE_TYPE);
