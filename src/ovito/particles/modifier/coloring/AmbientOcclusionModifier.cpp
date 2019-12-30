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
#include <ovito/particles/objects/ParticlesVis.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/opengl/OpenGLSceneRenderer.h>
#include "AmbientOcclusionModifier.h"
#include "AmbientOcclusionRenderer.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Coloring)

IMPLEMENT_OVITO_CLASS(AmbientOcclusionModifier);
DEFINE_PROPERTY_FIELD(AmbientOcclusionModifier, intensity);
DEFINE_PROPERTY_FIELD(AmbientOcclusionModifier, samplingCount);
DEFINE_PROPERTY_FIELD(AmbientOcclusionModifier, bufferResolution);
SET_PROPERTY_FIELD_LABEL(AmbientOcclusionModifier, intensity, "Shading intensity");
SET_PROPERTY_FIELD_LABEL(AmbientOcclusionModifier, samplingCount, "Number of exposure samples");
SET_PROPERTY_FIELD_LABEL(AmbientOcclusionModifier, bufferResolution, "Render buffer resolution");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(AmbientOcclusionModifier, intensity, PercentParameterUnit, 0, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(AmbientOcclusionModifier, samplingCount, IntegerParameterUnit, 3, 2000);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(AmbientOcclusionModifier, bufferResolution, IntegerParameterUnit, 1, AmbientOcclusionModifier::MAX_AO_RENDER_BUFFER_RESOLUTION);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AmbientOcclusionModifier::AmbientOcclusionModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_intensity(0.7),
	_samplingCount(40),
	_bufferResolution(3)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool AmbientOcclusionModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> AmbientOcclusionModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	if(Application::instance()->headlessMode())
		throwException(tr("The ambient occlusion modifier requires OpenGL support and cannot be used when program is running in headless mode. "
						  "Please run program on a machine where access to graphics hardware is available."));

	// Get modifier input.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const PropertyObject* typeProperty = particles->getProperty(ParticlesObject::TypeProperty);
	const PropertyObject* radiusProperty = particles->getProperty(ParticlesObject::RadiusProperty);
	const PropertyObject* shapeProperty = particles->getProperty(ParticlesObject::AsphericalShapeProperty);

	// Compute bounding box of input particles.
	Box3 boundingBox;
	if(ParticlesVis* particleVis = particles->visElement<ParticlesVis>()) {
		boundingBox.addBox(particleVis->particleBoundingBox(posProperty, typeProperty, radiusProperty, shapeProperty, true));
	}

	// The render buffer resolution.
	int res = std::min(std::max(bufferResolution(), 0), (int)MAX_AO_RENDER_BUFFER_RESOLUTION);
	int resolution = (128 << res);

	TimeInterval validityInterval = input.stateValidity();
	std::vector<FloatType> radii = particles->inputParticleRadii();

	// Create the offscreen surface for rendering.
	auto offscreenSurface = std::make_unique<QOffscreenSurface>();
	offscreenSurface->setFormat(OpenGLSceneRenderer::getDefaultSurfaceFormat());
	offscreenSurface->create();

	// Create the AmbientOcclusionRenderer instance.
	OORef<AmbientOcclusionRenderer> renderer(new AmbientOcclusionRenderer(dataset(), QSize(resolution, resolution), *offscreenSurface));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	auto engine = std::make_shared<AmbientOcclusionEngine>(validityInterval, particles, resolution, samplingCount(), posProperty->storage(), boundingBox, std::move(radii), renderer);

	// Make sure the renderer and the offscreen surface stay alive until the compute engine finished.
	engine->task()->finally(dataset()->executor(), [offscreenSurface = std::move(offscreenSurface), renderer = std::move(renderer)]() {});

	return engine;
}

/******************************************************************************
* Compute engine constructor.
******************************************************************************/
AmbientOcclusionModifier::AmbientOcclusionEngine::AmbientOcclusionEngine(const TimeInterval& validityInterval, ParticleOrderingFingerprint fingerprint, int resolution, int samplingCount, PropertyPtr positions,
		const Box3& boundingBox, std::vector<FloatType> particleRadii, AmbientOcclusionRenderer* renderer) :
	ComputeEngine(validityInterval),
	_resolution(resolution),
	_samplingCount(samplingCount),
	_positions(positions),
	_boundingBox(boundingBox),
	_particleRadii(std::move(particleRadii)),
	_renderer(renderer),
	_brightness(std::make_shared<PropertyStorage>(fingerprint.particleCount(), PropertyStorage::Float, 1, 0, QStringLiteral("Brightness"), true)),
	_inputFingerprint(std::move(fingerprint))
{
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void AmbientOcclusionModifier::AmbientOcclusionEngine::perform()
{
	if(positions()->size() == 0)
		return;
	if(_boundingBox.isEmpty())
		throw Exception(tr("Modifier input is degenerate or contains no particles."));

	task()->setProgressText(tr("Computing ambient occlusion"));

	_renderer->startRender(nullptr, nullptr);
	try {
		// The buffered particle geometry used to render the particles.
		std::shared_ptr<ParticlePrimitive> particleBuffer;

		task()->setProgressMaximum(_samplingCount);
		for(int sample = 0; sample < _samplingCount; sample++) {
			if(!task()->setProgressValue(sample))
				break;

			// Generate lighting direction on unit sphere.
			FloatType y = (FloatType)sample * 2 / _samplingCount - FloatType(1) + FloatType(1) / _samplingCount;
			FloatType phi = (FloatType)sample * FLOATTYPE_PI * (FloatType(3) - sqrt(FloatType(5)));
			Vector3 dir(cos(phi), y, sin(phi));

			// Set up view projection.
			ViewProjectionParameters projParams;
			projParams.viewMatrix = AffineTransformation::lookAlong(_boundingBox.center(), dir, Vector3(0,0,1));

			// Transform bounding box to camera space.
			Box3 bb = _boundingBox.transformed(projParams.viewMatrix).centerScale(FloatType(1.01));

			// Complete projection parameters.
			projParams.aspectRatio = 1;
			projParams.isPerspective = false;
			projParams.inverseViewMatrix = projParams.viewMatrix.inverse();
			projParams.fieldOfView = FloatType(0.5) * _boundingBox.size().length();
			projParams.znear = -bb.maxc.z();
			projParams.zfar  = std::max(-bb.minc.z(), projParams.znear + FloatType(1));
			projParams.projectionMatrix = Matrix4::ortho(-projParams.fieldOfView, projParams.fieldOfView,
								-projParams.fieldOfView, projParams.fieldOfView,
								projParams.znear, projParams.zfar);
			projParams.inverseProjectionMatrix = projParams.projectionMatrix.inverse();
			projParams.validityInterval = TimeInterval::infinite();

			_renderer->beginFrame(0, projParams, nullptr);
			_renderer->setWorldTransform(AffineTransformation::Identity());
			try {
				// Create particle buffer.
				if(!particleBuffer || !particleBuffer->isValid(_renderer)) {
					particleBuffer = _renderer->createParticlePrimitive(ParticlePrimitive::FlatShading, ParticlePrimitive::LowQuality, ParticlePrimitive::SphericalShape, false);
					particleBuffer->setSize(positions()->size());
					particleBuffer->setParticlePositions(ConstPropertyAccess<Point3>(positions()).cbegin());
					particleBuffer->setParticleRadii(_particleRadii.data());
				}
				particleBuffer->render(_renderer);
			}
			catch(...) {
				_renderer->endFrame(false);
				throw;
			}
			_renderer->endFrame(true);

			// Extract brightness values from rendered image.
			const QImage image = _renderer->image();
			PropertyAccess<FloatType> brightnessValues(brightness());
			for(int y = 0; y < _resolution; y++) {
				const QRgb* pixel = reinterpret_cast<const QRgb*>(image.scanLine(y));
				for(int x = 0; x < _resolution; x++, ++pixel) {
					quint32 red = qRed(*pixel);
					quint32 green = qGreen(*pixel);
					quint32 blue = qBlue(*pixel);
					quint32 alpha = qAlpha(*pixel);
					quint32 id = red + (green << 8) + (blue << 16) + (alpha << 24);
					if(id == 0)
						continue;
					quint32 particleIndex = id - 1;
					OVITO_ASSERT(particleIndex < brightnessValues.size());
					brightnessValues[particleIndex] += 1;
				}
			}
		}
	}
	catch(...) {
		_renderer->endRender();
		throw;
	}
	_renderer->endRender();

	if(!task()->isCanceled()) {
		task()->setProgressValue(_samplingCount);
		// Normalize brightness values by particle area.
		auto r = _particleRadii.cbegin();
		PropertyAccess<FloatType> brightnessValues(brightness());
		for(FloatType& b : brightnessValues) {
			if(*r != 0)
				b /= (*r) * (*r);
			++r;
		}
		// Normalize brightness values by global maximum.
		FloatType maxBrightness = *boost::max_element(brightnessValues);
		if(maxBrightness != 0) {
			for(FloatType& b : brightnessValues) {
				b /= maxBrightness;
			}
		}
	}
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void AmbientOcclusionModifier::AmbientOcclusionEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	AmbientOcclusionModifier* modifier = static_object_cast<AmbientOcclusionModifier>(modApp->modifier());
	OVITO_ASSERT(modifier);

	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();
	if(_inputFingerprint.hasChanged(particles))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));
	OVITO_ASSERT(brightness() && particles->elementCount() == brightness()->size());

	// Get effective intensity.
	FloatType intens = qBound(FloatType(0), modifier->intensity(), FloatType(1));

	// Get output property object.
	ConstPropertyAccess<FloatType> brightnessValues(brightness());
	PropertyAccess<Color> colorProperty = particles->createProperty(ParticlesObject::ColorProperty, true, {particles});
	const FloatType* b = brightnessValues.cbegin();
	for(Color& c : colorProperty) {
		FloatType factor = FloatType(1) - intens + (*b);
		if(factor < FloatType(1))
			c = c * factor;
		++b;
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
