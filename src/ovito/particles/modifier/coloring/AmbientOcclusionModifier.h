////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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


#include <ovito/particles/Particles.h>
#include <ovito/particles/util/ParticleOrderingFingerprint.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>

#include <QOffscreenSurface>

namespace Ovito { namespace Particles {

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
		virtual bool isApplicableTo(const DataCollection& input) const override;
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

		/// Destructor.
		virtual ~AmbientOcclusionEngine();
		
		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

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
	virtual Future<ComputeEnginePtr> createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// This controls the intensity of the shading effect.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, intensity, setIntensity);

	/// Controls the quality of the lighting computation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, samplingCount, setSamplingCount);

	/// Controls the resolution of the offscreen rendering buffer.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, bufferResolution, setBufferResolution);
};

}	// End of namespace
}	// End of namespace
