////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/data/VersionedDataObjectRef.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/ArrowPrimitive.h>
#include "VectorVis.h"
#include "ParticlesVis.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(VectorVis);
IMPLEMENT_OVITO_CLASS(VectorPickInfo);
DEFINE_PROPERTY_FIELD(VectorVis, reverseArrowDirection);
DEFINE_PROPERTY_FIELD(VectorVis, arrowPosition);
DEFINE_PROPERTY_FIELD(VectorVis, arrowColor);
DEFINE_PROPERTY_FIELD(VectorVis, arrowWidth);
DEFINE_PROPERTY_FIELD(VectorVis, scalingFactor);
DEFINE_PROPERTY_FIELD(VectorVis, shadingMode);
DEFINE_PROPERTY_FIELD(VectorVis, renderingQuality);
SET_PROPERTY_FIELD_LABEL(VectorVis, arrowColor, "Arrow color");
SET_PROPERTY_FIELD_LABEL(VectorVis, arrowWidth, "Arrow width");
SET_PROPERTY_FIELD_LABEL(VectorVis, scalingFactor, "Scaling factor");
SET_PROPERTY_FIELD_LABEL(VectorVis, reverseArrowDirection, "Reverse direction");
SET_PROPERTY_FIELD_LABEL(VectorVis, arrowPosition, "Position");
SET_PROPERTY_FIELD_LABEL(VectorVis, shadingMode, "Shading mode");
SET_PROPERTY_FIELD_LABEL(VectorVis, renderingQuality, "RenderingQuality");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(VectorVis, arrowWidth, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(VectorVis, scalingFactor, FloatParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
VectorVis::VectorVis(DataSet* dataset) : DataVis(dataset),
	_reverseArrowDirection(false),
	_arrowPosition(Base),
	_arrowColor(1, 1, 0),
	_arrowWidth(0.5),
	_scalingFactor(1),
	_shadingMode(ArrowPrimitive::FlatShading),
	_renderingQuality(ArrowPrimitive::LowQuality)
{
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 VectorVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	if(objectStack.size() < 2) return {};
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(objectStack[objectStack.size()-2]);
	if(!particles) return {};
	const PropertyObject* vectorProperty = dynamic_object_cast<PropertyObject>(objectStack.back());
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	if(vectorProperty && (vectorProperty->dataType() != PropertyStorage::Float || vectorProperty->componentCount() != 3))
		vectorProperty = nullptr;

	// The key type used for caching the computed bounding box:
	using CacheKey = std::tuple<
		VersionedDataObjectRef,		// Vector property + revision number
		VersionedDataObjectRef,		// Particle position property + revision number
		FloatType,					// Scaling factor
		FloatType					// Arrow width
	>;

	// Look up the bounding box in the vis cache.
	auto& bbox = dataset()->visCache().get<Box3>(CacheKey(
			vectorProperty,
			positionProperty,
			scalingFactor(),
			arrowWidth()));

	// Check if the cached bounding box information is still up to date.
	if(bbox.isEmpty()) {
		// If not, recompute bounding box from particle data.
		bbox = arrowBoundingBox(vectorProperty, positionProperty);
	}
	return bbox;
}

/******************************************************************************
* Computes the bounding box of the arrows.
******************************************************************************/
Box3 VectorVis::arrowBoundingBox(const PropertyObject* vectorProperty, const PropertyObject* positionProperty) const
{
	if(!positionProperty || !vectorProperty)
		return Box3();

	OVITO_ASSERT(positionProperty->type() == ParticlesObject::PositionProperty);
	OVITO_ASSERT(vectorProperty->dataType() == PropertyStorage::Float);
	OVITO_ASSERT(vectorProperty->componentCount() == 3);

	// Compute bounding box of particle positions (only those with non-zero vector).
	Box3 bbox;
	ConstPropertyAccess<Point3> positions(positionProperty);
	ConstPropertyAccess<Vector3> vectorData(vectorProperty);
	const Point3* p = positions.cbegin();
	for(const Vector3& v : vectorData) {
		if(v != Vector3::Zero())
			bbox.addPoint(*p);
		++p;
	}

	// Find largest vector magnitude.
	FloatType maxMagnitude = 0;
	for(const Vector3& v : vectorData) {
		FloatType m = v.squaredLength();
		if(m > maxMagnitude) maxMagnitude = m;
	}

	// Enlarge the bounding box by the largest vector magnitude + padding.
	return bbox.padBox((sqrt(maxMagnitude) * std::abs(scalingFactor())) + arrowWidth());
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
void VectorVis::render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, objectStack, contextNode, flowState, validityInterval));
		return;
	}

	// Get input data.
	if(objectStack.size() < 2) return;
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(objectStack[objectStack.size()-2]);
	if(!particles) return;
	const PropertyObject* vectorProperty = dynamic_object_cast<PropertyObject>(objectStack.back());
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	if(vectorProperty && (vectorProperty->dataType() != PropertyStorage::Float || vectorProperty->componentCount() != 3))
		vectorProperty = nullptr;
	const PropertyObject* vectorColorProperty = particles->getProperty(ParticlesObject::VectorColorProperty);

	// Make sure we don't exceed our internal limits.
	if(vectorProperty && vectorProperty->size() > (size_t)std::numeric_limits<int>::max()) {
		qWarning() << "WARNING: Cannot render more than" << std::numeric_limits<int>::max() << "vector arrows.";
		return;
	}

	// The key type used for caching the rendering primitive:
	using CacheKey = std::tuple<
		CompatibleRendererGroup,	// The scene renderer
		VersionedDataObjectRef,		// Vector property + revision number
		VersionedDataObjectRef,		// Particle position property + revision number
		FloatType,					// Scaling factor
		FloatType,					// Arrow width
		Color,						// Arrow color
		bool,						// Reverse arrow direction
		ArrowPosition,				// Arrow position
		VersionedDataObjectRef		// Vector color property + revision number
	>;

	// Lookup the rendering primitive in the vis cache.
	auto& arrowPrimitive = dataset()->visCache().get<std::shared_ptr<ArrowPrimitive>>(CacheKey(
			renderer,
			vectorProperty,
			positionProperty,
			scalingFactor(),
			arrowWidth(),
			arrowColor(),
			reverseArrowDirection(),
			arrowPosition(),
			vectorColorProperty));

	// Check if we already have a valid rendering primitive that is up to date.
	if(!arrowPrimitive
			|| !arrowPrimitive->isValid(renderer)
			|| !arrowPrimitive->setShadingMode(shadingMode())
			|| !arrowPrimitive->setRenderingQuality(renderingQuality())) {

		arrowPrimitive = renderer->createArrowPrimitive(ArrowPrimitive::ArrowShape, shadingMode(), renderingQuality());

		// Determine number of non-zero vectors.
		int vectorCount = 0;
		ConstPropertyAccess<Vector3> vectorData(vectorProperty);
		if(vectorProperty && positionProperty) {
			for(const Vector3& v : vectorData) {
				if(v != Vector3::Zero())
					vectorCount++;
			}
		}

		arrowPrimitive->startSetElements(vectorCount);
		if(vectorCount) {
			FloatType scalingFac = scalingFactor();
			if(reverseArrowDirection())
				scalingFac = -scalingFac;
			ColorA color(arrowColor());
			FloatType width = arrowWidth();
			ArrowPrimitive* buffer = arrowPrimitive.get();
			ConstPropertyAccess<Point3> positions(positionProperty);
			const Point3* pos = positions.cbegin();
			ConstPropertyAccess<Color> vectorColorData(vectorColorProperty);
			const Color* pcol = vectorColorData ? vectorColorData.cbegin() : nullptr;
			int index = 0;
			for(const Vector3& vec : vectorData) {
				if(vec != Vector3::Zero()) {
					Vector3 v = vec * scalingFac;
					Point3 base = *pos;
					if(arrowPosition() == Head)
						base -= v;
					else if(arrowPosition() == Center)
						base -= v * FloatType(0.5);
					if(pcol)
						color = *pcol;
					buffer->setElement(index++, base, v, color, width);
				}
				++pos;
				if(pcol) ++pcol;
			}
			OVITO_ASSERT(pos == positions.cend());
			OVITO_ASSERT(!pcol || pcol == vectorColorData.cend());
			OVITO_ASSERT(index == vectorCount);
		}
		arrowPrimitive->endSetElements();
	}

	if(renderer->isPicking()) {
		OORef<VectorPickInfo> pickInfo(new VectorPickInfo(this, flowState, vectorProperty));
		renderer->beginPickObject(contextNode, pickInfo);
	}
	arrowPrimitive->render(renderer);
	if(renderer->isPicking()) {
		renderer->endPickObject();
	}
}

/******************************************************************************
* Given an sub-object ID returned by the Viewport::pick() method, looks up the
* corresponding particle index.
******************************************************************************/
size_t VectorPickInfo::particleIndexFromSubObjectID(quint32 subobjID) const
{
	if(_vectorProperty) {
		size_t particleIndex = 0;
		ConstPropertyAccess<Vector3> vectorData(_vectorProperty);
		for(const Vector3& v : vectorData) {
			if(v != Vector3::Zero()) {
				if(subobjID == 0) return particleIndex;
				subobjID--;
			}
			particleIndex++;
		}
	}
	return std::numeric_limits<size_t>::max();
}

/******************************************************************************
* Returns a human-readable string describing the picked object,
* which will be displayed in the status bar by OVITO.
******************************************************************************/
QString VectorPickInfo::infoString(PipelineSceneNode* objectNode, quint32 subobjectId)
{
	size_t particleIndex = particleIndexFromSubObjectID(subobjectId);
	if(particleIndex == std::numeric_limits<size_t>::max()) return QString();
	return ParticlePickInfo::particleInfoString(pipelineState(), particleIndex);
}


}	// End of namespace
}	// End of namespace
