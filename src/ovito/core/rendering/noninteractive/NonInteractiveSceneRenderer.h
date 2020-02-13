////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include "DefaultArrowPrimitive.h"
#include "DefaultImagePrimitive.h"
#include "DefaultLinePrimitive.h"
#include "DefaultParticlePrimitive.h"
#include "DefaultTextPrimitive.h"
#include "DefaultMeshPrimitive.h"
#include "DefaultMarkerPrimitive.h"

namespace Ovito {

/**
 * \brief Abstract base class for non-interactive scene renderers.
 */
class OVITO_CORE_EXPORT NonInteractiveSceneRenderer : public SceneRenderer
{
	Q_OBJECT
	OVITO_CLASS(NonInteractiveSceneRenderer)

public:

	/// Constructor.
	NonInteractiveSceneRenderer(DataSet* dataset) : SceneRenderer(dataset), _modelTM(AffineTransformation::Identity()) {}

	/// This method is called just before renderFrame() is called.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp) override;

	/// Changes the current local to world transformation matrix.
	virtual void setWorldTransform(const AffineTransformation& tm) override { _modelTM = tm; }

	/// Returns the current local-to-world transformation matrix.
	virtual const AffineTransformation& worldTransform() const override { return _modelTM; }

	/// Returns the current model-to-world transformation matrix.
	const AffineTransformation& modelTM() const { return _modelTM; }

	/// Requests a new line geometry buffer from the renderer.
	virtual std::shared_ptr<LinePrimitive> createLinePrimitive() override {
		return std::make_shared<DefaultLinePrimitive>();
	}

	/// Requests a new particle geometry buffer from the renderer.
	virtual std::shared_ptr<ParticlePrimitive> createParticlePrimitive(
			ParticlePrimitive::ShadingMode shadingMode,
			ParticlePrimitive::RenderingQuality renderingQuality,
			ParticlePrimitive::ParticleShape shape,
			bool translucentParticles) override {
		return std::make_shared<DefaultParticlePrimitive>(shadingMode, renderingQuality, shape, translucentParticles);
	}

	/// Requests a new marker geometry buffer from the renderer.
	virtual std::shared_ptr<MarkerPrimitive> createMarkerPrimitive(MarkerPrimitive::MarkerShape shape) override {
		return std::make_shared<DefaultMarkerPrimitive>(shape);
	}

	/// Requests a new text geometry buffer from the renderer.
	virtual std::shared_ptr<TextPrimitive> createTextPrimitive() override {
		return std::make_shared<DefaultTextPrimitive>();
	}

	/// Requests a new image geometry buffer from the renderer.
	virtual std::shared_ptr<ImagePrimitive> createImagePrimitive() override {
		return std::make_shared<DefaultImagePrimitive>();
	}

	/// Requests a new arrow geometry buffer from the renderer.
	virtual std::shared_ptr<ArrowPrimitive> createArrowPrimitive(
			ArrowPrimitive::Shape shape,
			ArrowPrimitive::ShadingMode shadingMode,
			ArrowPrimitive::RenderingQuality renderingQuality,
			bool translucentElements) override {
		return std::make_shared<DefaultArrowPrimitive>(shape, shadingMode, renderingQuality, translucentElements);
	}

	/// Requests a new triangle mesh buffer from the renderer.
	virtual std::shared_ptr<MeshPrimitive> createMeshPrimitive() override {
		return std::make_shared<DefaultMeshPrimitive>();
	}

	/// Renders the line geometry stored in the given buffer.
	virtual void renderLines(const DefaultLinePrimitive& lineBuffer) = 0;

	/// Renders the particles stored in the given buffer.
	virtual void renderParticles(const DefaultParticlePrimitive& particleBuffer) = 0;

	/// Renders the arrow elements stored in the given buffer.
	virtual void renderArrows(const DefaultArrowPrimitive& arrowBuffer) = 0;

	/// Renders the text stored in the given buffer.
	virtual void renderText(const DefaultTextPrimitive& textBuffer, const Point2& pos, int alignment) = 0;

	/// Renders the image stored in the given buffer.
	virtual void renderImage(const DefaultImagePrimitive& imageBuffer, const Point2& pos, const Vector2& size) = 0;

	/// Renders the triangle mesh stored in the given buffer.
	virtual void renderMesh(const DefaultMeshPrimitive& meshBuffer) = 0;

	/// Determines if this renderer can share geometry data and other resources with the given other renderer.
	virtual bool sharesResourcesWith(SceneRenderer* otherRenderer) const override;

private:

	/// The current model-to-world transformation matrix.
	AffineTransformation _modelTM;
};

}	// End of namespace
