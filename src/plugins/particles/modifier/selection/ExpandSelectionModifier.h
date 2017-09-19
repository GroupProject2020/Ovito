///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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
#include <plugins/particles/objects/BondsStorage.h>
#include <core/dataset/data/simcell/SimulationCell.h>
#include <core/dataset/data/properties/PropertyStorage.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)

/**
 * \brief Extends the current particle selection by adding particles to the selection
 *        that are neighbors of an already selected particle.
 */
class OVITO_PARTICLES_EXPORT ExpandSelectionModifier : public AsynchronousModifier
{
public:

	enum ExpansionMode {
		BondedNeighbors,	///< Expands the selection to particles that are bonded to an already selected particle.
		CutoffRange,		///< Expands the selection to particles that within a cutoff range of an already selected particle.
		NearestNeighbors,	///< Expands the selection to the N nearest particles of already selected particles.
	};
	Q_ENUMS(ExpansionMode);

	/// Compile-time constant for the maximum number of nearest neighbors that can be taken into account.
	enum { MAX_NEAREST_NEIGHBORS = 30 };

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE ExpandSelectionModifier(DataSet* dataset);

public:
	
	/// Give this modifier class its own metaclass.
	class OOMetaClass : public AsynchronousModifier::OOMetaClass 
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
	};

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// Holds the modifier's results.
	class ExpandSelectionResults : public ComputeEngineResults
	{
	public:

		/// Constructor.
		ExpandSelectionResults(const ConstPropertyPtr& inputSelection) :
			_outputSelection(std::make_shared<PropertyStorage>(*inputSelection)) {}

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
		
		const PropertyPtr& outputSelection() { return _outputSelection; }
		void setOutputSelection(PropertyPtr ptr) { _outputSelection = std::move(ptr); }
		size_t numSelectedParticlesInput() const { return _numSelectedParticlesInput; }
		size_t numSelectedParticlesOutput() const { return _numSelectedParticlesOutput; }
		void setNumSelectedParticlesInput(size_t count) { _numSelectedParticlesInput = count; }
		void setNumSelectedParticlesOutput(size_t count) { _numSelectedParticlesOutput = count; }
		
	private:

		PropertyPtr _outputSelection;
		size_t _numSelectedParticlesInput;
		size_t _numSelectedParticlesOutput;
	};

	/// The modifier's compute engine.
	class ExpandSelectionEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		ExpandSelectionEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, const SimulationCell& simCell, ConstPropertyPtr inputSelection, int numIterations) :
			ComputeEngine(validityInterval),
			_numIterations(numIterations),
			_positions(std::move(positions)), 
			_simCell(simCell),
			_inputSelection(std::move(inputSelection)),
			_results(std::make_shared<ExpandSelectionResults>(_inputSelection)) { setResult(_results); }

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Performs one iteration of the expansion.
		virtual void expandSelection() = 0;

		const SimulationCell& simCell() const { return _simCell; }

		const ConstPropertyPtr& positions() const { return _positions; }

		const ConstPropertyPtr& inputSelection() const { return _inputSelection; }
		
		/// Returns a pointer to the engine's computation results.
		ExpandSelectionResults* results() const { return _results.get(); }

	protected:

		const int _numIterations;
		const SimulationCell _simCell;
		const ConstPropertyPtr _positions;
		ConstPropertyPtr _inputSelection;
		std::shared_ptr<ExpandSelectionResults> _results;
	};

	/// Computes the expanded selection by using the nearest neighbor criterion.
	class ExpandSelectionNearestEngine : public ExpandSelectionEngine
	{
	public:

		/// Constructor.
		ExpandSelectionNearestEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, const SimulationCell& simCell, ConstPropertyPtr inputSelection, int numIterations, int numNearestNeighbors) :
			ExpandSelectionEngine(validityInterval, std::move(positions), simCell, std::move(inputSelection), numIterations), 
			_numNearestNeighbors(numNearestNeighbors) {}

		/// Expands the selection by one step.
		virtual void expandSelection() override;

	private:

		const int _numNearestNeighbors;
	};

	/// Computes the expanded selection when using a cutoff range criterion.
	class ExpandSelectionCutoffEngine : public ExpandSelectionEngine
	{
	public:

		/// Constructor.
		ExpandSelectionCutoffEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, const SimulationCell& simCell, ConstPropertyPtr inputSelection, int numIterations, FloatType cutoff) :
			ExpandSelectionEngine(validityInterval, std::move(positions), simCell, std::move(inputSelection), numIterations), 
			_cutoffRange(cutoff) {}

		/// Expands the selection by one step.
		virtual void expandSelection() override;

	private:

		const FloatType _cutoffRange;
	};

	/// Computes the expanded selection when using bonds.
	class ExpandSelectionBondedEngine : public ExpandSelectionEngine
	{
	public:

		/// Constructor.
		ExpandSelectionBondedEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, const SimulationCell& simCell, ConstPropertyPtr inputSelection, int numIterations, ConstBondsPtr bonds) :
			ExpandSelectionEngine(validityInterval, std::move(positions), simCell, std::move(inputSelection), numIterations), 
			_bonds(std::move(bonds)) {}

		/// Expands the selection by one step.
		virtual void expandSelection() override;

	private:

		const ConstBondsPtr _bonds;
	};

private:

	/// The expansion mode.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ExpansionMode, mode, setMode);

	/// The selection cutoff range.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, cutoffRange, setCutoffRange);

	/// The number of nearest neighbors to select.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, numNearestNeighbors, setNumNearestNeighbors);

	/// The number of expansion steps to perform.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, numberOfIterations, setNumberOfIterations);

	Q_OBJECT
	OVITO_CLASS

	Q_CLASSINFO("DisplayName", "Expand selection");
	Q_CLASSINFO("ModifierCategory", "Selection");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


