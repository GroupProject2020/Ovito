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

#include <ospray/ospray_cpp.h>

namespace Ovito { namespace OSPRay {

IMPLEMENT_OVITO_CLASS(OSPRayRenderer);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, refinementIterations);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, samplesPerPixel);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, maxRayRecursion);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, directLightSourceEnabled);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, shadowsEnabled);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, defaultLightSourceIntensity);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, ambientOcclusionEnabled);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, ambientOcclusionSamples);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, ambientOcclusionBrightness);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, depthOfFieldEnabled);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, dofFocalLength);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, dofAperture);
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, refinementIterations, "Refinement iterations");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, samplesPerPixel, "Samples per pixel");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, maxRayRecursion, "Maximum ray recursion depth");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, directLightSourceEnabled, "Direct light");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, shadowsEnabled, "Shadows");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, defaultLightSourceIntensity, "Direct light intensity");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, ambientOcclusionEnabled, "Ambient occlusion");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, ambientOcclusionSamples, "Ambient occlusion samples");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, ambientOcclusionBrightness, "Ambient occlusion brightness");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, depthOfFieldEnabled, "Depth of field");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, dofFocalLength, "Focal length");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, dofAperture, "Aperture");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRayRenderer, refinementIterations, IntegerParameterUnit, 1, 500);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRayRenderer, samplesPerPixel, IntegerParameterUnit, 1, 500);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRayRenderer, maxRayRecursion, IntegerParameterUnit, 1, 100);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(OSPRayRenderer, defaultLightSourceIntensity, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(OSPRayRenderer, ambientOcclusionBrightness, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(OSPRayRenderer, dofFocalLength, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRayRenderer, dofAperture, FloatParameterUnit, 0, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRayRenderer, ambientOcclusionSamples, IntegerParameterUnit, 1, 100);

/******************************************************************************
* Default constructor.
******************************************************************************/
OSPRayRenderer::OSPRayRenderer(DataSet* dataset) : NonInteractiveSceneRenderer(dataset),
	_refinementIterations(10),
	_directLightSourceEnabled(true), 
	_shadowsEnabled(true),
	_samplesPerPixel(4),
	_maxRayRecursion(20),
	_ambientOcclusionEnabled(true), 
	_ambientOcclusionSamples(12),
	_defaultLightSourceIntensity(FloatType(3.0)), 
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

	// Create OSPRay device.
	OSPDevice device = ospGetCurrentDevice();
	if(!device) {
		device = ospNewDevice();
		if(ospLoadModule("ovito") != OSP_NO_ERROR)
			throwException(tr("Failed to load OSPRay module 'ovito': %1").arg(ospDeviceGetLastErrorMsg(device)));
	}

	// Use only the number of parallel rendering threads allowed by the user. 
	ospDeviceSet1i(device, "numThreads", Application::instance()->idealThreadCount());
	ospDeviceCommit(device);

	// Activate OSPRay device.
	ospSetCurrentDevice(device);

	return true;
}

/******************************************************************************
* Renders a single animation frame into the given frame buffer.
******************************************************************************/
bool OSPRayRenderer::renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, const PromiseBase& promise)
{
	promise.setProgressText(tr("Handing scene data to OSPRay renderer"));
	try {

		// image size
		ospcommon::vec2i imgSize;
		imgSize.x = renderSettings()->outputImageWidth(); 
		imgSize.y = renderSettings()->outputImageHeight();

		// Calculate camera information.
		Point3 cam_pos;
		Vector3 cam_dir;
		Vector3 cam_up;
		if(projParams().isPerspective) {
			cam_pos = projParams().inverseProjectionMatrix * Point3(0,0,0);
			cam_dir = projParams().inverseProjectionMatrix * Point3(0,0,0) - Point3::Origin();
			cam_up = projParams().inverseProjectionMatrix * Point3(0,1,0) - cam_pos;
			cam_pos = Point3::Origin() + projParams().inverseViewMatrix.translation();
			cam_dir = (projParams().inverseViewMatrix * cam_dir).normalized();
			cam_up = (projParams().inverseViewMatrix * cam_up).normalized();
		}
		else {
			cam_pos = projParams().inverseProjectionMatrix * Point3(0,0,-1);
			cam_dir = projParams().inverseProjectionMatrix * Point3(0,0,1) - cam_pos;
			cam_up = projParams().inverseProjectionMatrix * Point3(0,1,-1) - cam_pos;
			cam_pos = projParams().inverseViewMatrix * cam_pos;
			cam_dir = (projParams().inverseViewMatrix * cam_dir).normalized();
			cam_up = (projParams().inverseViewMatrix * cam_up).normalized();
		}

		// create and setup camera
		ospray::cpp::Camera camera(projParams().isPerspective ? "perspective" : "orthographic");
		camera.set("aspect", imgSize.x / (float)imgSize.y);
		camera.set("pos", cam_pos.x(), cam_pos.y(), cam_pos.z());
		camera.set("dir", cam_dir.x(), cam_dir.y(), cam_dir.z());
		camera.set("up",  cam_up.x(), cam_up.y(), cam_up.z());
		camera.set("nearClip",  projParams().znear);
		if(projParams().isPerspective)
			camera.set("fovy", projParams().fieldOfView * FloatType(180) / FLOATTYPE_PI);
		else
			camera.set("height", projParams().fieldOfView * 2);	
		if(projParams().isPerspective && depthOfFieldEnabled() && dofFocalLength() > 0 && dofAperture() > 0) {
			camera.set("apertureRadius", dofAperture());
			camera.set("focusDistance", dofFocalLength());
		}	
		camera.commit();
		
		// Transfer renderable geometry from OVITO to OSPRay renderer.
		ospray::cpp::Model world;
		_world = &world;
		if(!renderScene(promise))
			return false;
		world.commit();	
			
		// Create OSPRay renderer
#if 1
		ospray::cpp::Renderer renderer("scivis"); // choose Scientific Visualization renderer
#else
		ospray::cpp::Renderer renderer("raytracer"); // choose Path Tracer renderer
#endif	

		// Create direct light.
		std::vector<ospray::cpp::Light> lightSources;
		if(directLightSourceEnabled()) {
			ospray::cpp::Light light = renderer.newLight("distant");
			Vector3 lightDir = projParams().inverseViewMatrix * Vector3(0.2f,-0.2f,-1.0f);
			light.set("direction", lightDir.x(), lightDir.y(), lightDir.z());
			light.set("intensity", defaultLightSourceIntensity());
			light.set("isVisible", false);
			lightSources.push_back(std::move(light));
		}

		// Create and setup light for Ambient Occlusion
		if(ambientOcclusionEnabled()) {
			ospray::cpp::Light light = renderer.newLight("ambient");
			light.set("intensity", ambientOcclusionBrightness());
			lightSources.push_back(std::move(light));
		}

		// Create list of all light sources.
		std::vector<OSPLight> lightHandles(lightSources.size());
		for(size_t i = 0; i < lightSources.size(); i++) {
			lightSources[i].commit();
			lightHandles[i] = lightSources[i].handle();
		}
		ospray::cpp::Data lights(lightHandles.size(), OSP_LIGHT, lightHandles.data());
		lights.commit();

		TimeInterval iv;
		Color backgroundColor;
		renderSettings()->backgroundColorController()->getColorValue(time(), backgroundColor, iv);
		
		renderer.set("model",  world);
		renderer.set("camera", camera);
		renderer.set("lights", lights);
		renderer.set("spp", std::max(samplesPerPixel(), 1));
#if 1
		renderer.set("bgColor", backgroundColor.r(), backgroundColor.g(), backgroundColor.b(), renderSettings()->generateAlphaChannel() ? FloatType(0) : FloatType(1));
		renderer.set("shadowsEnabled", shadowsEnabled());
		renderer.set("aoSamples", ambientOcclusionEnabled() ? ambientOcclusionSamples() : 0);
		renderer.set("aoTransparencyEnabled", true);
#endif
		renderer.set("maxDepth", std::max(maxRayRecursion(), 1));
		renderer.commit();

		// Create and set up OSPRay framebuffer.
		ospray::cpp::FrameBuffer framebuffer(imgSize, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
		framebuffer.clear(OSP_FB_COLOR | OSP_FB_ACCUM);

		// Render frame.
		promise.setProgressMaximum(refinementIterations());
		promise.setProgressText(tr("Rendering image"));
		if(promise.isCanceled())
			return false;

		for(int iteration = 0; iteration < promise.progressMaximum(); iteration++) {
			renderer.renderFrame(framebuffer, OSP_FB_COLOR | OSP_FB_ACCUM);

			// Access framebuffer data and copy it to our own framebuffer.
			uint32_t* fb = (uint32_t*)framebuffer.map(OSP_FB_COLOR);
			QImage img(reinterpret_cast<const uchar*>(fb), imgSize.x, imgSize.y, QImage::Format_RGBA8888);
#if 1
			frameBuffer->image() = img.mirrored();
#else 
			QPainter painter(&frameBuffer->image());
			if(!renderSettings()->generateAlphaChannel()) {
				painter.fillRect(frameBuffer->image().rect(), backgroundColor);
			}
			painter.drawImage(0, 0, img.mirrored());
#endif		
			frameBuffer->update();
			framebuffer.unmap(fb);

			// Update progress display.
			promise.setProgressValue(iteration);
			if(promise.isCanceled())
				break;	
		}

		// Execute recorded overlay draw calls.
		QPainter painter(&frameBuffer->image());
		for(const auto& imageCall : _imageDrawCalls) {
			QRectF rect(std::get<1>(imageCall).x(), std::get<1>(imageCall).y(), std::get<2>(imageCall).x(), std::get<2>(imageCall).y());
			painter.drawImage(rect, std::get<0>(imageCall));
			frameBuffer->update(rect.toAlignedRect());
		}
		for(const auto& textCall : _textDrawCalls) {
			QRectF pos(std::get<3>(textCall).x(), std::get<3>(textCall).y(), 0, 0);
			painter.setPen(std::get<1>(textCall));
			painter.setFont(std::get<2>(textCall));
			QRectF boundingRect;
			painter.drawText(pos, std::get<4>(textCall) | Qt::TextSingleLine | Qt::TextDontClip, std::get<0>(textCall), &boundingRect);
			frameBuffer->update(boundingRect.toAlignedRect());
		}	
	}
	catch(const std::runtime_error& ex) {
		throwException(tr("OSPRay error: %1").arg(ex.what()));
	}
	
	return !promise.isCanceled();
}

/******************************************************************************
* Finishes the rendering pass. This is called after all animation frames have been rendered
* or when the rendering operation has been aborted.
******************************************************************************/
void OSPRayRenderer::endRender()
{
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

		// Compile buffer with sphere data in OSPRay format.
		std::vector<ospcommon::vec4f> sphereData(particleBuffer.positions().size());
		std::vector<ospcommon::vec4f> colorData(particleBuffer.positions().size());
		auto sphereIter = sphereData.begin();
		auto colorIter = colorData.begin();
		for(; p != p_end; ++p, ++c, ++r) {
			if(c->a() > 0) {
				Point3 tp = tm * (*p);
				(*sphereIter)[0] = tp.x();
				(*sphereIter)[1] = tp.y();
				(*sphereIter)[2] = tp.z();
				(*sphereIter)[3] = *r;
				(*colorIter)[0] = c->r();
				(*colorIter)[1] = c->g();
				(*colorIter)[2] = c->b();
				(*colorIter)[3] = c->a();
				++sphereIter;
				++colorIter;
			}
		}
		size_t nspheres = sphereIter - sphereData.begin();
		if(nspheres == 0) return;
		
		ospray::cpp::Geometry spheres("spheres");
		spheres.set("bytes_per_sphere", (int)sizeof(ospcommon::vec4f));
		spheres.set("offset_radius", (int)sizeof(float) * 3);

		ospray::cpp::Data data(nspheres, OSP_FLOAT4, sphereData.data());
		data.commit();
		spheres.set("spheres", data);

		data = ospray::cpp::Data(nspheres, OSP_FLOAT4, colorData.data());
		data.commit();
		spheres.set("color", data);
		
		spheres.commit();
		_world->addGeometry(spheres);
	}
}

/******************************************************************************
* Renders the arrow elements stored in the given buffer.
******************************************************************************/
void OSPRayRenderer::renderArrows(const DefaultArrowPrimitive& arrowBuffer)
{
	const AffineTransformation tm = modelTM();

	// Compile buffer with cylinder data in OSPRay format.
	std::vector<std::array<float,7>> cylData(arrowBuffer.elements().size());
	std::vector<ospcommon::vec4f> colorData(arrowBuffer.elements().size());
	std::vector<std::array<float,7>> discData(arrowBuffer.shape() == ArrowPrimitive::CylinderShape ? 0 : arrowBuffer.elements().size()*2);
	std::vector<ospcommon::vec4f> discColorData(discData.size());
	std::vector<std::array<float,7>> coneData(arrowBuffer.shape() == ArrowPrimitive::CylinderShape ? 0 : arrowBuffer.elements().size());
	std::vector<ospcommon::vec4f> coneColorData(coneData.size());
	auto cylIter = cylData.begin();
	auto colorIter = colorData.begin();
	auto discIter = discData.begin();
	auto discColorIter = discColorData.begin();
	auto coneIter = coneData.begin();
	auto coneColorIter = coneColorData.begin();
	for(const DefaultArrowPrimitive::ArrowElement& element : arrowBuffer.elements()) {
		if(element.color.a() > 0) {
			Point3 tp = tm * element.pos;
			Vector3 ta;
			if(arrowBuffer.shape() == ArrowPrimitive::CylinderShape) {
				ta = tm * element.dir;
			}
			else {
				FloatType arrowHeadRadius = element.width * FloatType(2.5);
				FloatType arrowHeadLength = arrowHeadRadius * FloatType(1.8);
				FloatType length = element.dir.length();
				if(length == 0)
					continue;

				if(length > arrowHeadLength) {
					tp = tm * element.pos;
					ta = tm * (element.dir * ((length - arrowHeadLength) / length));
					Vector3 tb = tm * (element.dir * (arrowHeadLength / length));
					Vector3 normal = ta;
					normal.normalizeSafely();
					(*discIter)[0] = tp.x();
					(*discIter)[1] = tp.y();
					(*discIter)[2] = tp.z();
					(*discIter)[3] = -normal.x();
					(*discIter)[4] = -normal.y();
					(*discIter)[5] = -normal.z();
					(*discIter)[6] = element.width;
					(*discColorIter)[0] = element.color.r();
					(*discColorIter)[1] = element.color.g();
					(*discColorIter)[2] = element.color.b();
					(*discColorIter)[3] = element.color.a();
					++discIter;
					++discColorIter;
					(*discIter)[0] = tp.x() + ta.x();
					(*discIter)[1] = tp.y() + ta.y();
					(*discIter)[2] = tp.z() + ta.z();
					(*discIter)[3] = -normal.x();
					(*discIter)[4] = -normal.y();
					(*discIter)[5] = -normal.z();
					(*discIter)[6] = arrowHeadRadius;
					(*discColorIter)[0] = element.color.r();
					(*discColorIter)[1] = element.color.g();
					(*discColorIter)[2] = element.color.b();
					(*discColorIter)[3] = element.color.a();
					++discIter;
					++discColorIter;
					(*coneIter)[0] = tp.x() + ta.x() + tb.x();
					(*coneIter)[1] = tp.y() + ta.y() + tb.y();
					(*coneIter)[2] = tp.z() + ta.z() + tb.z();
					(*coneIter)[3] = -tb.x();
					(*coneIter)[4] = -tb.y();
					(*coneIter)[5] = -tb.z();
					(*coneIter)[6] = arrowHeadRadius;
					(*coneColorIter)[0] = element.color.r();
					(*coneColorIter)[1] = element.color.g();
					(*coneColorIter)[2] = element.color.b();
					(*coneColorIter)[3] = element.color.a();
					++coneIter;
					++coneColorIter;
				}
				else {
					FloatType r = arrowHeadRadius * length / arrowHeadLength;
					ta = tm * element.dir;
					Vector3 normal = ta;
					normal.normalizeSafely();
					(*discIter)[0] = tp.x();
					(*discIter)[1] = tp.y();
					(*discIter)[2] = tp.z();
					(*discIter)[3] = -normal.x();
					(*discIter)[4] = -normal.y();
					(*discIter)[5] = -normal.z();
					(*discIter)[6] = r;
					(*discColorIter)[0] = element.color.r();
					(*discColorIter)[1] = element.color.g();
					(*discColorIter)[2] = element.color.b();
					(*discColorIter)[3] = element.color.a();
					++discIter;
					++discColorIter;
					(*coneIter)[0] = tp.x() + ta.x();
					(*coneIter)[1] = tp.y() + ta.y();
					(*coneIter)[2] = tp.z() + ta.z();
					(*coneIter)[3] = -ta.x();
					(*coneIter)[4] = -ta.y();
					(*coneIter)[5] = -ta.z();
					(*coneIter)[6] = r;
					(*coneColorIter)[0] = element.color.r();
					(*coneColorIter)[1] = element.color.g();
					(*coneColorIter)[2] = element.color.b();
					(*coneColorIter)[3] = element.color.a();
					++coneIter;
					++coneColorIter;
					continue;
				}							
			}
			(*cylIter)[0] = tp.x();
			(*cylIter)[1] = tp.y();
			(*cylIter)[2] = tp.z();
			(*cylIter)[3] = tp.x() + ta.x();
			(*cylIter)[4] = tp.y() + ta.y();
			(*cylIter)[5] = tp.z() + ta.z();
			(*cylIter)[6] = element.width;
			(*colorIter)[0] = element.color.r();
			(*colorIter)[1] = element.color.g();
			(*colorIter)[2] = element.color.b();
			(*colorIter)[3] = element.color.a();
			++cylIter;
			++colorIter;
		}
	}
	size_t ncylinders = cylIter - cylData.begin();
	if(ncylinders != 0) {
		ospray::cpp::Geometry cylinders("cylinders");
		cylinders.set("bytes_per_cylinder", (int)sizeof(float) * 7);
		cylinders.set("offset_radius", (int)sizeof(float) * 6);

		ospray::cpp::Data data(ncylinders * 7, OSP_FLOAT, cylData.data());
		data.commit();
		cylinders.set("cylinders", data);

		data = ospray::cpp::Data(ncylinders, OSP_FLOAT4, colorData.data());
		data.commit();
		cylinders.set("color", data);
		
		cylinders.commit();
		_world->addGeometry(cylinders);
	}

	size_t ndiscs = discIter - discData.begin();
	if(ndiscs != 0) {
		ospray::cpp::Geometry discs("discs");
		discs.set("bytes_per_disc", (int)sizeof(float) * 7);
		discs.set("offset_center", (int)sizeof(float) * 0);
		discs.set("offset_normal", (int)sizeof(float) * 3);
		discs.set("offset_radius", (int)sizeof(float) * 6);

		ospray::cpp::Data data(ndiscs * 7, OSP_FLOAT, discData.data());
		data.commit();
		discs.set("discs", data);

		data = ospray::cpp::Data(ndiscs, OSP_FLOAT4, discColorData.data());
		data.commit();
		discs.set("color", data);
		
		discs.commit();
		_world->addGeometry(discs);
	}

	size_t ncones = coneIter - coneData.begin();
	if(ncones != 0) {
		ospray::cpp::Geometry cones("cones");
		cones.set("bytes_per_cone", (int)sizeof(float) * 7);
		cones.set("offset_center", (int)sizeof(float) * 0);
		cones.set("offset_axis", (int)sizeof(float) * 3);
		cones.set("offset_radius", (int)sizeof(float) * 6);

		ospray::cpp::Data data(ncones * 7, OSP_FLOAT, coneData.data());
		data.commit();
		cones.set("cones", data);

		data = ospray::cpp::Data(ncones, OSP_FLOAT4, coneColorData.data());
		data.commit();
		cones.set("color", data);
		
		cones.commit();
		_world->addGeometry(cones);
	}	
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
	const TriMesh& mesh = meshBuffer.mesh();

	// Allocate render vertex buffer.
	int renderVertexCount = mesh.faceCount() * 3;
	if(renderVertexCount == 0)
		return;

	std::vector<ColorAT<float>> colors(renderVertexCount);
	std::vector<Vector_3<float>> normals(renderVertexCount);
	std::vector<Point_3<float>> positions(renderVertexCount);
	std::vector<std::array<int,3>> indices(mesh.faceCount());

	const AffineTransformationT<float> tm = (AffineTransformationT<float>)modelTM();
	const Matrix_3<float> normalTM = tm.linear().inverse().transposed();
	quint32 allMask = 0;

	// Compute face normals.
	std::vector<Vector_3<float>> faceNormals(mesh.faceCount());
	auto faceNormal = faceNormals.begin();
	for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face, ++faceNormal) {
		const Point3& p0 = mesh.vertex(face->vertex(0));
		Vector3 d1 = mesh.vertex(face->vertex(1)) - p0;
		Vector3 d2 = mesh.vertex(face->vertex(2)) - p0;
		*faceNormal = normalTM * (Vector_3<float>)d2.cross(d1);
		if(*faceNormal != Vector_3<float>::Zero()) {
			//faceNormal->normalize();
			allMask |= face->smoothingGroups();
		}
	}

	// Initialize render vertices.
	auto rv_pos = positions.begin();
	auto rv_normal = normals.begin();
	auto rv_color = colors.begin();
	auto rv_indices = indices.begin();
	faceNormal = faceNormals.begin();
	ColorAT<float> defaultVertexColor = ColorAT<float>(meshBuffer.meshColor());
	int vindex = 0;
	for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face, ++faceNormal, ++rv_indices) {

		// Initialize render vertices for this face.
		for(size_t v = 0; v < 3; v++, ++rv_pos, ++rv_normal, ++rv_color) {
			(*rv_indices)[v] = vindex++;
			if(face->smoothingGroups())
				*rv_normal = Vector_3<float>::Zero();
			else
				*rv_normal = *faceNormal;
			*rv_pos = tm * (Point_3<float>)mesh.vertex(face->vertex(v));

			if(mesh.hasVertexColors())
				*rv_color = ColorAT<float>(mesh.vertexColor(face->vertex(v)));
			else if(mesh.hasFaceColors())
				*rv_color = ColorAT<float>(mesh.faceColor(face - mesh.faces().constBegin()));
			else if(face->materialIndex() < meshBuffer.materialColors().size() && face->materialIndex() >= 0)
				*rv_color = ColorAT<float>(meshBuffer.materialColors()[face->materialIndex()]);
			else
				*rv_color = defaultVertexColor;
		}
	}

	if(allMask) {
		std::vector<Vector_3<float>> groupVertexNormals(mesh.vertexCount());
		for(int group = 0; group < OVITO_MAX_NUM_SMOOTHING_GROUPS; group++) {
			quint32 groupMask = quint32(1) << group;
            if((allMask & groupMask) == 0) continue;

			// Reset work arrays.
            std::fill(groupVertexNormals.begin(), groupVertexNormals.end(), Vector_3<float>::Zero());

			// Compute vertex normals at original vertices for current smoothing group.
            faceNormal = faceNormals.begin();
			for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face, ++faceNormal) {
				// Skip faces which do not belong to the current smoothing group.
				if((face->smoothingGroups() & groupMask) == 0) continue;

				// Add face's normal to vertex normals.
				for(size_t fv = 0; fv < 3; fv++)
					groupVertexNormals[face->vertex(fv)] += *faceNormal;
			}

			// Transfer vertex normals from original vertices to render vertices.
			rv_normal = normals.begin();
			for(const auto& face : mesh.faces()) {
				if(face.smoothingGroups() & groupMask) {
					for(size_t fv = 0; fv < 3; fv++, ++rv_normal)
						*rv_normal += groupVertexNormals[face.vertex(fv)];
				}
				else rv_normal += 3;
			}
		}
	}

	ospray::cpp::Geometry triangles("triangles");	
		
	ospray::cpp::Data data(positions.size(), OSP_FLOAT3, positions.data());
	data.commit();
	triangles.set("vertex", data);

	data = ospray::cpp::Data(colors.size(), OSP_FLOAT4, colors.data());
	data.commit();
	triangles.set("vertex.color", data);

	data = ospray::cpp::Data(normals.size(), OSP_FLOAT3, normals.data());
	data.commit();
	triangles.set("vertex.normal", data);
	
	data = ospray::cpp::Data(mesh.faceCount(), OSP_INT3, indices.data());
	data.commit();
	triangles.set("index", data);
	
	triangles.commit();
	_world->addGeometry(triangles);
}

}	// End of namespace
}	// End of namespace
