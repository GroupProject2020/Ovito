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


#include <plugins/particles/Particles.h>
#include <core/dataset/data/DataVis.h>
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

	/// Constructor.
	Q_INVOKABLE ParticlesVis(DataSet* dataset);

	/// Renders the visual element.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode) override;

	/// Computes the bounding box of the visual element.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// Returns the default display color for particles.
	Color defaultParticleColor() const { return Color(1,1,1); }

	/// Returns the display color used for selected particles.
	Color selectionParticleColor() const { return Color(1,0,0); }

	/// Returns the actual particle shape used to render the particles.
	ParticlePrimitive::ParticleShape effectiveParticleShape(ParticleProperty* shapeProperty, ParticleProperty* orientationProperty) const;

	/// Returns the actual rendering quality used to render the particles.
	ParticlePrimitive::RenderingQuality effectiveRenderingQuality(SceneRenderer* renderer, ParticleProperty* positionProperty) const;

	/// Determines the display particle colors.
	void particleColors(std::vector<Color>& output, ParticleProperty* colorProperty, ParticleProperty* typeProperty, ParticleProperty* selectionProperty = nullptr);

	/// Determines the display particle radii.
	void particleRadii(std::vector<FloatType>& output, ParticleProperty* radiusProperty, ParticleProperty* typeProperty);

	/// Determines the display radius of a single particle.
	FloatType particleRadius(size_t particleIndex, ParticleProperty* radiusProperty, ParticleProperty* typeProperty);

	/// s the display color of a single particle.
	ColorA particleColor(size_t particleIndex, ParticleProperty* colorProperty, ParticleProperty* typeProperty, ParticleProperty* selectionProperty, ParticleProperty* transparencyProperty);

	/// Computes the bounding box of the particles.
	Box3 particleBoundingBox(ParticleProperty* positionProperty, ParticleProperty* typeProperty, ParticleProperty* radiusProperty, ParticleProperty* shapeProperty, bool includeParticleRadius);

	/// Render a marker around a particle to highlight it in the viewports.
	void highlightParticle(size_t particleIndex, const PipelineFlowState& flowState, SceneRenderer* renderer);

	/// Compute the (local) bounding box of the marker around a particle used to highlight it in the viewports.
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


