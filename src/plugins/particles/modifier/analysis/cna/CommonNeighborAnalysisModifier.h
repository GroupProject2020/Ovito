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
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <plugins/particles/objects/BondsStorage.h>
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/particles/objects/ParticleProperty.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief A modifier that performs the common neighbor analysis (CNA) to identify
 *        local coordination structures.
 */
class OVITO_PARTICLES_EXPORT CommonNeighborAnalysisModifier : public StructureIdentificationModifier
{
	Q_OBJECT
	OVITO_CLASS(CommonNeighborAnalysisModifier)

	Q_CLASSINFO("DisplayName", "Common neighbor analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

	enum CNAMode {
		FixedCutoffMode,	///< Performs the conventional CNA using a global cutoff radius.
		AdaptiveCutoffMode,	///< Performs the adaptive CNA, which picks an optimal cutoff for each atom.
		BondMode,			///< Performs the CNA based on the existing network of bonds.
	};
	Q_ENUMS(CNAMode);

#ifndef Q_CC_MSVC
	/// The maximum number of neighbor atoms taken into account for the common neighbor analysis.
	static constexpr int MAX_NEIGHBORS = 14;
#else
	enum { MAX_NEIGHBORS = 14 };
#endif

	/// The structure types recognized by the common neighbor analysis.
	enum StructureType {
		OTHER = 0,				//< Unidentified structure
		FCC,					//< Face-centered cubic
		HCP,					//< Hexagonal close-packed
		BCC,					//< Body-centered cubic
		ICO,					//< Icosahedral structure

		NUM_STRUCTURE_TYPES 	//< This just counts the number of defined structure types.
	};
	Q_ENUMS(StructureType);

	/// Pair of neighbor atoms that form a bond (bit-wise storage).
	typedef unsigned int CNAPairBond;

	/**
	 * A bit-flag array indicating which pairs of neighbors are bonded
	 * and which are not.
	 */
	struct NeighborBondArray
	{
		/// Two-dimensional bit array that stores the bonds between neighbors.
		unsigned int neighborArray[32];

		/// Resets all bits.
		NeighborBondArray() {
			memset(neighborArray, 0, sizeof(neighborArray));
		}

		/// Returns whether two nearest neighbors have a bond between them.
		inline bool neighborBond(int neighborIndex1, int neighborIndex2) const {
			OVITO_ASSERT(neighborIndex1 < 32);
			OVITO_ASSERT(neighborIndex2 < 32);
			return (neighborArray[neighborIndex1] & (1<<neighborIndex2));
		}

		/// Sets whether two nearest neighbors have a bond between them.
		inline void setNeighborBond(int neighborIndex1, int neighborIndex2, bool bonded) {
			OVITO_ASSERT(neighborIndex1 < 32);
			OVITO_ASSERT(neighborIndex2 < 32);
			if(bonded) {
				neighborArray[neighborIndex1] |= (1<<neighborIndex2);
				neighborArray[neighborIndex2] |= (1<<neighborIndex1);
			}
			else {
				neighborArray[neighborIndex1] &= ~(1<<neighborIndex2);
				neighborArray[neighborIndex2] &= ~(1<<neighborIndex1);
			}
		}
	};

public:

	/// Constructor.
	Q_INVOKABLE CommonNeighborAnalysisModifier(DataSet* dataset);

	/// Find all atoms that are nearest neighbors of the given pair of atoms.
	static int findCommonNeighbors(const NeighborBondArray& neighborArray, int neighborIndex, unsigned int& commonNeighbors, int numNeighbors);

	/// Finds all bonds between common nearest neighbors.
	static int findNeighborBonds(const NeighborBondArray& neighborArray, unsigned int commonNeighbors, int numNeighbors, CNAPairBond* neighborBonds);

	/// Find all chains of bonds between common neighbors and determine the length
	/// of the longest continuous chain.
	static int calcMaxChainLength(CNAPairBond* neighborBonds, int numBonds);

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// Holds the modifier's results.
	class CNAResults : public StructureIdentificationResults
	{
	public:

		/// Inherit constructor of base class.
		using StructureIdentificationResults::StructureIdentificationResults;

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
	};

	/// Analysis engine that performs the conventional common neighbor analysis.
	class FixedCNAEngine : public StructureIdentificationEngine
	{
	public:

		/// Constructor.
		FixedCNAEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, const SimulationCell& simCell, QVector<bool> typesToIdentify, ConstPropertyPtr selection, FloatType cutoff) :
			StructureIdentificationEngine(validityInterval, std::move(positions), simCell, std::move(typesToIdentify), std::move(selection)), _cutoff(cutoff) {}

		/// Computes the modifier's results.
		virtual void perform() override;

	private:

		/// The CNA cutoff radius.
		const FloatType _cutoff;
	};

	/// Analysis engine that performs the adaptive common neighbor analysis.
	class AdaptiveCNAEngine : public StructureIdentificationEngine
	{
	public:

		/// Constructor.
		using StructureIdentificationEngine::StructureIdentificationEngine;
		
		/// Computes the modifier's results.
		virtual void perform() override;
	};

	/// Holds the modifier's results.
	class BondCNAResults : public CNAResults
	{
	public:

		/// Constructor.
		BondCNAResults(size_t particleCount, size_t bondCount) :
			CNAResults(particleCount),
			_cnaIndices(std::make_shared<PropertyStorage>(bondCount, qMetaTypeId<int>(), 3, 0, tr("CNA Indices"), false)) {}

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

		/// Returns the output bonds property that stores the computed CNA indices.
		const PropertyPtr& cnaIndices() const { return _cnaIndices; }

	private:

		PropertyPtr _cnaIndices;
	};

	/// Analysis engine that performs the common neighbor analysis based on existing bonds.
	class BondCNAEngine : public StructureIdentificationEngine
	{
	public:

		/// Constructor.
		BondCNAEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, const SimulationCell& simCell, QVector<bool> typesToIdentify, ConstPropertyPtr selection, ConstBondsPtr bonds) :
			StructureIdentificationEngine(validityInterval, std::move(positions), simCell, std::move(typesToIdentify), std::move(selection)), 
			_bonds(std::move(bonds)) {}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Returns the input bonds between particles.
		const ConstBondsPtr& bonds() const { return _bonds; }

	private:

		const ConstBondsPtr _bonds;
	};

	/// Determines the coordination structure of a single particle using the common neighbor analysis method.
	static StructureType determineStructureAdaptive(NearestNeighborFinder& neighList, size_t particleIndex, const QVector<bool>& typesToIdentify);

	/// Determines the coordination structure of a single particle using the common neighbor analysis method.
	static StructureType determineStructureFixed(CutoffNeighborFinder& neighList, size_t particleIndex, const QVector<bool>& typesToIdentify);

	/// The cutoff radius used for the conventional CNA.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, cutoff, setCutoff, PROPERTY_FIELD_MEMORIZE);

	/// Controls how the CNA is performed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(CNAMode, mode, setMode, PROPERTY_FIELD_MEMORIZE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


