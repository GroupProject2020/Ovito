///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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
#include <core/utilities/concurrent/ParallelFor.h>
#include <plugins/particles/util/TreeNeighborListBuilder.h>

#include "IdentifyDiamondModifier.h"
#include <plugins/particles/modifier/analysis/cna/CommonNeighborAnalysisModifier.h>

namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, IdentifyDiamondModifier, StructureIdentificationModifier);
IMPLEMENT_OVITO_OBJECT(Particles, IdentifyDiamondModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(IdentifyDiamondModifier, IdentifyDiamondModifierEditor);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
IdentifyDiamondModifier::IdentifyDiamondModifier(DataSet* dataset) : StructureIdentificationModifier(dataset)
{
	// Create the structure types.
	createStructureType(OTHER, tr("Other"));
	createStructureType(CUBIC_DIAMOND, tr("Cubic diamond"), Color(0.2f, 0.95f, 0.8f));
	createStructureType(HEX_DIAMOND, tr("Hexagonal diamond"), Color(0.95f, 0.8f, 0.2f));
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::Engine> IdentifyDiamondModifier::createEngine(TimePoint time, TimeInterval& validityInterval)
{
	if(structureTypes().size() != NUM_STRUCTURE_TYPES)
		throw Exception(tr("The number of structure types has changed. Please remove this modifier from the modification pipeline and insert it again."));

	// Get modifier input.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	SimulationCell* simCell = expectSimulationCell();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<Engine>(posProperty->storage(), simCell->data());
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void IdentifyDiamondModifier::Engine::compute(FutureInterfaceBase& futureInterface)
{
	futureInterface.setProgressText(tr("Finding nearest neighbors"));

	// Prepare the neighbor list builder.
	TreeNeighborListBuilder neighborListBuilder(5);
	if(!neighborListBuilder.prepare(positions(), cell()) || futureInterface.isCanceled())
		return;

	// List of four neighbors for each atom.
	struct NeighborInfo {
		Vector3 vec;
		int index;
	};
	std::vector<std::array<NeighborInfo,5>> neighLists(positions()->size());

	// Determine four nearest neighbors of each atom and store vectors in the working array.
	parallelFor(positions()->size(), futureInterface, [&neighborListBuilder, &neighLists](size_t index) {
		TreeNeighborListBuilder::Locator<5> loc(neighborListBuilder);
		loc.findNeighbors(neighborListBuilder.particlePos(index));
		for(size_t i = 0; i < loc.results().size(); i++) {
			neighLists[index][i].vec = loc.results()[i].delta;
			neighLists[index][i].index = loc.results()[i].index;
		}
		for(size_t i = loc.results().size(); i < 5; i++) {
			neighLists[index][i].vec.setZero();
			neighLists[index][i].index = -1;
		}
	});

	// Create output storage.
	ParticleProperty* output = structures();

	// Perform structure identification.
	futureInterface.setProgressText(tr("Identifying diamond structures"));
	parallelFor(positions()->size(), futureInterface, [&neighLists, output, this](size_t index) {
		// Mark atom as 'other' by default.
		output->setInt(index, OTHER);

		const std::array<NeighborInfo,5>& nlist = neighLists[index];

		// Compute local cutoff radius.
		FloatType sum = 0;
		for(size_t i = 0; i < 4; i++) {
			sum += nlist[i].vec.squaredLength();
		}
		sum /= 4;
		constexpr FloatType factor1 = 1.3164965809;   // = (4.0/sqrt(3.0)) * ((sqrt(3.0)/4.0 + sqrt(0.5)) / 2)
		constexpr FloatType factor2 = 1.9711971193;   // = (4.0/sqrt(3.0)) * ((1.0 + sqrt(0.5)) / 2)
		FloatType localCutoffSquared = sum * (factor2 * factor2);

		// Make sure the fifth neighbor is beyond the first nearest neighbor shell.
		if(nlist[4].index != -1 && nlist[4].vec.squaredLength() < sum * (factor1 * factor1))
			return;

		// Generate list of second nearest neighbors.
		std::array<Vector3,12> secondNeighbors;
		auto vout = secondNeighbors.begin();
		for(size_t i = 0; i < 4; i++) {
			const Vector3& v0 = nlist[i].vec;
			if(nlist[i].index == -1) return;
			const std::array<NeighborInfo,5>& nlist2 = neighLists[nlist[i].index];
			for(size_t j = 0; j < 4; j++) {
				Vector3 v = v0 + nlist2[j].vec;
				if(v.isZero(1e-1f)) continue;
				if(vout == secondNeighbors.end()) return;
				if(v.squaredLength() > localCutoffSquared) return;
				*vout++ = v;
			}
			if(vout != secondNeighbors.begin() + i*3 + 3) return;
		}

		// Compute bonds between common neighbors.
		CommonNeighborAnalysisModifier::NeighborBondArray neighborArray;
		for(int ni1 = 0; ni1 < 12; ni1++) {
			neighborArray.setNeighborBond(ni1, ni1, false);
			for(int ni2 = ni1+1; ni2 < 12; ni2++)
				neighborArray.setNeighborBond(ni1, ni2, (secondNeighbors[ni1] - secondNeighbors[ni2]).squaredLength() <= localCutoffSquared);
		}

		// Determine whether second nearest neighbors form FCC or HCP using common neighbor analysis.
		int n421 = 0;
		int n422 = 0;
		for(int ni = 0; ni < 12; ni++) {

			// Determine number of neighbors the two atoms have in common.
			unsigned int commonNeighbors;
			int numCommonNeighbors = CommonNeighborAnalysisModifier::findCommonNeighbors(neighborArray, ni, commonNeighbors, 12);
			if(numCommonNeighbors != 4) return;

			// Determine the number of bonds among the common neighbors.
			CommonNeighborAnalysisModifier::CNAPairBond neighborBonds[12*12];
			int numNeighborBonds = CommonNeighborAnalysisModifier::findNeighborBonds(neighborArray, commonNeighbors, 12, neighborBonds);
			if(numNeighborBonds != 2) return;

			// Determine the number of bonds in the longest continuous chain.
			int maxChainLength = CommonNeighborAnalysisModifier::calcMaxChainLength(neighborBonds, numNeighborBonds);
			if(maxChainLength == 1) n421++;
			else if(maxChainLength == 2) n422++;
			else return;
		}
		if(n421 == 12) output->setInt(index, CUBIC_DIAMOND);
		else if(n421 == 6 && n422 == 6) output->setInt(index, HEX_DIAMOND);
	});
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void IdentifyDiamondModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Identify diamond structure"), rolloutParams, "particles.modifiers.identify_diamond_structure.html");

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(6);

	// Status label.
	layout1->addWidget(statusLabel());

	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this);
	layout1->addSpacing(10);
	layout1->addWidget(new QLabel(tr("Structure types:")));
	layout1->addWidget(structureTypesPUI->tableWidget());
	layout1->addWidget(new QLabel(tr("(Double-click to change colors)")));
}

};	// End of namespace
