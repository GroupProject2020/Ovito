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


#include <core/Core.h>
#include <core/utilities/concurrent/Task.h>
#include "Modifier.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief Base class for modifiers that compute their results in a background thread.
 */
class OVITO_CORE_EXPORT AsynchronousModifier : public Modifier
{
	Q_OBJECT
	OVITO_CLASS(AsynchronousModifier)
	
public:

	/**
	 * Base class for data structures holding the results of an asynchronous modifier computation.
	 */
	class OVITO_CORE_EXPORT ComputeEngineResults 
	{
	public:

		/// Destructor.
		virtual ~ComputeEngineResults() = default;

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) = 0;

		/// Indicates whether the computation results stored in this object may be 
		/// reused and tentatively applied to changing pipeline flow states without recomputation.
		virtual bool isReapplicable() { return true; }
	};

	/// A managed pointer to a ComputeEngineResults instance.
	using ComputeEngineResultsPtr = std::shared_ptr<ComputeEngineResults>;

	/**
	 * Abstract base class for compute engines of AsynchronousModifier implementaAsynchronousModifiertions.
	 */
	class OVITO_CORE_EXPORT ComputeEngine : public AsynchronousTask<ComputeEngineResultsPtr>
	{
	public:

		/// Constructs a new compute engine.
		ComputeEngine(const TimeInterval& validityInterval) :
			_validityInterval(validityInterval) {}

		/// Destructor.
		virtual ~ComputeEngine();

		/// Returns the validity period of the results computed by this engine.
		const TimeInterval& validityInterval() const { return _validityInterval; }

		/// Changes the stored validity period of the results computed by this engine.
		void setValidityInterval(const TimeInterval& iv) { _validityInterval = iv; }

	private:

		/// The validity period of the results computed by this engine.
		TimeInterval _validityInterval;
	};

	/// A managed pointer to a ComputeEngine instance.
	using ComputeEnginePtr = std::shared_ptr<ComputeEngine>;

public:

	/// Constructor.
	AsynchronousModifier(DataSet* dataset);

	/// Create a new modifier application that refers to this modifier instance.
	virtual OORef<ModifierApplication> createModifierApplication() override;
	
	/// Asks the object for the result of the data pipeline.
	virtual Future<PipelineFlowState> evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

	/// Modifies the input data in an immediate, preliminary way.
	virtual PipelineFlowState evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

	/// Decides whether a preliminary viewport update is performed every time the modifier 
	/// itself changes. For asynchronous modifier this is disabled.
	virtual bool performPreliminaryUpdateAfterChange() override { return false; }
	
protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) = 0;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


