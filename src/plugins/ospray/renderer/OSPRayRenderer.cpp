///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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

#include <core/Core.h>
#include <core/rendering/FrameBuffer.h>
#include <core/rendering/RenderSettings.h>
#include <core/oo/CloneHelper.h>
#include <core/app/Application.h>
#include <core/dataset/scene/ObjectNode.h>
#include <core/utilities/concurrent/Task.h>
#include <core/utilities/units/UnitsManager.h>
#include "OSPRayRenderer.h"

namespace Ovito { namespace OSPRay {

IMPLEMENT_OVITO_CLASS(OSPRayRenderer);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, antialiasingEnabled);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, antialiasingSamples);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, directLightSourceEnabled);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, shadowsEnabled);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, defaultLightSourceIntensity);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, ambientOcclusionEnabled);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, ambientOcclusionSamples);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, ambientOcclusionBrightness);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, depthOfFieldEnabled);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, dofFocalLength);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, dofAperture);
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, antialiasingEnabled, "Enable anti-aliasing");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, antialiasingSamples, "Anti-aliasing samples");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, directLightSourceEnabled, "Direct light");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, shadowsEnabled, "Shadows");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, defaultLightSourceIntensity, "Direct light intensity");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, ambientOcclusionEnabled, "Ambient occlusion");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, ambientOcclusionSamples, "Ambient occlusion samples");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, ambientOcclusionBrightness, "Ambient occlusion brightness");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, depthOfFieldEnabled, "Depth of field");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, dofFocalLength, "Focal length");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, dofAperture, "Aperture");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRayRenderer, antialiasingSamples, IntegerParameterUnit, 1, 500);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(OSPRayRenderer, defaultLightSourceIntensity, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(OSPRayRenderer, ambientOcclusionBrightness, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(OSPRayRenderer, dofFocalLength, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRayRenderer, dofAperture, FloatParameterUnit, 0, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRayRenderer, ambientOcclusionSamples, IntegerParameterUnit, 1, 100);

/******************************************************************************
* Default constructor.
******************************************************************************/
OSPRayRenderer::OSPRayRenderer(DataSet* dataset) : NonInteractiveSceneRenderer(dataset),
	_antialiasingEnabled(true), 
	_directLightSourceEnabled(true), 
	_shadowsEnabled(true),
	_antialiasingSamples(12), 
	_ambientOcclusionEnabled(true), 
	_ambientOcclusionSamples(12),
	_defaultLightSourceIntensity(FloatType(0.9)), 
	_ambientOcclusionBrightness(FloatType(0.8)), 
	_depthOfFieldEnabled(false),
	_dofFocalLength(40), 
	_dofAperture(FloatType(1e-2))
{
}

/******************************************************************************
* Prepares the renderer for rendering of the given scene.
******************************************************************************/
bool OSPRayRenderer::startRender(DataSet* dataset, RenderSettings* settings)
{
	if(!NonInteractiveSceneRenderer::startRender(dataset, settings))
		return false;

	_ospDevice = std::make_unique<ospray::cpp::Device>();
	ospDeviceSetStatusFunc(_ospDevice->handle(), [](const char* messageText) {
		qDebug() << "log output:" << messageText;
	});

	// Use only the number of parallel rendering threads allowed by the user. 
	_ospDevice->set("numThreads", Application::instance()->idealThreadCount());
	_ospDevice->set("logLevel", 1);
	_ospDevice->commit();

	// Activate OSPRay device.
	_ospDevice->setCurrent();

	return true;
}

/******************************************************************************
* Renders a single animation frame into the given frame buffer.
******************************************************************************/
bool OSPRayRenderer::renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, const PromiseBase& promise)
{
	promise.setProgressText(tr("Handing scene data to OSPRay renderer"));

	// image size
	ospcommon::vec2i imgSize;
	imgSize.x = renderSettings()->outputImageWidth(); 
	imgSize.y = renderSettings()->outputImageHeight();

	// camera
	ospcommon::vec3f cam_pos{0.f, 0.f, 0.f};
	ospcommon::vec3f cam_up{0.f, 1.f, 0.f};
	ospcommon::vec3f cam_view{0.1f, 0.f, 1.f};	

	// create and setup camera
	ospray::cpp::Camera camera("perspective");
	camera.set("aspect", imgSize.x / (float)imgSize.y);
	camera.set("pos", cam_pos);
	camera.set("dir", cam_view);
	camera.set("up",  cam_up);
	camera.commit(); // commit each object to indicate modifications are done

	// triangle mesh data
	float vertex[] = { -1.0f, -1.0f, 3.0f, 0.f,
		-1.0f,  1.0f, 3.0f, 0.f,
		1.0f, -1.0f, 3.0f, 0.f,
		0.1f,  0.1f, 0.3f, 0.f };
	float color[] =  { 0.9f, 0.5f, 0.5f, 1.0f,
		0.8f, 0.8f, 0.8f, 1.0f,
		0.8f, 0.8f, 0.8f, 1.0f,
		0.5f, 0.9f, 0.5f, 1.0f };
	int32_t index[] = { 0, 1, 2, 1, 2, 3 };	

	// create and setup model and mesh
	ospray::cpp::Geometry mesh("triangles");
	ospray::cpp::Data data(4, OSP_FLOAT3A, vertex); // OSP_FLOAT3 format is also supported for vertex positions
	data.commit();
	mesh.set("vertex", data);

	data = ospray::cpp::Data(4, OSP_FLOAT4, color);
	data.commit();
	mesh.set("vertex.color", data);

	data = ospray::cpp::Data(2, OSP_INT3, index); // OSP_INT4 format is also supported for triangle indices
	data.commit();
	mesh.set("index", data);

	mesh.commit();

	ospray::cpp::Model world;
	world.addGeometry(mesh);
	world.commit();	

	// create renderer
	ospray::cpp::Renderer renderer("scivis"); // choose Scientific Visualization renderer

	// create and setup light for Ambient Occlusion
	ospray::cpp::Light light = renderer.newLight("ambient");
	light.commit();
	auto lightHandle = light.handle();
	ospray::cpp::Data lights(1, OSP_LIGHT, &lightHandle);
	lights.commit();

	// complete setup of renderer
	renderer.set("aoSamples", 1);
	renderer.set("bgColor", 1.0f); // white, transparent
	renderer.set("model",  world);
	renderer.set("camera", camera);
	renderer.set("lights", lights);
	renderer.commit();

	// create and setup framebuffer
	ospray::cpp::FrameBuffer framebuffer(imgSize, OSP_FB_SRGBA, OSP_FB_COLOR | /*OSP_FB_DEPTH |*/ OSP_FB_ACCUM);
	framebuffer.clear(OSP_FB_COLOR | OSP_FB_ACCUM);

	// render frame
	promise.setProgressMaximum(10);
	promise.setProgressText(tr("Rendering image"));
	for(int frames = 0; frames < 10; frames++) {
		renderer.renderFrame(framebuffer, OSP_FB_COLOR | OSP_FB_ACCUM);	
		// access framebuffer and write its content as PPM file
		uint32_t* fb = (uint32_t*)framebuffer.map(OSP_FB_COLOR);
		QImage img(reinterpret_cast<const uchar*>(fb), imgSize.x, imgSize.y, QImage::Format_RGBA8888);
		frameBuffer->image() = img.copy();
		frameBuffer->update();
		framebuffer.unmap(fb);

		promise.setProgressValue(frames);
		if(promise.isCanceled())
			break;	
	}

	return !promise.isCanceled();
}

/******************************************************************************
* Finishes the rendering pass. This is called after all animation frames have been rendered
* or when the rendering operation has been aborted.
******************************************************************************/
void OSPRayRenderer::endRender()
{
	_ospDevice.reset();

	// Release draw call buffers.
	_imageDrawCalls.clear();
	_textDrawCalls.clear();

	NonInteractiveSceneRenderer::endRender();
}

/******************************************************************************
* Renders the line geometry stored in the given buffer.
******************************************************************************/
void OSPRayRenderer::renderLines(const DefaultLinePrimitive& lineBuffer)
{
	// Lines are not supported by this renderer.
}

/******************************************************************************
* Renders the particles stored in the given buffer.
******************************************************************************/
void OSPRayRenderer::renderParticles(const DefaultParticlePrimitive& particleBuffer)
{
	auto p = particleBuffer.positions().cbegin();
	auto p_end = particleBuffer.positions().cend();
	auto c = particleBuffer.colors().cbegin();
	auto r = particleBuffer.radii().cbegin();

	const AffineTransformation tm = modelTM();

	if(particleBuffer.particleShape() == ParticlePrimitive::SphericalShape) {
		
#if 0
		OSPData sphereData = ospNewData(particleBuffer.positions().size(),
			OSPDataType,
			const void *source,
			const uint32_t dataCreationFlags = 0);

		OSPGeometry spheres = ospNewGeometry("spheres");
		ospDeviceSet1i(spheres, "bytes_per_sphere", 16);
		ospDeviceSet1i(spheres, "offset_radius", 12);
		
		// Rendering spherical particles.
		for(; p != p_end; ++p, ++c, ++r) {
			if(c->a() > 0) {
				Point3 tp = tm * (*p);
			}
		}

#endif
	}
}

/******************************************************************************
* Renders the arrow elements stored in the given buffer.
******************************************************************************/
void OSPRayRenderer::renderArrows(const DefaultArrowPrimitive& arrowBuffer)
{
}

/******************************************************************************
* Renders the text stored in the given buffer.
******************************************************************************/
void OSPRayRenderer::renderText(const DefaultTextPrimitive& textBuffer, const Point2& pos, int alignment)
{
	_textDrawCalls.push_back(std::make_tuple(textBuffer.text(), textBuffer.color(), textBuffer.font(), pos, alignment));
}

/******************************************************************************
* Renders the image stored in the given buffer.
******************************************************************************/
void OSPRayRenderer::renderImage(const DefaultImagePrimitive& imageBuffer, const Point2& pos, const Vector2& size)
{
	_imageDrawCalls.push_back(std::make_tuple(imageBuffer.image(), pos, size));
}

/******************************************************************************
* Renders the triangle mesh stored in the given buffer.
******************************************************************************/
void OSPRayRenderer::renderMesh(const DefaultMeshPrimitive& meshBuffer)
{
}

}	// End of namespace
}	// End of namespace
