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

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticlesVis.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <core/app/Application.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/utilities/units/UnitsManager.h>
#include <opengl_renderer/OpenGLSceneRenderer.h>
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
	_intensity(0.7f), _samplingCount(40), _bufferResolution(3)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool AmbientOcclusionModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
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
	ParticleInputHelper ph(dataset(), input);
	ParticleProperty* posProperty = ph.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);
	ParticleProperty* typeProperty = ph.inputStandardProperty<ParticleProperty>(ParticleProperty::TypeProperty);
	ParticleProperty* radiusProperty = ph.inputStandardProperty<ParticleProperty>(ParticleProperty::RadiusProperty);
	ParticleProperty* shapeProperty = ph.inputStandardProperty<ParticleProperty>(ParticleProperty::AsphericalShapeProperty);

	// Compute bounding box of input particles.
	Box3 boundingBox;
	for(DataVis* vis : posProperty->visElements()) {
		if(ParticlesVis* particleVis = dynamic_object_cast<ParticlesVis>(vis)) {
			boundingBox.addBox(particleVis->particleBoundingBox(posProperty, typeProperty, radiusProperty, shapeProperty));
		}
	}

	// The render buffer resolution.
	int res = std::min(std::max(bufferResolution(), 0), (int)MAX_AO_RENDER_BUFFER_RESOLUTION);
	int resolution = (128 << res);

	TimeInterval validityInterval = input.stateValidity();
	std::vector<FloatType> radii = ph.inputParticleRadii(time, validityInterval);

	// Create the offscreen surface for rendering.
	auto offscreenSurface = std::make_unique<QOffscreenSurface>();
	offscreenSurface->setFormat(OpenGLSceneRenderer::getDefaultSurfaceFormat());
	offscreenSurface->create();

	// Create the AmbientOcclusionRenderer instance.
	OORef<AmbientOcclusionRenderer> renderer(new AmbientOcclusionRenderer(dataset(), QSize(resolution, resolution), *offscreenSurface));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	auto engine = std::make_shared<AmbientOcclusionEngine>(validityInterval, resolution, samplingCount(), posProperty->storage(), boundingBox, std::move(radii), renderer);

	// Make sure the renderer and the offscreen surface stay alive until the compute engine finished.
	engine->finally(dataset()->executor(), [offscreenSurface = std::move(offscreenSurface), renderer = std::move(renderer)]() {});

	return engine;
}

/******************************************************************************
* Compute engine constructor.
******************************************************************************/
AmbientOcclusionModifier::AmbientOcclusionEngine::AmbientOcclusionEngine(const TimeInterval& validityInterval, int resolution, int samplingCount, PropertyPtr positions, 
		const Box3& boundingBox, std::vector<FloatType> particleRadii, AmbientOcclusionRenderer* renderer) :
	_resolution(resolution),
	_samplingCount(samplingCount),
	_positions(positions),
	_boundingBox(boundingBox),
	_particleRadii(std::move(particleRadii)),
	_renderer(renderer),
	_results(std::make_shared<AmbientOcclusionResults>(validityInterval, positions->size()))
{
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void AmbientOcclusionModifier::AmbientOcclusionEngine::perform()
{
	if(_boundingBox.isEmpty() || positions()->size() == 0)
		throw Exception(tr("Modifier input is degenerate or contains no particles."));

	setProgressText(tr("Computing ambient occlusion"));

	_renderer->startRender(nullptr, nullptr);
	try {
		// The buffered particle geometry used to render the particles.
		std::shared_ptr<ParticlePrimitive> particleBuffer;

		setProgressMaximum(_samplingCount);
		for(int sample = 0; sample < _samplingCount; sample++) {
			if(!setProgressValue(sample))
				break;

			// Generate lighting direction on unit sphere.
			FloatType y = (FloatType)sample * 2 / _samplingCount - FloatType(1) + FloatType(1) / _samplingCount;
			//FloatType r = sqrt(FloatType(1) - y * y);
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
			projParams.fieldOfView = 0.5f * _boundingBox.size().length();
			projParams.znear = -bb.maxc.z();
			projParams.zfar  = std::max(-bb.minc.z(), projParams.znear + 1.0f);
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
					particleBuffer->setParticlePositions(positions()->constDataPoint3());
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
			FloatType* brightnessValues = _results->brightness()->dataFloat();
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
					OVITO_ASSERT(particleIndex < positions()->size());
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

	if(!isCanceled()) {
		setProgressValue(_samplingCount);
		// Normalize brightness values.
		FloatType maxBrightness = *std::max_element(_results->brightness()->constDataFloat(), _results->brightness()->constDataFloat() + _results->brightness()->size());
		if(maxBrightness != 0) {
			for(FloatType& b : _results->brightness()->floatRange()) {
				b /= maxBrightness;
			}
		}
	}

	// Return the results of the compute engine.
	setResult(std::move(_results));
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState AmbientOcclusionModifier::AmbientOcclusionResults::apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	OVITO_ASSERT(brightness());

	AmbientOcclusionModifier* modifier = static_object_cast<AmbientOcclusionModifier>(modApp->modifier());
	OVITO_ASSERT(modifier);

	PipelineFlowState output = input;
	ParticleInputHelper pih(modApp->dataset(), input);
	ParticleOutputHelper poh(modApp->dataset(), output);
	
	if(poh.outputParticleCount() != brightness()->size())
		modifier->throwException(tr("The number of input particles has changed. The stored results have become invalid."));
	
	// Get effective intensity.
	FloatType intens = std::min(std::max(modifier->intensity(), FloatType(0)), FloatType(1));

	// Get output property object.
	ParticleProperty* colorProperty = poh.outputStandardProperty<ParticleProperty>(ParticleProperty::ColorProperty);
	OVITO_ASSERT(colorProperty->size() == brightness()->size());

	std::vector<Color> existingColors = pih.inputParticleColors(time, output.mutableStateValidity());
	OVITO_ASSERT(colorProperty->size() == existingColors.size());
	const FloatType* b = brightness()->constDataFloat();
	Color* c = colorProperty->dataColor();
	Color* c_end = c + colorProperty->size();
	auto c_in = existingColors.cbegin();
	for(; c != c_end; ++b, ++c, ++c_in) {
		FloatType factor = FloatType(1) - intens + (*b);
		if(factor < FloatType(1))
			*c = factor * (*c_in);
		else
			*c = *c_in;
	}

	return output;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
