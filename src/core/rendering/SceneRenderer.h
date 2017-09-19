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

/**
 * \file SceneRenderer.h
 * \brief Contains the definition of the Ovito::SceneRenderer class.
 */

#pragma once


#include <core/Core.h>
#include <core/dataset/animation/TimeInterval.h>
#include <core/oo/RefTarget.h>
#include <core/viewport/Viewport.h>
#include "LinePrimitive.h"
#include "ParticlePrimitive.h"
#include "TextPrimitive.h"
#include "ImagePrimitive.h"
#include "ArrowPrimitive.h"
#include "MeshPrimitive.h"
#include "MarkerPrimitive.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/**
 * Abstract base class for object-specific information used in the picking system.
 */
class OVITO_CORE_EXPORT ObjectPickInfo : public OvitoObject
{
	Q_OBJECT
	OVITO_CLASS(ObjectPickInfo)

protected:

	/// Constructor
	ObjectPickInfo() {}

public:

	/// Returns a human-readable string describing the picked object, which will be displayed in the status bar by OVITO.
	virtual QString infoString(ObjectNode* objectNode, quint32 subobjectId) { return QString(); }
};

/**
 * Abstract base class for scene renderers, which produce a picture of the three-dimensional scene.
 */
class OVITO_CORE_EXPORT SceneRenderer : public RefTarget
{
	OVITO_CLASS(SceneRenderer)
	Q_OBJECT

public:

	enum StereoRenderingTask {
		NonStereoscopic,
		StereoscopicLeft,
		StereoscopicRight
	};

public:

	/// Prepares the renderer for rendering and sets the data set to be rendered.
	virtual bool startRender(DataSet* dataset, RenderSettings* settings) {
		OVITO_ASSERT_MSG(_renderDataset == nullptr, "SceneRenderer::startRender()", "startRender() called again without calling endRender() first.");
		_renderDataset = dataset;
		_settings = settings;
		return true;
	}

	/// Returns the dataset being rendered.
	/// This information is only available between calls to startRender() and endRender().
	DataSet* renderDataset() const {
		OVITO_CHECK_POINTER(_renderDataset); // Make sure startRender() has been called to set the data set.
		return _renderDataset;
	}

	/// Returns the general rendering settings.
	/// This information is only available between calls to startRender() and endRender().
	RenderSettings* renderSettings() const { OVITO_CHECK_POINTER(_settings); return _settings; }

	/// Is called after rendering has finished.
	virtual void endRender() {
		_renderDataset = nullptr;
		_settings = nullptr;
	}

	/// Returns the view projection parameters.
	const ViewProjectionParameters& projParams() const { return _projParams; }

	/// Changes the view projection parameters.
	void setProjParams(const ViewProjectionParameters& params) { _projParams = params; }

	/// Returns the animation time being rendered.
	TimePoint time() const { return _time; }

	/// Returns the viewport whose contents are currently being rendered.
	/// This may be NULL.
	Viewport* viewport() const { return _viewport; }

	/// Returns the final size of the rendered image in pixels.
	virtual QSize outputSize() const;

	/// \brief Computes the bounding box of the entire scene to be rendered.
	/// \param time The time at which the bounding box should be computed.
	/// \return An axis-aligned box in the world coordinate system that contains
	///         everything to be rendered.
	Box3 computeSceneBoundingBox(TimePoint time, const ViewProjectionParameters& params, Viewport* vp, const PromiseBase& promise);

	/// This method is called just before renderFrame() is called.
	/// Sets the view projection parameters, the animation frame to render,
	/// and the viewport who is being rendered.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp) {
		_time = time;
		_viewport = vp;
		setProjParams(params);
	}

	/// Renders the current animation frame.
	/// Returns false if the operation has been canceled by the user.
	virtual bool renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, const PromiseBase& promise) = 0;

	/// This method is called after renderFrame() has been called.
	virtual void endFrame(bool renderSuccessful) {}

	/// Changes the current local-to-world transformation matrix.
	virtual void setWorldTransform(const AffineTransformation& tm) = 0;

	/// Returns the current local-to-world transformation matrix.
	virtual const AffineTransformation& worldTransform() const = 0;

	/// Requests a new line geometry buffer from the renderer.
	virtual std::shared_ptr<LinePrimitive> createLinePrimitive() = 0;

	/// Requests a new particle geometry buffer from the renderer.
	virtual std::shared_ptr<ParticlePrimitive> createParticlePrimitive(ParticlePrimitive::ShadingMode shadingMode = ParticlePrimitive::NormalShading,
			ParticlePrimitive::RenderingQuality renderingQuality = ParticlePrimitive::MediumQuality,
			ParticlePrimitive::ParticleShape shape = ParticlePrimitive::SphericalShape,
			bool translucentParticles = false) = 0;

	/// Requests a new marker geometry buffer from the renderer.
	virtual std::shared_ptr<MarkerPrimitive> createMarkerPrimitive(MarkerPrimitive::MarkerShape shape) = 0;

	/// Requests a new text geometry buffer from the renderer.
	virtual std::shared_ptr<TextPrimitive> createTextPrimitive() = 0;

	/// Requests a new image geometry buffer from the renderer.
	virtual std::shared_ptr<ImagePrimitive> createImagePrimitive() = 0;

	/// Requests a new arrow geometry buffer from the renderer.
	virtual std::shared_ptr<ArrowPrimitive> createArrowPrimitive(ArrowPrimitive::Shape shape,
			ArrowPrimitive::ShadingMode shadingMode = ArrowPrimitive::NormalShading,
			ArrowPrimitive::RenderingQuality renderingQuality = ArrowPrimitive::MediumQuality) = 0;

	/// Requests a new triangle mesh geometry buffer from the renderer.
	virtual std::shared_ptr<MeshPrimitive> createMeshPrimitive() = 0;

	/// Returns whether this renderer is rendering an interactive viewport.
	/// \return true if rendering a real-time viewport; false if rendering an output image.
	/// The default implementation returns false.
	virtual bool isInteractive() const { return false; }

	/// Returns whether object picking mode is active.
	bool isPicking() const { return _isPicking; }

	/// Returns whether bounding box calculation pass is active.
	bool isBoundingBoxPass() const { return _isBoundingBoxPass; }

	/// Adds a bounding box given in local coordinates to the global bounding box.
	/// This method must be called during the bounding box render pass.
	void addToLocalBoundingBox(const Box3& bb) {
		_sceneBoundingBox.addBox(bb.transformed(worldTransform()));
	}

	/// Adds a point given in local coordinates to the global bounding box.
	/// This method must be called during the bounding box render pass.
	void addToLocalBoundingBox(const Point3& p) {
		_sceneBoundingBox.addPoint(worldTransform() * p);
	}

	/// When picking mode is active, this registers an object being rendered.
	virtual quint32 beginPickObject(ObjectNode* objNode, ObjectPickInfo* pickInfo = nullptr) { return 0; }

	/// Call this when rendering of a pickable object is finished.
	virtual void endPickObject() {}

	/// Returns the line rendering width to use in object picking mode.
	virtual FloatType defaultLinePickingWidth() { return 1; }

	/// Temporarily enables/disables the depth test while rendering.
	/// This method is mainly used with the interactive viewport renderer.
	virtual void setDepthTestEnabled(bool enabled) {}

	/// Activates the special highlight rendering mode.
	/// This method is mainly used with the interactive viewport renderer.
	virtual void setHighlightMode(int pass) {}

protected:

	/// Constructor.
	SceneRenderer(DataSet* dataset);

	/// \brief Renders all nodes in the scene.
	virtual bool renderScene(const PromiseBase& promise);

	/// \brief Render a scene node (and all its children).
	virtual bool renderNode(SceneNode* node, const PromiseBase& promise);

	/// \brief This virtual method is responsible for rendering additional content that is only
	///       visible in the interactive viewports.
	virtual void renderInteractiveContent() {}

	/// \brief Renders the visual representation of the modifiers.
	void renderModifiers(bool renderOverlay);

	/// \brief Renders the visual representation of the modifiers.
	void renderModifiers(ObjectNode* objNode, bool renderOverlay);

	/// \brief Gets the trajectory of motion of a node.
	std::vector<Point3> getNodeTrajectory(SceneNode* node);

	/// \brief Renders the trajectory of motion of a node in the interactive viewports.
	void renderNodeTrajectory(SceneNode* node);

	/// Sets whether object picking mode is active.
	void setPicking(bool enable) { _isPicking = enable; }

private:

	/// The data set being rendered.
	DataSet* _renderDataset = nullptr;

	/// The current render settings.
	RenderSettings* _settings = nullptr;

	/// The viewport whose contents are currently being rendered.
	Viewport* _viewport = nullptr;

	/// The view projection parameters.
	ViewProjectionParameters _projParams;

	/// The animation time being rendered.
	TimePoint _time;

	/// Indicates that an object picking pass is active.
	bool _isPicking = false;

	/// Indicates that a bounding box pass is active.
	bool _isBoundingBoxPass = false;

	/// Working variable used for computing the bounding box of the entire scene.
	Box3 _sceneBoundingBox;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


