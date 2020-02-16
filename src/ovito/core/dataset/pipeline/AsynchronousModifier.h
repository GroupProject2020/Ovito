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


#include <ovito/core/Core.h>
#include <ovito/core/utilities/concurrent/AsynchronousTask.h>
#include "Modifier.h"

namespace Ovito {

/**
 * \brief Base class for modifiers that compute their results in a background thread.
 */
class OVITO_CORE_EXPORT AsynchronousModifier : public Modifier
{
	Q_OBJECT
	OVITO_CLASS(AsynchronousModifier)

public:

	/**
	 * Abstract base class for compute engines.
	 */
	class OVITO_CORE_EXPORT ComputeEngine : public AsynchronousTaskBase
	{
	public:

		/// Constructor.
		explicit ComputeEngine(const TimeInterval& validityInterval = TimeInterval::infinite()) :
			_validityInterval(validityInterval) {}

#ifdef Q_OS_LINUX
		/// Destructor.
		virtual ~ComputeEngine();
#endif

		/// Computes the modifier's results.
		virtual void perform() = 0;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) = 0;

		/// Returns the validity period of the stored results.
		const TimeInterval& validityInterval() const { return _validityInterval; }

		/// Changes the stored validity period of the results.
		void setValidityInterval(const TimeInterval& iv) { _validityInterval = iv; }

	private:

		/// The validity period of the stored results.
		TimeInterval _validityInterval;
	};

	/// A managed pointer to a ComputeEngine instance.
	using ComputeEnginePtr = std::shared_ptr<ComputeEngine>;

public:

	/// Constructor.
	AsynchronousModifier(DataSet* dataset);

	/// Modifies the input data synchronously.
	virtual void evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// Suppress preliminary viewport updates when a parameter of the asynchronous modifier changes.
	virtual bool performPreliminaryUpdateAfterChange() override { return false; }

	/// This method indicates whether outdated computation results should be immediately discarded
	/// whenever the inputs of the modifier changes. By default, the method returns false to indicate
	/// that the results should be kept as long as a new computation is in progress. During this
	/// transient phase, the old resuls may still be used by the pipeline for preliminary viewport updates.
	virtual bool discardResultsOnInputChange() const { return false; }

	/// This method indicates whether cached computation results of the modifier should be discarded whenever
	/// a parameter of the modifier changes. The default implementation returns true. Subclasses can override
	/// this method if the asynchronous computation results do not depend on certain parameters and their change
	/// should not trigger a recomputation.
	virtual bool discardResultsOnModifierChange(const PropertyFieldEvent& event) const { return true; }

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;
	
	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Asks the object for the result of the data pipeline.
	virtual Future<PipelineFlowState> evaluate(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input) = 0;
};

// Export this class template specialization from the DLL under Windows.
extern template class OVITO_CORE_EXPORT Future<AsynchronousModifier::ComputeEnginePtr>;

}	// End of namespace
