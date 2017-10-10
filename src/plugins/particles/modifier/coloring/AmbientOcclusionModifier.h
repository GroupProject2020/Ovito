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

	/// Holds the modifier's results.
	class AmbientOcclusionResults : public ComputeEngineResults
	{
	public:

		/// Constructor.
		AmbientOcclusionResults(const TimeInterval& validityInterval, size_t particleCount) : ComputeEngineResults(validityInterval),
			_brightness(std::make_shared<PropertyStorage>(particleCount, PropertyStorage::Float, 1, 0, tr("Brightness"), true)) {}

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
	
		/// Returns the property storage that contains the computed per-particle brightness values.
		const PropertyPtr& brightness() const { return _brightness; }
		
	private:
		
		PropertyPtr _brightness;	
	};

	/// Computes the modifier's results.
	class AmbientOcclusionEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		AmbientOcclusionEngine(const TimeInterval& validityInterval, int resolution, int samplingCount, PropertyPtr positions, 
			const Box3& boundingBox, std::vector<FloatType> particleRadii, AmbientOcclusionRenderer* renderer);

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Returns the property storage that contains the input particle positions.
		const PropertyPtr& positions() const { return _positions; }

	private:

		AmbientOcclusionRenderer* _renderer;
		const int _resolution;
		const int _samplingCount;
		const PropertyPtr _positions;
		const Box3 _boundingBox;
		const std::vector<FloatType> _particleRadii;
		std::shared_ptr<AmbientOcclusionResults> _results;
	};

public:

	/// Constructor.
	Q_INVOKABLE AmbientOcclusionModifier(DataSet* dataset);

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


