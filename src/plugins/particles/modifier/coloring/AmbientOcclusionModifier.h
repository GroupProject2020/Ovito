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

#pragma once


#include <plugins/particles/Particles.h>
#include <plugins/particles/util/ParticleOrderingFingerprint.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>

#include <QOffscreenSurface>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Coloring)

class AmbientOcclusionRenderer;		// defined in AmbientOcclusionRenderer.h

/**
 * \brief Calculates ambient occlusion lighting for particles.
 */
class OVITO_PARTICLES_EXPORT AmbientOcclusionModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class AmbientOcclusionModifierClass : public ModifierClass 
	{
	public:

		/// Inherit constructor from base class.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(AmbientOcclusionModifier, AmbientOcclusionModifierClass)

	Q_CLASSINFO("DisplayName", "Ambient occlusion");
	Q_CLASSINFO("ModifierCategory", "Coloring");

public:

	enum { MAX_AO_RENDER_BUFFER_RESOLUTION = 4 };

	/// Computes the modifier's results.
	class AmbientOcclusionEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		AmbientOcclusionEngine(const TimeInterval& validityInterval, ParticleOrderingFingerprint fingerprint, int resolution, int samplingCount, PropertyPtr positions, 
			const Box3& boundingBox, std::vector<FloatType> particleRadii, AmbientOcclusionRenderer* renderer);

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_positions.reset();
			decltype(_particleRadii){}.swap(_particleRadii);
			ComputeEngine::cleanup();
		}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
	
		/// Returns the property storage that contains the computed per-particle brightness values.
		const PropertyPtr& brightness() const { return _brightness; }

		/// Returns the property storage that contains the input particle positions.
		const PropertyPtr& positions() const { return _positions; }

	private:

		AmbientOcclusionRenderer* _renderer;
		const int _resolution;
		const int _samplingCount;
		PropertyPtr _positions;
		const Box3 _boundingBox;
		std::vector<FloatType> _particleRadii;
		PropertyPtr _brightness;
		ParticleOrderingFingerprint _inputFingerprint;
	};

public:

	/// Constructor.
	Q_INVOKABLE AmbientOcclusionModifier(DataSet* dataset);

	/// This method indicates whether cached computation results of the modifier should be discarded whenever
	/// a parameter of the modifier changes.
	virtual bool discardResultsOnModifierChange(const PropertyFieldEvent& event) const override { 
		// Avoid a full recomputation when the user changes the intensity parameter.
		if(event.field() == &PROPERTY_FIELD(intensity)) return false;
		return AsynchronousModifier::discardResultsOnModifierChange(event);
	}

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// This controls the intensity of the shading effect.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, intensity, setIntensity);

	/// Controls the quality of the lighting computation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, samplingCount, setSamplingCount);

	/// Controls the resolution of the offscreen rendering buffer.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, bufferResolution, setBufferResolution);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


