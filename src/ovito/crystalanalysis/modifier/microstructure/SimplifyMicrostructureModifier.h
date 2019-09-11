///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/Microstructure.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Reduces the complexity of a microstructure model.
 */
class OVITO_CRYSTALANALYSIS_EXPORT SimplifyMicrostructureModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class OOMetaClass : public AsynchronousModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(SimplifyMicrostructureModifier, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Simplify microstructure");
	Q_CLASSINFO("ModifierCategory", "Modification");

public:

	/// Constructor.
	Q_INVOKABLE SimplifyMicrostructureModifier(DataSet* dataset);

	/// Decides whether a preliminary viewport update is performed after the modifier has been
	/// evaluated but before the entire pipeline evaluation is complete.
	/// We suppress such preliminary updates for this modifier, because it produces a microstructure object,
	/// which requires further asynchronous processing before a viewport update makes sense.
	virtual bool performPreliminaryUpdateAfterEvaluation() override { return false; }

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// Computation engine of the modifier.
	class SimplifyMicrostructureEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		SimplifyMicrostructureEngine(const Microstructure* microstructureObj, int smoothingLevel, FloatType kPB, FloatType lambda) :
			_microstructure(microstructureObj),
            _smoothingLevel(smoothingLevel),
            _kPB(kPB),
            _lambda(lambda) {}

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			ComputeEngine::cleanup();
		}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		/// Returns the output microstructure.
		const MicrostructureData& microstructure() const { return _microstructure; }

		/// Returns the input simulation cell.
		const SimulationCell& cell() const { return microstructure().cell(); }

	private:

        /// Performs one iteration of the smoothing algorithm.
        void smoothMeshIteration(FloatType prefactor);

		/// The microstructure modified by the modifier.
		MicrostructureData _microstructure;
        int _smoothingLevel;
        FloatType _kPB;
        FloatType _lambda;
	};

	/// Controls the number of smoothing iterations to perform.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, smoothingLevel, setSmoothingLevel, PROPERTY_FIELD_MEMORIZE);

	/// First control parameter of the smoothing algorithm.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, kPB, setkPB, PROPERTY_FIELD_MEMORIZE);

	/// Second control parameter of the smoothing algorithm.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, lambda, setLambda, PROPERTY_FIELD_MEMORIZE);
};

}	// End of namespace
}	// End of namespace
}	// End of namespace
