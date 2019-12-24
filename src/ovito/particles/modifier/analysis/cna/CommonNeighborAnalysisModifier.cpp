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

#include <ovito/particles/Particles.h>
#include <ovito/particles/util/NearestNeighborFinder.h>
#include <ovito/particles/util/CutoffNeighborFinder.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/ParticleBondMap.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "CommonNeighborAnalysisModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(CommonNeighborAnalysisModifier);
DEFINE_PROPERTY_FIELD(CommonNeighborAnalysisModifier, cutoff);
DEFINE_PROPERTY_FIELD(CommonNeighborAnalysisModifier, mode);
SET_PROPERTY_FIELD_LABEL(CommonNeighborAnalysisModifier, cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(CommonNeighborAnalysisModifier, mode, "Mode");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CommonNeighborAnalysisModifier, cutoff, WorldParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CommonNeighborAnalysisModifier::CommonNeighborAnalysisModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
	_cutoff(3.2), _mode(AdaptiveCutoffMode)
{
	// Create the structure types.
	createStructureType(OTHER, ParticleType::PredefinedStructureType::OTHER);
	createStructureType(FCC, ParticleType::PredefinedStructureType::FCC);
	createStructureType(HCP, ParticleType::PredefinedStructureType::HCP);
	createStructureType(BCC, ParticleType::PredefinedStructureType::BCC);
	createStructureType(ICO, ParticleType::PredefinedStructureType::ICO);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> CommonNeighborAnalysisModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	if(structureTypes().size() != NUM_STRUCTURE_TYPES)
		throwException(tr("The number of structure types has changed. Please remove this modifier from the pipeline and insert it again."));

	// Get modifier input.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();
	if(simCell->is2D())
		throwException(tr("The CNA modifier does not support 2d simulation cells."));

	// Get particle selection.
	ConstPropertyPtr selectionProperty;
	if(onlySelectedParticles())
		selectionProperty = particles->expectProperty(ParticlesObject::SelectionProperty)->storage();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	if(mode() == AdaptiveCutoffMode) {
		return std::make_shared<AdaptiveCNAEngine>(particles, posProperty->storage(), simCell->data(), getTypesToIdentify(NUM_STRUCTURE_TYPES), std::move(selectionProperty));
	}
	else if(mode() == BondMode) {
		const PropertyObject* topologyProperty = particles->expectBonds()->expectProperty(BondsObject::TopologyProperty);
		const PropertyObject* periodicImagesProperty = particles->expectBonds()->getProperty(BondsObject::PeriodicImageProperty);
		return std::make_shared<BondCNAEngine>(particles, posProperty->storage(), simCell->data(), getTypesToIdentify(NUM_STRUCTURE_TYPES), std::move(selectionProperty),
			topologyProperty->storage(), periodicImagesProperty ? periodicImagesProperty->storage() : nullptr);
	}
	else {
		return std::make_shared<FixedCNAEngine>(particles, posProperty->storage(), simCell->data(), getTypesToIdentify(NUM_STRUCTURE_TYPES), std::move(selectionProperty), cutoff());
	}
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CommonNeighborAnalysisModifier::AdaptiveCNAEngine::perform()
{
	task()->setProgressText(tr("Performing adaptive common neighbor analysis"));

	// Prepare the neighbor list.
	NearestNeighborFinder neighFinder(MAX_NEIGHBORS);
	if(!neighFinder.prepare(*positions(), cell(), selection().get(), task().get()))
		return;

	// Create output storage.
	PropertyStorage& output = *structures();

	// Perform analysis on each particle.
	parallelFor(positions()->size(), *task(), [this, &neighFinder, &output](size_t index) {
		// Skip particles that are not included in the analysis.
		if(!selection() || selection()->get<int>(index))
			output.set<int>(index, determineStructureAdaptive(neighFinder, index, typesToIdentify()));
		else
			output.set<int>(index, OTHER);
	});
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CommonNeighborAnalysisModifier::FixedCNAEngine::perform()
{
	task()->setProgressText(tr("Performing common neighbor analysis"));

	// Prepare the neighbor list.
	CutoffNeighborFinder neighborListBuilder;
	if(!neighborListBuilder.prepare(_cutoff, *positions(), cell(), selection().get(), task().get()))
		return;

	// Create output storage.
	PropertyStorage& output = *structures();

	// Perform analysis on each particle.
	parallelFor(positions()->size(), *task(), [this, &neighborListBuilder, &output](size_t index) {
		// Skip particles that are not included in the analysis.
		if(!selection() || selection()->get<int>(index))
			output.set<int>(index, determineStructureFixed(neighborListBuilder, index, typesToIdentify()));
		else
			output.set<int>(index, OTHER);
	});
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CommonNeighborAnalysisModifier::BondCNAEngine::perform()
{
	task()->setProgressText(tr("Performing common neighbor analysis"));

	// Prepare particle bond map.
	ParticleBondMap bondMap(bondTopology(), bondPeriodicImages());

	// Compute per-bond CNA indices.
	bool maxNeighborLimitExceeded = false;
	bool maxCommonNeighborBondLimitExceeded = false;
	parallelFor(bondTopology()->size(), *task(), [this, &bondMap, &maxNeighborLimitExceeded, &maxCommonNeighborBondLimitExceeded](size_t bondIndex) {
		size_t currentBondParticle1 = bondTopology()->get<qlonglong>(bondIndex, 0);
		size_t currentBondParticle2 = bondTopology()->get<qlonglong>(bondIndex, 1);
		if(currentBondParticle1 >= positions()->size()) return;
		if(currentBondParticle2 >= positions()->size()) return;
		Vector3I currentBondPbcShift = bondPeriodicImages() ? bondPeriodicImages()->get<Vector3I>(bondIndex) : Vector3I::Zero();

		// Determine common neighbors shared by both particles.
		int numCommonNeighbors = 0;
		std::array<std::pair<size_t, Vector3I>, 32> commonNeighbors;
		for(Bond neighborBond1 : bondMap.bondsOfParticle(currentBondParticle1)) {
			OVITO_ASSERT(neighborBond1.index1 == currentBondParticle1);
			for(Bond neighborBond2 : bondMap.bondsOfParticle(currentBondParticle2)) {
				OVITO_ASSERT(neighborBond2.index1 == currentBondParticle2);
				if(neighborBond2.index2 == neighborBond1.index2 && neighborBond1.pbcShift == currentBondPbcShift + neighborBond2.pbcShift) {
					if(numCommonNeighbors == commonNeighbors.size()) {
						maxNeighborLimitExceeded = true;
						return;
					}
					commonNeighbors[numCommonNeighbors].first = neighborBond1.index2;
					commonNeighbors[numCommonNeighbors].second = neighborBond1.pbcShift;
					numCommonNeighbors++;
					break;
				}
			}
		}

		// Determine which of the common neighbors are inter-connected by bonds.
		std::array<CNAPairBond, 64> commonNeighborBonds;
		int numCommonNeighborBonds = 0;
		for(int ni1 = 0; ni1 < numCommonNeighbors; ni1++) {
			for(Bond neighborBond : bondMap.bondsOfParticle(commonNeighbors[ni1].first)) {
				for(int ni2 = 0; ni2 < ni1; ni2++) {
					if(commonNeighbors[ni2].first == neighborBond.index2 && commonNeighbors[ni1].second + neighborBond.pbcShift == commonNeighbors[ni2].second) {
						if(numCommonNeighborBonds == commonNeighborBonds.size()) {
							maxCommonNeighborBondLimitExceeded = true;
							return;
						}
						commonNeighborBonds[numCommonNeighborBonds++] = (1<<ni1) | (1<<ni2);
						break;
					}
				}
			}
		}

		// Determine the number of bonds in the longest continuous chain.
		int maxChainLength = calcMaxChainLength(commonNeighborBonds.data(), numCommonNeighborBonds);

		// Store results in bond property.
		cnaIndices()->set<int>(bondIndex, 0, numCommonNeighbors);
		cnaIndices()->set<int>(bondIndex, 1, numCommonNeighborBonds);
		cnaIndices()->set<int>(bondIndex, 2, maxChainLength);
	});
	if(task()->isCanceled())
		return;
	if(maxNeighborLimitExceeded)
		throw Exception(tr("Two of the particles have more than 32 common neighbors, which is the built-in limit. Cannot perform CNA in this case."));
	if(maxCommonNeighborBondLimitExceeded)
		throw Exception(tr("There are more than 64 bonds between common neighbors, which is the built-in limit. Cannot perform CNA in this case."));

	// Create output storage.
	PropertyStorage& output = *structures();

	// Classify particles.
	parallelFor(positions()->size(), *task(), [this, &output, &bondMap](size_t particleIndex) {

		int n421 = 0;
		int n422 = 0;
		int n444 = 0;
		int n555 = 0;
		int n666 = 0;
		int ntotal = 0;
		for(size_t neighborBondIndex : bondMap.bondIndicesOfParticle(particleIndex)) {
			const Point3I& indices = cnaIndices()->get<Point3I>(neighborBondIndex);
			if(indices[0] == 4) {
				if(indices[1] == 2) {
					if(indices[2] == 1) n421++;
					else if(indices[2] == 2) n422++;
				}
				else if(indices[1] == 4 && indices[2] == 4) n444++;
			}
			else if(indices[0] == 5 && indices[1] == 5 && indices[2] == 5) n555++;
			else if(indices[0] == 6 && indices[1] == 6 && indices[2] == 6) n666++;
			else {
				output.set<int>(particleIndex, OTHER);
				return;
			}
			ntotal++;
		}

		if(n421 == 12 && ntotal == 12 && typesToIdentify()[FCC])
			output.set<int>(particleIndex, FCC);
		else if(n421 == 6 && n422 == 6 && ntotal == 12 && typesToIdentify()[HCP])
			output.set<int>(particleIndex, HCP);
		else if(n444 == 6 && n666 == 8 && ntotal == 14 && typesToIdentify()[BCC])
			output.set<int>(particleIndex, BCC);
		else if(n555 == 12 && ntotal == 12 && typesToIdentify()[ICO])
			output.set<int>(particleIndex, ICO);
		else
			output.set<int>(particleIndex, OTHER);
	});
}

/******************************************************************************
* Find all atoms that are nearest neighbors of the given pair of atoms.
******************************************************************************/
int CommonNeighborAnalysisModifier::findCommonNeighbors(const NeighborBondArray& neighborArray, int neighborIndex, unsigned int& commonNeighbors, int numNeighbors)
{
	commonNeighbors = neighborArray.neighborArray[neighborIndex];
#ifndef Q_CC_MSVC
	// Count the number of bits set in neighbor bit-field.
	return __builtin_popcount(commonNeighbors);
#else
	// Count the number of bits set in neighbor bit-field.
	unsigned int v = commonNeighbors - ((commonNeighbors >> 1) & 0x55555555);
	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
	return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
#endif
}

/******************************************************************************
* Finds all bonds between common nearest neighbors.
******************************************************************************/
int CommonNeighborAnalysisModifier::findNeighborBonds(const NeighborBondArray& neighborArray, unsigned int commonNeighbors, int numNeighbors, CNAPairBond* neighborBonds)
{
	int numBonds = 0;

	unsigned int nib[32];
	int nibn = 0;
	unsigned int ni1b = 1;
	for(int ni1 = 0; ni1 < numNeighbors; ni1++, ni1b <<= 1) {
		if(commonNeighbors & ni1b) {
			unsigned int b = commonNeighbors & neighborArray.neighborArray[ni1];
			for(int n = 0; n < nibn; n++) {
				if(b & nib[n]) {
					neighborBonds[numBonds++] = ni1b | nib[n];
				}
			}
			nib[nibn++] = ni1b;
		}
	}
	return numBonds;
}

/******************************************************************************
* Find all chains of bonds.
******************************************************************************/
static int getAdjacentBonds(unsigned int atom, CommonNeighborAnalysisModifier::CNAPairBond* bondsToProcess, int& numBonds, unsigned int& atomsToProcess, unsigned int& atomsProcessed)
{
    int adjacentBonds = 0;
	for(int b = numBonds - 1; b >= 0; b--) {
		if(atom & *bondsToProcess) {
            ++adjacentBonds;
   			atomsToProcess |= *bondsToProcess & (~atomsProcessed);
   			memmove(bondsToProcess, bondsToProcess + 1, sizeof(CommonNeighborAnalysisModifier::CNAPairBond) * b);
   			numBonds--;
		}
		else ++bondsToProcess;
	}
	return adjacentBonds;
}

/******************************************************************************
* Find all chains of bonds between common neighbors and determine the length
* of the longest continuous chain.
******************************************************************************/
int CommonNeighborAnalysisModifier::calcMaxChainLength(CNAPairBond* neighborBonds, int numBonds)
{
    // Group the common bonds into clusters.
	int maxChainLength = 0;
	while(numBonds) {
        // Make a new cluster starting with the first remaining bond to be processed.
		numBonds--;
        unsigned int atomsToProcess = neighborBonds[numBonds];
        unsigned int atomsProcessed = 0;
		int clusterSize = 1;
        do {
#ifndef Q_CC_MSVC
        	// Determine the number of trailing 0-bits in atomsToProcess, starting at the least significant bit position.
			int nextAtomIndex = __builtin_ctz(atomsToProcess);
#else
			unsigned long nextAtomIndex;
			_BitScanForward(&nextAtomIndex, atomsToProcess);
			OVITO_ASSERT(nextAtomIndex >= 0 && nextAtomIndex < 32);
#endif
			unsigned int nextAtom = 1 << nextAtomIndex;
        	atomsProcessed |= nextAtom;
			atomsToProcess &= ~nextAtom;
			clusterSize += getAdjacentBonds(nextAtom, neighborBonds, numBonds, atomsToProcess, atomsProcessed);
		}
        while(atomsToProcess);
        if(clusterSize > maxChainLength)
        	maxChainLength = clusterSize;
	}
	return maxChainLength;
}

/******************************************************************************
* Determines the coordination structure of a single particle using the
* adaptive common neighbor analysis method.
******************************************************************************/
CommonNeighborAnalysisModifier::StructureType CommonNeighborAnalysisModifier::determineStructureAdaptive(NearestNeighborFinder& neighFinder, size_t particleIndex, const QVector<bool>& typesToIdentify)
{
	// Construct local neighbor list builder.
	NearestNeighborFinder::Query<MAX_NEIGHBORS> neighQuery(neighFinder);

	// Find N nearest neighbors of current atom.
	neighQuery.findNeighbors(particleIndex);
	int numNeighbors = neighQuery.results().size();

	/////////// 12 neighbors ///////////
	if(typesToIdentify[FCC] || typesToIdentify[HCP] || typesToIdentify[ICO]) {

		// Number of neighbors to analyze.
		int nn = 12; // For FCC, HCP and Icosahedral atoms

		// Early rejection of under-coordinated atoms:
		if(numNeighbors < nn)
			return OTHER;

		// Compute scaling factor.
		FloatType localScaling = 0;
		for(int n = 0; n < nn; n++)
			localScaling += sqrt(neighQuery.results()[n].distanceSq);
		FloatType localCutoff = localScaling / nn * (1.0f + sqrt(2.0f)) * 0.5f;
		FloatType localCutoffSquared =  localCutoff * localCutoff;

		// Compute common neighbor bit-flag array.
		NeighborBondArray neighborArray;
		for(int ni1 = 0; ni1 < nn; ni1++) {
			neighborArray.setNeighborBond(ni1, ni1, false);
			for(int ni2 = ni1+1; ni2 < nn; ni2++)
				neighborArray.setNeighborBond(ni1, ni2, (neighQuery.results()[ni1].delta - neighQuery.results()[ni2].delta).squaredLength() <= localCutoffSquared);
		}

		int n421 = 0;
		int n422 = 0;
		int n555 = 0;
		for(int ni = 0; ni < nn; ni++) {

			// Determine number of neighbors the two atoms have in common.
			unsigned int commonNeighbors;
			int numCommonNeighbors = findCommonNeighbors(neighborArray, ni, commonNeighbors, nn);
			if(numCommonNeighbors != 4 && numCommonNeighbors != 5)
				break;

			// Determine the number of bonds among the common neighbors.
			CNAPairBond neighborBonds[MAX_NEIGHBORS*MAX_NEIGHBORS];
			int numNeighborBonds = findNeighborBonds(neighborArray, commonNeighbors, nn, neighborBonds);
			if(numNeighborBonds != 2 && numNeighborBonds != 5)
				break;

			// Determine the number of bonds in the longest continuous chain.
			int maxChainLength = calcMaxChainLength(neighborBonds, numNeighborBonds);
			if(numCommonNeighbors == 4 && numNeighborBonds == 2) {
				if(maxChainLength == 1) n421++;
				else if(maxChainLength == 2) n422++;
				else break;
			}
			else if(numCommonNeighbors == 5 && numNeighborBonds == 5 && maxChainLength == 5) n555++;
			else break;
		}
		if(n421 == 12 && typesToIdentify[FCC]) return FCC;
		else if(n421 == 6 && n422 == 6 && typesToIdentify[HCP]) return HCP;
		else if(n555 == 12 && typesToIdentify[ICO]) return ICO;
	}

	/////////// 14 neighbors ///////////
	if(typesToIdentify[BCC]) {

		// Number of neighbors to analyze.
		int nn = 14; // For BCC atoms

		// Early rejection of under-coordinated atoms:
		if(numNeighbors < nn)
			return OTHER;

		// Compute scaling factor.
		FloatType localScaling = 0;
		for(int n = 0; n < 8; n++)
			localScaling += sqrt(neighQuery.results()[n].distanceSq / (3.0f/4.0f));
		for(int n = 8; n < 14; n++)
			localScaling += sqrt(neighQuery.results()[n].distanceSq);
		FloatType localCutoff = localScaling / nn * 1.207f;
		FloatType localCutoffSquared =  localCutoff * localCutoff;

		// Compute common neighbor bit-flag array.
		NeighborBondArray neighborArray;
		for(int ni1 = 0; ni1 < nn; ni1++) {
			neighborArray.setNeighborBond(ni1, ni1, false);
			for(int ni2 = ni1+1; ni2 < nn; ni2++)
				neighborArray.setNeighborBond(ni1, ni2, (neighQuery.results()[ni1].delta - neighQuery.results()[ni2].delta).squaredLength() <= localCutoffSquared);
		}

		int n444 = 0;
		int n666 = 0;
		for(int ni = 0; ni < nn; ni++) {

			// Determine number of neighbors the two atoms have in common.
			unsigned int commonNeighbors;
			int numCommonNeighbors = findCommonNeighbors(neighborArray, ni, commonNeighbors, nn);
			if(numCommonNeighbors != 4 && numCommonNeighbors != 6)
				break;

			// Determine the number of bonds among the common neighbors.
			CNAPairBond neighborBonds[MAX_NEIGHBORS*MAX_NEIGHBORS];
			int numNeighborBonds = findNeighborBonds(neighborArray, commonNeighbors, nn, neighborBonds);
			if(numNeighborBonds != 4 && numNeighborBonds != 6)
				break;

			// Determine the number of bonds in the longest continuous chain.
			int maxChainLength = calcMaxChainLength(neighborBonds, numNeighborBonds);
			if(numCommonNeighbors == 4 && numNeighborBonds == 4 && maxChainLength == 4) n444++;
			else if(numCommonNeighbors == 6 && numNeighborBonds == 6 && maxChainLength == 6) n666++;
			else break;
		}
		if(n666 == 8 && n444 == 6) return BCC;
	}

	return OTHER;
}

/******************************************************************************
* Determines the coordination structure of a single particle using the
* conventional common neighbor analysis method.
******************************************************************************/
CommonNeighborAnalysisModifier::StructureType CommonNeighborAnalysisModifier::determineStructureFixed(CutoffNeighborFinder& neighList, size_t particleIndex, const QVector<bool>& typesToIdentify)
{
	// Store neighbor vectors in a local array.
	int numNeighbors = 0;
	Vector3 neighborVectors[MAX_NEIGHBORS];
	for(CutoffNeighborFinder::Query neighborQuery(neighList, particleIndex); !neighborQuery.atEnd(); neighborQuery.next()) {
		if(numNeighbors == MAX_NEIGHBORS) return OTHER;
		neighborVectors[numNeighbors] = neighborQuery.delta();
		numNeighbors++;
	}

	if(numNeighbors != 12 && numNeighbors != 14)
		return OTHER;

	// Compute bond bit-flag array.
	NeighborBondArray neighborArray;
	for(int ni1 = 0; ni1 < numNeighbors; ni1++) {
		neighborArray.setNeighborBond(ni1, ni1, false);
		for(int ni2 = ni1+1; ni2 < numNeighbors; ni2++)
			neighborArray.setNeighborBond(ni1, ni2, (neighborVectors[ni1] - neighborVectors[ni2]).squaredLength() <= neighList.cutoffRadiusSquared());
	}

	if(numNeighbors == 12) { // Detect FCC and HCP atoms each having 12 NN.
		int n421 = 0;
		int n422 = 0;
		int n555 = 0;
		for(int ni = 0; ni < 12; ni++) {

			// Determine number of neighbors the two atoms have in common.
			unsigned int commonNeighbors;
			int numCommonNeighbors = findCommonNeighbors(neighborArray, ni, commonNeighbors, 12);
			if(numCommonNeighbors != 4 && numCommonNeighbors != 5)
				return OTHER;

			// Determine the number of bonds among the common neighbors.
			CNAPairBond neighborBonds[MAX_NEIGHBORS*MAX_NEIGHBORS];
			int numNeighborBonds = findNeighborBonds(neighborArray, commonNeighbors, 12, neighborBonds);
			if(numNeighborBonds != 2 && numNeighborBonds != 5)
				break;

			// Determine the number of bonds in the longest continuous chain.
			int maxChainLength = calcMaxChainLength(neighborBonds, numNeighborBonds);
			if(numCommonNeighbors == 4 && numNeighborBonds == 2) {
				if(maxChainLength == 1) n421++;
				else if(maxChainLength == 2) n422++;
				else return OTHER;
			}
			else if(numCommonNeighbors == 5 && numNeighborBonds == 5 && maxChainLength == 5) n555++;
			else return OTHER;
		}
		if(n421 == 12 && typesToIdentify[FCC]) return FCC;
		else if(n421 == 6 && n422 == 6 && typesToIdentify[HCP]) return HCP;
		else if(n555 == 12 && typesToIdentify[ICO]) return ICO;
	}
	else if(numNeighbors == 14 && typesToIdentify[BCC]) { // Detect BCC atoms having 14 NN (in 1st and 2nd shell).
		int n444 = 0;
		int n666 = 0;
		for(int ni = 0; ni < 14; ni++) {

			// Determine number of neighbors the two atoms have in common.
			unsigned int commonNeighbors;
			int numCommonNeighbors = findCommonNeighbors(neighborArray, ni, commonNeighbors, 14);
			if(numCommonNeighbors != 4 && numCommonNeighbors != 6)
				return OTHER;

			// Determine the number of bonds among the common neighbors.
			CNAPairBond neighborBonds[MAX_NEIGHBORS*MAX_NEIGHBORS];
			int numNeighborBonds = findNeighborBonds(neighborArray, commonNeighbors, 14, neighborBonds);
			if(numNeighborBonds != 4 && numNeighborBonds != 6)
				break;

			// Determine the number of bonds in the longest continuous chain.
			int maxChainLength = calcMaxChainLength(neighborBonds, numNeighborBonds);
			if(numCommonNeighbors == 4 && numNeighborBonds == 4 && maxChainLength == 4) n444++;
			else if(numCommonNeighbors == 6 && numNeighborBonds == 6 && maxChainLength == 6) n666++;
			else return OTHER;
		}
		if(n666 == 8 && n444 == 6) return BCC;
	}

	return OTHER;
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void CommonNeighborAnalysisModifier::CNAEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	StructureIdentificationEngine::emitResults(time, modApp, state);

	// Also output structure type counts, which have been computed by the base class.
	state.addAttribute(QStringLiteral("CommonNeighborAnalysis.counts.OTHER"), QVariant::fromValue(getTypeCount(OTHER)), modApp);
	state.addAttribute(QStringLiteral("CommonNeighborAnalysis.counts.FCC"), QVariant::fromValue(getTypeCount(FCC)), modApp);
	state.addAttribute(QStringLiteral("CommonNeighborAnalysis.counts.HCP"), QVariant::fromValue(getTypeCount(HCP)), modApp);
	state.addAttribute(QStringLiteral("CommonNeighborAnalysis.counts.BCC"), QVariant::fromValue(getTypeCount(BCC)), modApp);
	state.addAttribute(QStringLiteral("CommonNeighborAnalysis.counts.ICO"), QVariant::fromValue(getTypeCount(ICO)), modApp);
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the modification pipeline.
******************************************************************************/
void CommonNeighborAnalysisModifier::BondCNAEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	CNAEngine::emitResults(time, modApp, state);
	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();
	particles->makeMutable(particles->expectBonds())->createProperty(cnaIndices());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
