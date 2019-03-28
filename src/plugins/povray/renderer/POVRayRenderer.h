///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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

#include <QTemporaryFile>

namespace Ovito { namespace POVRay {

/**
 * \brief A scene renderer that calls the external POV-Ray ray-tracing engine.
 */
class OVITO_POVRAY_EXPORT POVRayRenderer : public NonInteractiveSceneRenderer
{
	Q_OBJECT
	OVITO_CLASS(POVRayRenderer)
	Q_CLASSINFO("DisplayName", "POV-Ray");

public:

	/// Constructor.
	Q_INVOKABLE POVRayRenderer(DataSet* dataset);

	///	Prepares the renderer for rendering of the given scene.
	/// Throws an exception on error. Returns false when the operation has been aborted by the user.
	virtual bool startRender(DataSet* dataset, RenderSettings* settings) override;

	/// This method is called just before renderFrame() is called.
	/// Sets the view projection parameters, the animation frame to render,
	/// and the viewport who is being rendered.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp) override;

	/// Renders a single animation frame into the given frame buffer.
	/// Throws an exception on error. Returns false when the operation has been aborted by the user.
	virtual bool renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, AsyncOperation& operation) override;

	/// This method is called after renderFrame() has been called.
	virtual void endFrame(bool renderSuccessful) override;

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

	/// Sets the (open) I/O device to which the renderer should write the POV-Ray scene.
	void setScriptOutputDevice(QIODevice* device) {
		_outputStream.setDevice(device);
	}

private:

    /// Writes a 3d vector to the output stream in POV-Ray format.
	void write(const Vector3& v) {
		_outputStream << "<" << v.x() << ", " << v.z() << ", " << v.y() << ">";
	}

    /// Writes a 3d point to the output stream in POV-Ray format.
	void write(const Point3& p) {
		_outputStream << "<" << p.x() << ", " << p.z() << ", " << p.y() << ">";
	}

    /// Writes a color to the output stream in POV-Ray format.
	void write(const Color& c) {
		_outputStream << "rgb <" << c.r() << ", " << c.g() << ", " << c.b() << ">";
	}

    /// Writes a color with alpha channel to the output stream in POV-Ray format.
	void write(const ColorA& c) {
		_outputStream << "rgbt <" << c.r() << ", " << c.g() << ", " << c.b() << ", " << std::max(FloatType(0), FloatType(1) - c.a()) << ">";
	}

    /// Writes a matrix to the output stream in POV-Ray format.
	void write(const AffineTransformation& m) {
		_outputStream << "<";
		_outputStream << m(0,0) << ", " << m(2,0) << ", " << m(1,0) << ", ";
		_outputStream << m(0,2) << ", " << m(2,2) << ", " << m(1,2) << ", ";
		_outputStream << m(0,1) << ", " << m(2,1) << ", " << m(1,1) << ", ";
		_outputStream << m(0,3) << ", " << m(2,3) << ", " << m(1,3);
		_outputStream << ">";
	}

private:

	/// List of image primitives that need to be painted over the final image.
	std::vector<std::tuple<QImage,Point2,Vector2>> _imageDrawCalls;

	/// List of text primitives that need to be painted over the final image.
	std::vector<std::tuple<QString,ColorA,QFont,Point2,int>> _textDrawCalls;

	/// The stream which the POVRay script is written to.
	QTextStream _outputStream;

	/// The temporary file for passing the scene data to POV-Ray.
	std::unique_ptr<QTemporaryFile> _sceneFile;

	/// The temporary file for receiving the rendered image from POV-Ray.
	std::unique_ptr<QTemporaryFile> _imageFile;

	/// This is used by the POVRayExporter class to make the export process interruptable.
	TaskPtr _exportOperation;

	/// The POV-Ray quality level to use for rendering (0 <= level <= 11).
	/// See POV-Ray documentation for details.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, qualityLevel, setQualityLevel, PROPERTY_FIELD_MEMORIZE);

	/// Turns anti-aliasing on/off
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, antialiasingEnabled, setAntialiasingEnabled, PROPERTY_FIELD_MEMORIZE);

	/// Controls the AA sampling method (only 1 or 2 are valid).
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, samplingMethod, setSamplingMethod, PROPERTY_FIELD_MEMORIZE);

	/// Controls the anti-aliasing threshold.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, AAThreshold, setAAThreshold, PROPERTY_FIELD_MEMORIZE);

	/// Controls the number of AA samples.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, antialiasDepth, setAntialiasDepth, PROPERTY_FIELD_MEMORIZE);

	/// Turns on AA-jitter.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, jitterEnabled, setJitterEnabled, PROPERTY_FIELD_MEMORIZE);

	/// Shows or supresses the POV-Ray rendering window.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, povrayDisplayEnabled, setPovrayDisplayEnabled, PROPERTY_FIELD_MEMORIZE);

	/// Turns on radiosity.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, radiosityEnabled, setRadiosityEnabled, PROPERTY_FIELD_MEMORIZE);

	/// Controls the number of rays that are sent out whenever a new radiosity value has to be calculated.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, radiosityRayCount, setRadiosityRayCount, PROPERTY_FIELD_MEMORIZE);

	/// Determines how many recursion levels are used to calculate the diffuse inter-reflection.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, radiosityRecursionLimit, setRadiosityRecursionLimit, PROPERTY_FIELD_MEMORIZE);

	/// Controls the fraction of error tolerated for the radiosity calculation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, radiosityErrorBound, setRadiosityErrorBound, PROPERTY_FIELD_MEMORIZE);

	/// Enables depth-of-field rendering.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, depthOfFieldEnabled, setDepthOfFieldEnabled, PROPERTY_FIELD_MEMORIZE);

	/// Controls the camera's focal length, which is used for depth-of-field rendering.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, dofFocalLength, setDofFocalLength, PROPERTY_FIELD_MEMORIZE);

	/// Controls the camera's aperture, which is used for depth-of-field rendering.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, dofAperture, setDofAperture, PROPERTY_FIELD_MEMORIZE);

	/// Controls the number of sampling rays used for focal blur.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, dofSampleCount, setDofSampleCount, PROPERTY_FIELD_MEMORIZE);

	/// Path to the external POV-Ray executable.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(QString, povrayExecutable, setPovrayExecutable, PROPERTY_FIELD_MEMORIZE);

	/// Enables omni­directional stereo projection.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, odsEnabled, setODSEnabled, PROPERTY_FIELD_MEMORIZE);

	/// The interpupillary distance for stereo projection.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, interpupillaryDistance, setInterpupillaryDistance, PROPERTY_FIELD_MEMORIZE);

	friend class POVRayExporter;
};

}	// End of namespace
}	// End of namespace


