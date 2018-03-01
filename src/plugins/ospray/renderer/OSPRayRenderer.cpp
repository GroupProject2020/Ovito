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
#include <core/app/PluginManager.h>
#include <core/dataset/scene/PipelineSceneNode.h>
#include <core/utilities/concurrent/Task.h>
#include <core/utilities/units/UnitsManager.h>
#include "OSPRayRenderer.h"

#include <ospray/ospray_cpp.h>
#include <render/LoadBalancer.h>
#include <ospcommon/tasking/parallel_for.h>

namespace Ovito { namespace OSPRay {

IMPLEMENT_OVITO_CLASS(OSPRayRenderer);
DEFINE_REFERENCE_FIELD(OSPRayRenderer, backend);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, refinementIterations);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, samplesPerPixel);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, maxRayRecursion);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, directLightSourceEnabled);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, defaultLightSourceIntensity);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, defaultLightSourceAngularDiameter);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, ambientLightEnabled);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, ambientBrightness);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, depthOfFieldEnabled);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, dofFocalLength);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, dofAperture);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, materialShininess);
DEFINE_PROPERTY_FIELD(OSPRayRenderer, materialSpecularBrightness);
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, backend, "OSPRay backend");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, refinementIterations, "Refinement passes");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, samplesPerPixel, "Samples per pixel");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, maxRayRecursion, "Max ray recursion depth");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, directLightSourceEnabled, "Direct light");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, defaultLightSourceIntensity, "Direct light intensity");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, defaultLightSourceAngularDiameter, "Angular diameter");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, ambientLightEnabled, "Ambient light");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, ambientBrightness, "Ambient light brightness");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, depthOfFieldEnabled, "Depth of field");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, dofFocalLength, "Focal length");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, dofAperture, "Aperture");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, materialShininess, "Shininess");
SET_PROPERTY_FIELD_LABEL(OSPRayRenderer, materialSpecularBrightness, "Specular brightness");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRayRenderer, refinementIterations, IntegerParameterUnit, 1, 500);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRayRenderer, samplesPerPixel, IntegerParameterUnit, 1, 500);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRayRenderer, maxRayRecursion, IntegerParameterUnit, 1, 100);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(OSPRayRenderer, defaultLightSourceIntensity, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRayRenderer, defaultLightSourceAngularDiameter, AngleParameterUnit, 0, FLOATTYPE_PI/4);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(OSPRayRenderer, ambientBrightness, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(OSPRayRenderer, dofFocalLength, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(OSPRayRenderer, dofAperture, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRayRenderer, materialShininess, FloatParameterUnit, 2, 10000);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRayRenderer, materialSpecularBrightness, PercentParameterUnit, 0, 1);

/******************************************************************************
* Default constructor.
******************************************************************************/
OSPRayRenderer::OSPRayRenderer(DataSet* dataset) : NonInteractiveSceneRenderer(dataset),
	_refinementIterations(8),
	_directLightSourceEnabled(true), 
	_samplesPerPixel(4),
	_maxRayRecursion(20),
	_defaultLightSourceIntensity(FloatType(3.0)), 
	_defaultLightSourceAngularDiameter(0),
	_ambientLightEnabled(true),
	_ambientBrightness(FloatType(0.8)), 
	_depthOfFieldEnabled(false),
	_dofFocalLength(40), 
	_dofAperture(FloatType(0.5f)),
	_materialShininess(10.0),
	_materialSpecularBrightness(0.05)
{
	// Create an instance of the default OSPRay rendering backend.
	OvitoClassPtr backendClass = PluginManager::instance().findClass("OSPRayRenderer", "OSPRaySciVisBackend");
	if(backendClass == nullptr) {
		QVector<OvitoClassPtr> classList = PluginManager::instance().listClasses(OSPRayBackend::OOClass());
		if(classList.isEmpty() == false) backendClass = classList.front();
	}
	if(backendClass)
		setBackend(static_object_cast<OSPRayBackend>(backendClass->createInstance(dataset)));
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

		// Load our extension module for OSPRay, which provides raytracing functions
		// for various geometry primitives (discs, cones, quadrics) needed by OVITO.

		// The ospLoadModule() function uses standard OS functions to load the dynamic library
		// (e.g. dlopen() on MacOS/Linux). Let them know where to find the extension module by 
		// setting the current working directory.

		QDir oldWDir = QDir::current();
#if defined(Q_OS_WIN)
		QDir::setCurrent(QCoreApplication::applicationDirPath());
#elif defined(Q_OS_MAC)
		QDir::setCurrent(QCoreApplication::applicationDirPath());
#else // Linux
		QDir::setCurrent(QCoreApplication::applicationDirPath() + QStringLiteral("/../lib/ovito"));
#endif

		if(ospLoadModule("ovito") != OSP_NO_ERROR)
			throwException(tr("Failed to load OSPRay module 'ovito': %1").arg(ospDeviceGetLastErrorMsg(device)));

		// Restore previous state.
		QDir::setCurrent(oldWDir.absolutePath());
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
	if(!backend())
		throwException(tr("No OSPRay rendering backend has been set."));

	promise.setProgressText(tr("Handing scene data to OSPRay renderer"));
	try {

		// Output image size:
		ospcommon::vec2i imgSize;
		imgSize.x = renderSettings()->outputImageWidth(); 
		imgSize.y = renderSettings()->outputImageHeight();

		// Make sure the target frame buffer has the right memory format.
		if(frameBuffer->image().format() != QImage::Format_ARGB32)
		frameBuffer->image() = frameBuffer->image().convertToFormat(QImage::Format_ARGB32);

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

		// Create and set up OSPRay camera
		OSPReferenceWrapper<ospray::cpp::Camera> camera(projParams().isPerspective ? "perspective" : "orthographic");
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

		// Create OSPRay renderer
		OSPReferenceWrapper<ospray::cpp::Renderer> renderer{backend()->createOSPRenderer()};
		_ospRenderer = &renderer;

		// Create standard material.
		OSPReferenceWrapper<ospray::cpp::Material> material{renderer.newMaterial("OBJMaterial")};
		material.set("Ns", materialShininess());
		material.set("Ks", materialSpecularBrightness(), materialSpecularBrightness(), materialSpecularBrightness());
		material.commit();
		_ospMaterial = &material;
		
		// Transfer renderable geometry from OVITO to OSPRay renderer.
		OSPReferenceWrapper<ospray::cpp::Model> world;
		_ospWorld = &world;
		if(!renderScene(promise))
			return false;
		world.commit();	
			
		// Create direct light.
		std::vector<OSPReferenceWrapper<ospray::cpp::Light>> lightSources;
		if(directLightSourceEnabled()) {
			OSPReferenceWrapper<ospray::cpp::Light> light{renderer.newLight("distant")};
			Vector3 lightDir = projParams().inverseViewMatrix * Vector3(0.2f,-0.2f,-1.0f);
			light.set("direction", lightDir.x(), lightDir.y(), lightDir.z());
			light.set("intensity", defaultLightSourceIntensity());
			light.set("isVisible", false);
			light.set("angularDiameter", defaultLightSourceAngularDiameter() * FloatType(180) / FLOATTYPE_PI);
			lightSources.push_back(std::move(light));
		}

		// Create and setup ambient light source.
		if(ambientLightEnabled()) {
			OSPReferenceWrapper<ospray::cpp::Light> light{renderer.newLight("ambient")};
			light.set("intensity", ambientBrightness());
			lightSources.push_back(std::move(light));
		}

		// Create list of all light sources.
		std::vector<OSPLight> lightHandles(lightSources.size());
		for(size_t i = 0; i < lightSources.size(); i++) {
			lightSources[i].commit();
			lightHandles[i] = lightSources[i].handle();
		}
		OSPReferenceWrapper<ospray::cpp::Data> lights(lightHandles.size(), OSP_LIGHT, lightHandles.data());
		lights.commit();

		TimeInterval iv;
		Color backgroundColor;
		renderSettings()->backgroundColorController()->getColorValue(time(), backgroundColor, iv);
		ColorA bgColorWithAlpha(backgroundColor.r(), backgroundColor.g(), backgroundColor.b(), renderSettings()->generateAlphaChannel() ? FloatType(0) : FloatType(1));
		
		renderer.set("model",  world);
		renderer.set("camera", camera);
		renderer.set("lights", lights);
		renderer.set("spp", std::max(samplesPerPixel(), 1));
		renderer.set("bgColor", bgColorWithAlpha.r(), bgColorWithAlpha.g(), bgColorWithAlpha.b(), bgColorWithAlpha.a());
		renderer.set("maxDepth", std::max(maxRayRecursion(), 1));
		renderer.commit();

		// Create and set up OSPRay framebuffer.
		OSPReferenceWrapper<ospray::cpp::FrameBuffer> osp_fb(imgSize, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
		osp_fb.clear(OSP_FB_COLOR | OSP_FB_ACCUM);

		// Clear frame buffer.
		frameBuffer->clear();
	
		// Define a custom load balancer for OSPRay that performs progressive updates of the frame buffer.
		class OVITOTiledLoadBalancer : public ospray::TiledLoadBalancer 
		{
		public:			
			void setProgressCallback(std::function<bool(int,int,int,int)> progressCallback) { 
				_progressCallback = std::move(progressCallback); 
			}

			float renderFrame(ospray::Renderer *renderer, ospray::FrameBuffer *fb, const ospray::uint32 channelFlags) override {
				void *perFrameData = renderer->beginFrame(fb);				
				int tileCount = fb->getTotalTiles();
				for(int taskIndex = 0; taskIndex < tileCount; taskIndex++) {
					const size_t numTiles_x = fb->getNumTiles().x;
					const size_t tile_y = taskIndex / numTiles_x;
					const size_t tile_x = taskIndex - tile_y*numTiles_x;
					const ospray::vec2i tileID(tile_x, tile_y);
					const ospray::int32 accumID = fb->accumID(tileID);

					if(fb->tileError(tileID) <= renderer->errorThreshold)
						continue;
#define MAX_TILE_SIZE 128
#if TILE_SIZE > MAX_TILE_SIZE
					auto tilePtr = make_unique<Tile>(tileID, fb->size, accumID);
					auto &tile   = *tilePtr;
#else
					ospray::Tile __aligned(64) tile(tileID, fb->size, accumID);
#endif
					ospray::tasking::parallel_for(numJobs(renderer->spp, accumID), [&](int tIdx) {
						renderer->renderTile(perFrameData, tile, tIdx);
					});
					fb->setTile(tile);

					if(_progressCallback) {
						if(!_progressCallback(tile.region.lower.x,tile.region.lower.y,tile.region.upper.x,tile.region.upper.y))
							break;
					}
				}

				renderer->endFrame(perFrameData,channelFlags);
				return fb->endFrame(renderer->errorThreshold);
			}
			std::string toString() const override {
				return "OVITOTiledLoadBalancer";
			}
		private:
			std::function<bool(int,int,int,int)> _progressCallback;			
		};
		auto loadBalancer = std::make_unique<OVITOTiledLoadBalancer>();
		loadBalancer->setProgressCallback([&osp_fb,frameBuffer,imgSize,&promise,bgColorWithAlpha,this](int x1, int y1, int x2, int y2) {
			// Access framebuffer data and copy it to our own framebuffer.
			uchar* fb = (uchar*)osp_fb.map(OSP_FB_COLOR);
			OVITO_ASSERT(frameBuffer->image().format() == QImage::Format_ARGB32);
			int bperline = renderSettings()->outputImageWidth() * 4;
			for(int y = y1; y < y2; y++) {
				uchar* dst = frameBuffer->image().scanLine(frameBuffer->image().height() - 1 - y) + x1 * 4;
				uchar* src = fb + y*bperline + x1 * 4;
				for(int x = x1; x < x2; x++, dst += 4, src += 4) {
					if(bgColorWithAlpha.a() == 0) {
						dst[0] = src[2];
						dst[1] = src[1];
						dst[2] = src[0];
						dst[3] = src[3];
					}
					else {
						// Compose colors ("source over" mode).
						FloatType srcAlpha = (FloatType)src[3] / 255;
						dst[0] = (uchar)(((FloatType(1) - srcAlpha) * bgColorWithAlpha.b() * 255 + (FloatType)src[2] * srcAlpha));
						dst[1] = (uchar)(((FloatType(1) - srcAlpha) * bgColorWithAlpha.g() * 255 + (FloatType)src[1] * srcAlpha));
						dst[2] = (uchar)(((FloatType(1) - srcAlpha) * bgColorWithAlpha.r() * 255 + (FloatType)src[0] * srcAlpha));
						dst[3] = 255;
					}
				}
			}
			frameBuffer->update(QRect(x1, frameBuffer->image().height() - y2, x2 - x1, y2 - y1));
			osp_fb.unmap(fb);
			return promise.incrementProgressValue((x2-x1) * (y2-y1));
		});
		ospray::TiledLoadBalancer::instance = std::move(loadBalancer);
			
		promise.beginProgressSubSteps(refinementIterations());
		for(int iteration = 0; iteration < refinementIterations() && !promise.isCanceled(); iteration++) {
			if(iteration != 0) promise.nextProgressSubStep();
			promise.setProgressText(tr("Rendering image (pass %1 of %2)").arg(iteration+1).arg(refinementIterations()));
			promise.setProgressMaximum(imgSize.x * imgSize.y);
			renderer.renderFrame(osp_fb, OSP_FB_COLOR | OSP_FB_ACCUM);
		}
		promise.endProgressSubSteps();

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
		size_t nspheres = sphereData.size();
		
		OSPReferenceWrapper<ospray::cpp::Geometry> spheres("spheres");
		spheres.set("bytes_per_sphere", (int)sizeof(ospcommon::vec4f));
		spheres.set("offset_radius", (int)sizeof(float) * 3);

		OSPReferenceWrapper<ospray::cpp::Data> data(nspheres, OSP_FLOAT4, sphereData.data());
		data.commit();
		spheres.set("spheres", data);

		data = ospray::cpp::Data(nspheres, OSP_FLOAT4, colorData.data());
		data.commit();
		spheres.set("color", data);
		
		spheres.setMaterial(*_ospMaterial);
		spheres.commit();
		_ospWorld->addGeometry(spheres);
	}
	else if(particleBuffer.particleShape() == ParticlePrimitive::SquareCubicShape || particleBuffer.particleShape() == ParticlePrimitive::BoxShape) {
		// Rendering cubic/box particles.

		// We will pass the particle geometry as a triangle mesh to OSPRay.
		std::vector<Point_3<float>> vertices;
		std::vector<ColorAT<float>> colors;
		std::vector<Vector_3<float>> normals;
		std::vector<int> indices;
		vertices.reserve(particleBuffer.positions().size() * 6 * 4);
		colors.reserve(particleBuffer.positions().size() * 6 * 4);
		normals.reserve(particleBuffer.positions().size() * 6 * 4);
		indices.reserve(particleBuffer.positions().size() * 6 * 2 * 3);
			
		auto shape = particleBuffer.shapes().begin();
		auto shape_end = particleBuffer.shapes().end();
		auto orientation = particleBuffer.orientations().cbegin();
		auto orientation_end = particleBuffer.orientations().cend();
		for(; p != p_end; ++p, ++c, ++r) {
			if(c->a() <= 0) continue;
			const ColorAT<float> color = (ColorAT<float>)*c;
			for(int i = 0; i < 6*4; i++) {
				colors.push_back(color);
			}
			Point_3<float> tp = (Point_3<float>)(tm * (*p));
			QuaternionT<float> quat(0,0,0,1);
			if(orientation != orientation_end) {
				quat = (QuaternionT<float>)*orientation++;
				// Normalize quaternion.
				float c = sqrt(quat.dot(quat));
				if(c <= 1e-9f)
					quat.setIdentity();
				else
					quat /= c;
			}
			Vector_3<float> s((float)*r);
			if(shape != shape_end) {
				s = (Vector_3<float>)*shape++;
				if(s == Vector_3<float>::Zero())
					s = Vector_3<float>(*r);
			}
			const Point_3<float> corners[8] = {
					tp + quat * Vector_3<float>(-s.x(), -s.y(), -s.z()), 
					tp + quat * Vector_3<float>( s.x(), -s.y(), -s.z()),
					tp + quat * Vector_3<float>( s.x(),  s.y(), -s.z()),
					tp + quat * Vector_3<float>(-s.x(),  s.y(), -s.z()),
					tp + quat * Vector_3<float>(-s.x(), -s.y(),  s.z()),
					tp + quat * Vector_3<float>( s.x(), -s.y(),  s.z()),
					tp + quat * Vector_3<float>( s.x(),  s.y(),  s.z()),
					tp + quat * Vector_3<float>(-s.x(),  s.y(),  s.z())
			};
			const Vector_3<float> faceNormals[6] = {
				quat * Vector_3<float>(-1,0,0), quat * Vector_3<float>(1,0,0),
				quat * Vector_3<float>(0,-1,0), quat * Vector_3<float>(0,1,0),
				quat * Vector_3<float>(0,0,-1), quat * Vector_3<float>(0,0,1)
			};
			int baseIndex;

			// -X face
			baseIndex = (int)vertices.size();
			vertices.push_back(corners[0]); vertices.push_back(corners[3]); vertices.push_back(corners[7]); vertices.push_back(corners[4]);
			indices.push_back(baseIndex + 0); indices.push_back(baseIndex + 1); indices.push_back(baseIndex + 2);
			indices.push_back(baseIndex + 0); indices.push_back(baseIndex + 2); indices.push_back(baseIndex + 3);
			for(int i = 0; i < 4; i++) normals.push_back(faceNormals[0]);
			// +X face
			baseIndex = (int)vertices.size();
			vertices.push_back(corners[1]); vertices.push_back(corners[5]); vertices.push_back(corners[6]); vertices.push_back(corners[2]);
			indices.push_back(baseIndex + 0); indices.push_back(baseIndex + 1); indices.push_back(baseIndex + 2);
			indices.push_back(baseIndex + 0); indices.push_back(baseIndex + 2); indices.push_back(baseIndex + 3);
			for(int i = 0; i < 4; i++) normals.push_back(faceNormals[1]);
			// -Y face
			baseIndex = (int)vertices.size();
			vertices.push_back(corners[0]); vertices.push_back(corners[4]); vertices.push_back(corners[5]); vertices.push_back(corners[1]);
			indices.push_back(baseIndex + 0); indices.push_back(baseIndex + 1); indices.push_back(baseIndex + 2);
			indices.push_back(baseIndex + 0); indices.push_back(baseIndex + 2); indices.push_back(baseIndex + 3);
			for(int i = 0; i < 4; i++) normals.push_back(faceNormals[2]);
			// +Y face
			baseIndex = (int)vertices.size();
			vertices.push_back(corners[2]); vertices.push_back(corners[6]); vertices.push_back(corners[7]); vertices.push_back(corners[3]);
			indices.push_back(baseIndex + 0); indices.push_back(baseIndex + 1); indices.push_back(baseIndex + 2);
			indices.push_back(baseIndex + 0); indices.push_back(baseIndex + 2); indices.push_back(baseIndex + 3);
			for(int i = 0; i < 4; i++) normals.push_back(faceNormals[3]);
			// -Z face
			baseIndex = (int)vertices.size();
			vertices.push_back(corners[0]); vertices.push_back(corners[1]); vertices.push_back(corners[2]); vertices.push_back(corners[3]);
			indices.push_back(baseIndex + 0); indices.push_back(baseIndex + 1); indices.push_back(baseIndex + 2);
			indices.push_back(baseIndex + 0); indices.push_back(baseIndex + 2); indices.push_back(baseIndex + 3);
			for(int i = 0; i < 4; i++) normals.push_back(faceNormals[4]);
			// +Z face
			baseIndex = (int)vertices.size();
			vertices.push_back(corners[4]); vertices.push_back(corners[7]); vertices.push_back(corners[6]); vertices.push_back(corners[5]);
			indices.push_back(baseIndex + 0); indices.push_back(baseIndex + 1); indices.push_back(baseIndex + 2);
			indices.push_back(baseIndex + 0); indices.push_back(baseIndex + 2); indices.push_back(baseIndex + 3);
			for(int i = 0; i < 4; i++) normals.push_back(faceNormals[5]);
		}
		OVITO_ASSERT(normals.size() == colors.size());
		OVITO_ASSERT(normals.size() == vertices.size());

		// Note: This for-loop is a workaround for a bug in OSPRay 1.4.2, which crashes when rendering
		// geometry with a color memory buffer whose size exceeds 2^31 bytes. We split up the geometry 
		// into chunks to stay below the 2^31 bytes limit.
		size_t nparticles = (colors.size() / (6 * 4));
		size_t maxChunkSize = ((1ull << 31) / (sizeof(ColorAT<float>) * 6 * 4)) - 1;
		for(size_t chunkOffset = 0; chunkOffset < nparticles; chunkOffset += maxChunkSize) {

			OSPReferenceWrapper<ospray::cpp::Geometry> triangles("triangles");	
			
			size_t chunkSize = std::min(maxChunkSize, nparticles - chunkOffset);
			OSPReferenceWrapper<ospray::cpp::Data> data(chunkSize * 6 * 4, OSP_FLOAT3, vertices.data() + (chunkOffset * 6 * 4));
			data.commit();
			triangles.set("vertex", data);

			data = ospray::cpp::Data(chunkSize * 6 * 4, OSP_FLOAT4, colors.data() + (chunkOffset * 6 * 4));
			data.commit();
			triangles.set("vertex.color", data);

			data = ospray::cpp::Data(chunkSize * 6 * 4, OSP_FLOAT3, normals.data() + (chunkOffset * 6 * 4));
			data.commit();
			triangles.set("vertex.normal", data);
			
			data = ospray::cpp::Data(chunkSize * 6 * 2, OSP_INT3, indices.data() + (chunkOffset * 6 * 3 * 2));
			data.commit();
			triangles.set("index", data);
			
			triangles.setMaterial(*_ospMaterial);
			triangles.commit();
			_ospWorld->addGeometry(triangles);		
		}
	}
	else if(particleBuffer.particleShape() == ParticlePrimitive::EllipsoidShape) {
		// Rendering ellipsoid particles.
		const Matrix3 linear_tm = tm.linear();
		auto shape = particleBuffer.shapes().cbegin();
		auto shape_end = particleBuffer.shapes().cend();
		auto orientation = particleBuffer.orientations().cbegin();
		auto orientation_end = particleBuffer.orientations().cend();
		std::vector<std::array<float,14>> quadricsData(particleBuffer.positions().size());
		std::vector<ospcommon::vec4f> colorData(particleBuffer.positions().size());
		auto quadricIter = quadricsData.begin();
		auto colorIter = colorData.begin();
		for(; p != p_end && shape != shape_end; ++p, ++c, ++shape, ++r) {
			if(c->a() <= 0) continue;
			Point3 tp = tm * (*p);
			Quaternion quat(0,0,0,1);
			if(orientation != orientation_end) {
				quat = *orientation++;
				// Normalize quaternion.
				FloatType c = sqrt(quat.dot(quat));
				if(c == 0)
					quat.setIdentity();
				else
					quat /= c;
			}
			(*quadricIter)[0] = tp.x();
			(*quadricIter)[1] = tp.y();
			(*quadricIter)[2] = tp.z();
			if(shape->x() != 0 && shape->y() != 0 && shape->z() != 0) {
				Matrix3 qmat(FloatType(1)/(shape->x()*shape->x()), 0, 0,
						     0, FloatType(1)/(shape->y()*shape->y()), 0,
						     0, 0, FloatType(1)/(shape->z()*shape->z()));
				Matrix3 rot = linear_tm * Matrix3::rotation(quat);
				Matrix3 quadric = rot * qmat * rot.transposed();
				(*quadricIter)[3] = std::max(shape->x(), std::max(shape->y(), shape->z()));
				(*quadricIter)[4] = quadric(0,0);
				(*quadricIter)[5] = quadric(0,1);
				(*quadricIter)[6] = quadric(0,2);
				(*quadricIter)[7] = 0;
				(*quadricIter)[8] =	quadric(1,1);
				(*quadricIter)[9] = quadric(1,2);
				(*quadricIter)[10] = 0;
				(*quadricIter)[11] = quadric(2,2);
				(*quadricIter)[12] = 0;
				(*quadricIter)[13] = -1;
			}
			else {
				(*quadricIter)[3] = *r;
				(*quadricIter)[4] = FloatType(1)/((*r)*(*r));
				(*quadricIter)[5] = 0;
				(*quadricIter)[6] = 0;
				(*quadricIter)[7] = 0;
				(*quadricIter)[8] =	FloatType(1)/((*r)*(*r));
				(*quadricIter)[9] = 0;
				(*quadricIter)[10] = 0;
				(*quadricIter)[11] = FloatType(1)/((*r)*(*r));
				(*quadricIter)[12] = 0;
				(*quadricIter)[13] = -1;
			}
			(*colorIter)[0] = c->r();
			(*colorIter)[1] = c->g();
			(*colorIter)[2] = c->b();
			(*colorIter)[3] = c->a();
			++quadricIter;
			++colorIter;		
		}
		size_t nquadrics = quadricIter - quadricsData.begin();
		if(nquadrics == 0) return;
		
		// Note: This for-loop is a workaround for a bug in OSPRay 1.4.2, which crashes when rendering
		// geometry with a color memory buffer whose size exceeds 2^31 bytes. We split up the geometry 
		// into chunks to stay below the 2^31 bytes limit.
		size_t maxChunkSize = ((1ull << 31) / sizeof(std::array<float,14>)) - 1;
		for(size_t chunkOffset = 0; chunkOffset < nquadrics; chunkOffset += maxChunkSize) {
		
			OSPReferenceWrapper<ospray::cpp::Geometry> quadrics("quadrics");

			size_t chunkSize = std::min(maxChunkSize, nquadrics - chunkOffset);
			OSPReferenceWrapper<ospray::cpp::Data> data(chunkSize * 14, OSP_FLOAT, quadricsData.data() + chunkOffset);
			data.commit();
			quadrics.set("quadrics", data);

			data = ospray::cpp::Data(chunkSize, OSP_FLOAT4, colorData.data() + chunkOffset);
			data.commit();
			quadrics.set("color", data);
			
			quadrics.setMaterial(*_ospMaterial);
			quadrics.commit();
			_ospWorld->addGeometry(quadrics);		
		}
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
	std::vector<std::array<float,7>> discData(arrowBuffer.elements().size()*2);
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
		Point3 tp = tm * element.pos;
		Vector3 ta;
		if(arrowBuffer.shape() == ArrowPrimitive::CylinderShape) {
			ta = tm * element.dir;
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
			(*discIter)[3] = normal.x();
			(*discIter)[4] = normal.y();
			(*discIter)[5] = normal.z();
			(*discIter)[6] = element.width;
			(*discColorIter)[0] = element.color.r();
			(*discColorIter)[1] = element.color.g();
			(*discColorIter)[2] = element.color.b();
			(*discColorIter)[3] = element.color.a();
			++discIter;
			++discColorIter;			
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
	size_t ncylinders = cylIter - cylData.begin();
	if(ncylinders != 0) {
		OSPReferenceWrapper<ospray::cpp::Geometry> cylinders("cylinders");
		cylinders.set("bytes_per_cylinder", (int)sizeof(float) * 7);
		cylinders.set("offset_radius", (int)sizeof(float) * 6);

		OSPReferenceWrapper<ospray::cpp::Data> data(ncylinders * 7, OSP_FLOAT, cylData.data());
		data.commit();
		cylinders.set("cylinders", data);

		data = ospray::cpp::Data(ncylinders, OSP_FLOAT4, colorData.data());
		data.commit();
		cylinders.set("color", data);
		
		cylinders.setMaterial(*_ospMaterial);
		cylinders.commit();
		_ospWorld->addGeometry(cylinders);
	}

	size_t ndiscs = discIter - discData.begin();
	if(ndiscs != 0) {
		OSPReferenceWrapper<ospray::cpp::Geometry> discs("discs");
		discs.set("bytes_per_disc", (int)sizeof(float) * 7);
		discs.set("offset_center", (int)sizeof(float) * 0);
		discs.set("offset_normal", (int)sizeof(float) * 3);
		discs.set("offset_radius", (int)sizeof(float) * 6);

		OSPReferenceWrapper<ospray::cpp::Data> data(ndiscs * 7, OSP_FLOAT, discData.data());
		data.commit();
		discs.set("discs", data);

		data = ospray::cpp::Data(ndiscs, OSP_FLOAT4, discColorData.data());
		data.commit();
		discs.set("color", data);
		
		discs.setMaterial(*_ospMaterial);
		discs.commit();
		_ospWorld->addGeometry(discs);
	}

	size_t ncones = coneIter - coneData.begin();
	if(ncones != 0) {
		OSPReferenceWrapper<ospray::cpp::Geometry> cones("cones");
		cones.set("bytes_per_cone", (int)sizeof(float) * 7);
		cones.set("offset_center", (int)sizeof(float) * 0);
		cones.set("offset_axis", (int)sizeof(float) * 3);
		cones.set("offset_radius", (int)sizeof(float) * 6);

		OSPReferenceWrapper<ospray::cpp::Data> data(ncones * 7, OSP_FLOAT, coneData.data());
		data.commit();
		cones.set("cones", data);

		data = ospray::cpp::Data(ncones, OSP_FLOAT4, coneColorData.data());
		data.commit();
		cones.set("color", data);
		
		cones.setMaterial(*_ospMaterial);
		cones.commit();
		_ospWorld->addGeometry(cones);
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

	OSPReferenceWrapper<ospray::cpp::Geometry> triangles("triangles");	
		
	OSPReferenceWrapper<ospray::cpp::Data> data(positions.size(), OSP_FLOAT3, positions.data());
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
	
	triangles.setMaterial(*_ospMaterial);
	triangles.commit();
	_ospWorld->addGeometry(triangles);
}

}	// End of namespace
}	// End of namespace
