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


#include <core/Core.h>
#include <core/rendering/noninteractive/NonInteractiveSceneRenderer.h>
#define TACHYON_INTERNAL 1
#include <tachyon/tachyon.h>

#ifdef Tachyon_EXPORTS		// This is defined by CMake when building the plugin library.
#  define OVITO_TACHYON_EXPORT Q_DECL_EXPORT
#else
#  define OVITO_TACHYON_EXPORT Q_DECL_IMPORT
#endif

namespace Ovito { namespace Tachyon {

/**
 * \brief A scene renderer that is based on the Tachyon open source ray-tracing engine
 */
class OVITO_TACHYON_EXPORT TachyonRenderer : public NonInteractiveSceneRenderer
{
	Q_OBJECT
	OVITO_CLASS(TachyonRenderer)
	Q_CLASSINFO("DisplayName", "Tachyon renderer");
	
public:

	/// Constructor.
	Q_INVOKABLE TachyonRenderer(DataSet* dataset);

	///	Prepares the renderer for rendering of the given scene.
	/// Throws an exception on error. Returns false when the operation has been aborted by the user.
	virtual bool startRender(DataSet* dataset, RenderSettings* settings) override;

	/// Renders a single animation frame into the given frame buffer.
	/// Throws an exception on error. Returns false when the operation has been aborted by the user.
	virtual bool renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, const PromiseBase& promise) override;

	///	Finishes the rendering pass. This is called after all animation frames have been rendered
	/// or when the rendering operation has been aborted.
	virtual void endRender() override;

	/// Renders the line geometry stored in the given buffer.
	virtual void renderLines(const DefaultLinePrimitive& lineBuffer) override;

	/// Renders the particles stored in the given buffer.
	virtual void renderParticles(const DefaultParticlePrimitive& particleBuffer) override;

	/// Renders the arrow elements stored in the given buffer.
	virtual void renderArrows(const DefaultArrowPrimitive& arrowBuffer) override;

	/// Renders the text stored in the given buffer.
	virtual void renderText(const DefaultTextPrimitive& textBuffer, const Point2& pos, int alignment) override;

	/// Renders the image stored in the given buffer.
	virtual void renderImage(const DefaultImagePrimitive& imageBuffer, const Point2& pos, const Vector2& size) override;

	/// Renders the triangle mesh stored in the given buffer.
	virtual void renderMesh(const DefaultMeshPrimitive& meshBuffer) override;

private:

	/// Creates a texture with the given color.
	void* getTachyonTexture(FloatType r, FloatType g, FloatType b, FloatType alpha = FloatType(1));

private:

	/// Controls anti-aliasing.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, antialiasingEnabled, setAntialiasingEnabled, PROPERTY_FIELD_MEMORIZE);

	/// Controls quality of anti-aliasing.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, antialiasingSamples, setAntialiasingSamples, PROPERTY_FIELD_MEMORIZE);

	/// Enables direct light source.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, directLightSourceEnabled, setDirectLightSourceEnabled, PROPERTY_FIELD_MEMORIZE);

	/// Enables shadows for the direct light source.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, shadowsEnabled, setShadowsEnabled, PROPERTY_FIELD_MEMORIZE);

	/// Controls the brightness of the default direct light source.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, defaultLightSourceIntensity, setDefaultLightSourceIntensity, PROPERTY_FIELD_MEMORIZE);

	/// Enables ambient occlusion lighting.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, ambientOcclusionEnabled, setAmbientOcclusionEnabled, PROPERTY_FIELD_MEMORIZE);

	/// Controls quality of ambient occlusion.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, ambientOcclusionSamples, setAmbientOcclusionSamples, PROPERTY_FIELD_MEMORIZE);

	/// Controls the brightness of the sky light source used for ambient occlusion.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, ambientOcclusionBrightness, setAmbientOcclusionBrightness, PROPERTY_FIELD_MEMORIZE);

	/// Enables depth-of-field rendering.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, depthOfFieldEnabled, setDepthOfFieldEnabled, PROPERTY_FIELD_MEMORIZE);

	/// Controls the camera's focal length, which is used for depth-of-field rendering.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, dofFocalLength, setDofFocalLength, PROPERTY_FIELD_MEMORIZE);

	/// Controls the camera's aperture, which is used for depth-of-field rendering.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, dofAperture, setDofAperture, PROPERTY_FIELD_MEMORIZE);

	/// The Tachyon internal scene handle.
	SceneHandle _rtscene = nullptr;

	/// List of image primitives that need to be painted over the final image.
	std::vector<std::tuple<QImage,Point2,Vector2>> _imageDrawCalls;

	/// List of text primitives that need to be painted over the final image.
	std::vector<std::tuple<QString,ColorA,QFont,Point2,int>> _textDrawCalls;
};

}	// End of namespace
}	// End of namespace


