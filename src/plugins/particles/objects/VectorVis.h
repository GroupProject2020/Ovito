///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include <core/rendering/ArrowPrimitive.h>
#include <core/rendering/SceneRenderer.h>
#include "ParticleProperty.h"

namespace Ovito { namespace Particles {

/**
 * \brief A visualization element for rendering per-particle vector arrows.
 */
class OVITO_PARTICLES_EXPORT VectorVis : public DataVis
{
	Q_OBJECT
	OVITO_CLASS(VectorVis)
	Q_CLASSINFO("DisplayName", "Vectors");
	
public:

	/// The position mode for the arrows.
	enum ArrowPosition {
		Base,
		Center,
		Head
	};
	Q_ENUMS(ArrowPosition);

public:

	/// \brief Constructor.
	Q_INVOKABLE VectorVis(DataSet* dataset);

	/// \brief Lets the visualization element render the data object.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

public:

    Q_PROPERTY(Ovito::ArrowPrimitive::ShadingMode shadingMode READ shadingMode WRITE setShadingMode);
    Q_PROPERTY(Ovito::ArrowPrimitive::RenderingQuality renderingQuality READ renderingQuality WRITE setRenderingQuality);

protected:

	/// Computes the bounding box of the arrows.
	Box3 arrowBoundingBox(ParticleProperty* vectorProperty, ParticleProperty* positionProperty);

protected:

	/// Reverses of the arrow pointing direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, reverseArrowDirection, setReverseArrowDirection);

	/// Controls how the arrows are positioned relative to the particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(ArrowPosition, arrowPosition, setArrowPosition, PROPERTY_FIELD_MEMORIZE);

	/// Controls the color of the arrows.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Color, arrowColor, setArrowColor, PROPERTY_FIELD_MEMORIZE);

	/// Controls the width of the arrows in world units.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, arrowWidth, setArrowWidth, PROPERTY_FIELD_MEMORIZE);

	/// Controls the scaling factor applied to the vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, scalingFactor, setScalingFactor, PROPERTY_FIELD_MEMORIZE);

	/// Controls the shading mode for arrows.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(ArrowPrimitive::ShadingMode, shadingMode, setShadingMode, PROPERTY_FIELD_MEMORIZE);

	/// Controls the rendering quality mode for arrows.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ArrowPrimitive::RenderingQuality, renderingQuality, setRenderingQuality);

	/// The buffered geometry used to render the arrows.
	std::shared_ptr<ArrowPrimitive> _buffer;

	/// This helper structure is used to detect any changes in the input data
	/// that require updating the geometry buffer.
	CacheStateHelper<
		VersionedDataObjectRef,			// Vector property + revision number
		VersionedDataObjectRef,			// Particle position property + revision number
		FloatType,						// Scaling factor
		FloatType,						// Arrow width
		Color,							// Arrow color
		bool,							// Reverse arrow direction
		ArrowPosition,					// Arrow position
		VersionedDataObjectRef			// Vector color property + revision number
		> _geometryCacheHelper;

	/// The bounding box that includes all arrows.
	Box3 _cachedBoundingBox;

	/// This helper structure is used to detect changes in the input
	/// that require recalculating the bounding box.
	CacheStateHelper<
		VersionedDataObjectRef,		// Vector property + revision number
		VersionedDataObjectRef,		// Particle position property + revision number
		FloatType,					// Scaling factor
		FloatType					// Arrow width
		> _boundingBoxCacheHelper;
};

/**
 * \brief This information record is attached to the arrows by the VectorVis when rendering
 * them in the viewports. It facilitates the picking of arrows with the mouse.
 */
class OVITO_PARTICLES_EXPORT VectorPickInfo : public ObjectPickInfo
{
	Q_OBJECT
	OVITO_CLASS(VectorPickInfo)
	
public:

	/// Constructor.
	VectorPickInfo(VectorVis* visElement, const PipelineFlowState& pipelineState, ParticleProperty* vectorProperty) :
		_visElement(visElement), _pipelineState(pipelineState), _vectorProperty(vectorProperty) {}

	/// The pipeline flow state containing the particle properties.
	const PipelineFlowState& pipelineState() const { return _pipelineState; }

	/// Returns a human-readable string describing the picked object, which will be displayed in the status bar by OVITO.
	virtual QString infoString(PipelineSceneNode* objectNode, quint32 subobjectId) override;

	/// Given an sub-object ID returned by the Viewport::pick() method, looks up the
	/// corresponding particle index.
	int particleIndexFromSubObjectID(quint32 subobjID) const;

private:

	/// The pipeline flow state containing the particle properties.
	PipelineFlowState _pipelineState;

	/// The vis element that rendered the arrows.
	OORef<VectorVis> _visElement;

	/// The vector property.
	OORef<ParticleProperty> _vectorProperty;
};


}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::VectorVis::ArrowPosition);
Q_DECLARE_TYPEINFO(Ovito::Particles::VectorVis::ArrowPosition, Q_PRIMITIVE_TYPE);
