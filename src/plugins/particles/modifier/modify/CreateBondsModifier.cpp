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

#include <plugins/particles/Particles.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <core/utilities/units/UnitsManager.h>
#include "CreateBondsModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(CreateBondsModifier);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, cutoffMode);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, uniformCutoff);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, minimumCutoff);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, onlyIntraMoleculeBonds);
DEFINE_REFERENCE_FIELD(CreateBondsModifier, bondsDisplay);
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, cutoffMode, "Cutoff mode");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, uniformCutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, minimumCutoff, "Lower cutoff");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, onlyIntraMoleculeBonds, "Suppress inter-molecular bonds");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, bondsDisplay, "Bonds display");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CreateBondsModifier, uniformCutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CreateBondsModifier, minimumCutoff, WorldParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CreateBondsModifier::CreateBondsModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_cutoffMode(UniformCutoff), _uniformCutoff(3.2), _onlyIntraMoleculeBonds(false), _minimumCutoff(0)
{
	// Create the display object for bonds rendering and assign it to the data object.
	setBondsDisplay(new BondsDisplay(dataset));
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool CreateBondsModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Sets the cutoff radii for pairs of particle types.
******************************************************************************/
void CreateBondsModifier::setPairCutoffs(const PairCutoffsList& pairCutoffs)
{
	// Make the property change undoable.
	dataset()->undoStack().undoablePropertyChange<PairCutoffsList>(this,
			&CreateBondsModifier::pairCutoffs, &CreateBondsModifier::setPairCutoffs);

	_pairCutoffs = pairCutoffs;

	notifyDependents(ReferenceEvent::TargetChanged);
}

/******************************************************************************
* Sets the cutoff radius for a pair of particle types.
******************************************************************************/
void CreateBondsModifier::setPairCutoff(const QString& typeA, const QString& typeB, FloatType cutoff)
{
	PairCutoffsList newList = pairCutoffs();
	if(cutoff > 0) {
		newList[qMakePair(typeA, typeB)] = cutoff;
		newList[qMakePair(typeB, typeA)] = cutoff;
	}
	else {
		newList.remove(qMakePair(typeA, typeB));
		newList.remove(qMakePair(typeB, typeA));
	}
	setPairCutoffs(newList);
}

/******************************************************************************
* Returns the pair-wise cutoff radius for a pair of particle types.
******************************************************************************/
FloatType CreateBondsModifier::getPairCutoff(const QString& typeA, const QString& typeB) const
{
	auto iter = pairCutoffs().find(qMakePair(typeA, typeB));
	if(iter != pairCutoffs().end()) return iter.value();
	iter = pairCutoffs().find(qMakePair(typeB, typeA));
	if(iter != pairCutoffs().end()) return iter.value();
	return 0;
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void CreateBondsModifier::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	AsynchronousModifier::saveToStream(stream, excludeRecomputableData);

	stream.beginChunk(0x01);
	stream << _pairCutoffs;
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void CreateBondsModifier::loadFromStream(ObjectLoadStream& stream)
{
	AsynchronousModifier::loadFromStream(stream);

	stream.expectChunk(0x01);
	stream >> _pairCutoffs;
	stream.closeChunk();
}

/******************************************************************************
* Creates a copy of this object.
******************************************************************************/
OORef<RefTarget> CreateBondsModifier::clone(bool deepCopy, CloneHelper& cloneHelper)
{
	// Let the base class create an instance of this class.
	OORef<CreateBondsModifier> clone = static_object_cast<CreateBondsModifier>(AsynchronousModifier::clone(deepCopy, cloneHelper));
	clone->_pairCutoffs = this->_pairCutoffs;
	return clone;
}

/******************************************************************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool CreateBondsModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// Do not propagate messages from the attached display object.
	if(source == bondsDisplay())
		return false;

	return AsynchronousModifier::referenceEvent(source, event);
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void CreateBondsModifier::initializeModifier(ModifierApplication* modApp)
{
	AsynchronousModifier::initializeModifier(modApp);

	// Adopt the upstream BondsDisplay object if there already is one.
	PipelineFlowState input = modApp->evaluateInputPreliminary();
	if(BondsObject* bondsObj = input.findObject<BondsObject>()) {
		for(DisplayObject* displayObj : bondsObj->displayObjects()) {
			if(BondsDisplay* bondsDisplay = dynamic_object_cast<BondsDisplay>(displayObj)) {
				setBondsDisplay(bondsDisplay);
				break;
			}
		}
	}
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the 
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> CreateBondsModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get modifier input.
	ParticleInputHelper ph(dataset(), input);
	ParticleProperty* posProperty = ph.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = ph.expectSimulationCell();

	// The neighbor list cutoff.
	FloatType maxCutoff = uniformCutoff();

	// Build table of pair-wise cutoff radii.
	ParticleProperty* typeProperty = nullptr;
	std::vector<std::vector<FloatType>> pairCutoffSquaredTable;
	if(cutoffMode() == PairCutoff) {
		typeProperty = ph.expectStandardProperty<ParticleProperty>(ParticleProperty::TypeProperty);
		if(typeProperty) {
			maxCutoff = 0;
			for(PairCutoffsList::const_iterator entry = pairCutoffs().begin(); entry != pairCutoffs().end(); ++entry) {
				FloatType cutoff = entry.value();
				if(cutoff > 0) {
					ElementType* ptype1 = typeProperty->elementType(entry.key().first);
					ElementType* ptype2 = typeProperty->elementType(entry.key().second);
					if(ptype1 && ptype2 && ptype1->id() >= 0 && ptype2->id() >= 0) {
						if((int)pairCutoffSquaredTable.size() <= std::max(ptype1->id(), ptype2->id())) pairCutoffSquaredTable.resize(std::max(ptype1->id(), ptype2->id()) + 1);
						if((int)pairCutoffSquaredTable[ptype1->id()].size() <= ptype2->id()) pairCutoffSquaredTable[ptype1->id()].resize(ptype2->id() + 1, FloatType(0));
						if((int)pairCutoffSquaredTable[ptype2->id()].size() <= ptype1->id()) pairCutoffSquaredTable[ptype2->id()].resize(ptype1->id() + 1, FloatType(0));
						pairCutoffSquaredTable[ptype1->id()][ptype2->id()] = cutoff * cutoff;
						pairCutoffSquaredTable[ptype2->id()][ptype1->id()] = cutoff * cutoff;
						if(cutoff > maxCutoff) maxCutoff = cutoff;
					}
				}
			}
			if(maxCutoff <= 0)
				throwException(tr("At least one positive bond cutoff must be set for a valid pair of particle types."));
		}
	}

	// Get molecule IDs.
	ParticleProperty* moleculeProperty = onlyIntraMoleculeBonds() ? ph.inputStandardProperty<ParticleProperty>(ParticleProperty::MoleculeProperty) : nullptr;

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<BondsEngine>(posProperty->storage(),
			typeProperty ? typeProperty->storage() : nullptr, simCell->data(), cutoffMode(),
			maxCutoff, minimumCutoff(), std::move(pairCutoffSquaredTable), moleculeProperty ? moleculeProperty->storage() : nullptr);
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CreateBondsModifier::BondsEngine::perform()
{
	setProgressText(tr("Generating bonds"));

	// Prepare the neighbor list.
	CutoffNeighborFinder neighborFinder;
	if(!neighborFinder.prepare(_maxCutoff, *_positions, _simCell, nullptr, this))
		return;

	FloatType minCutoffSquared = _minCutoff * _minCutoff;

	auto results = std::make_shared<BondsEngineResults>();

	// Generate bonds.
	size_t particleCount = _positions->size();
	setProgressMaximum(particleCount);
	if(!_particleTypes) {
		for(size_t particleIndex = 0; particleIndex < particleCount; particleIndex++) {
			for(CutoffNeighborFinder::Query neighborQuery(neighborFinder, particleIndex); !neighborQuery.atEnd(); neighborQuery.next()) {
				if(neighborQuery.distanceSquared() < minCutoffSquared)
					continue;
				if(_moleculeIDs && _moleculeIDs->getInt64(particleIndex) != _moleculeIDs->getInt64(neighborQuery.current()))
					continue;

				Bond bond = { particleIndex, neighborQuery.current(), neighborQuery.unwrappedPbcShift() };

				// Skip every other bond to create only one bond per particle pair.
				if(!bond.isOdd())
					results->bonds()->push_back(bond);
			}
			// Update progress indicator.
			if(!setProgressValueIntermittent(particleIndex))
				return;
		}
	}
	else {
		for(size_t particleIndex = 0; particleIndex < particleCount; particleIndex++) {
			for(CutoffNeighborFinder::Query neighborQuery(neighborFinder, particleIndex); !neighborQuery.atEnd(); neighborQuery.next()) {
				if(neighborQuery.distanceSquared() < minCutoffSquared)
					continue;
				if(_moleculeIDs && _moleculeIDs->getInt64(particleIndex) != _moleculeIDs->getInt64(neighborQuery.current()))
					continue;
				int type1 = _particleTypes->getInt(particleIndex);
				int type2 = _particleTypes->getInt(neighborQuery.current());
				if(type1 >= 0 && type1 < (int)_pairCutoffsSquared.size() && type2 >= 0 && type2 < (int)_pairCutoffsSquared[type1].size()) {
					if(neighborQuery.distanceSquared() <= _pairCutoffsSquared[type1][type2]) {
						Bond bond = { particleIndex, neighborQuery.current(), neighborQuery.unwrappedPbcShift() };
						// Skip every other bond to create only one bond per particle pair.
						if(!bond.isOdd())				
							results->bonds()->push_back(bond);
					}
				}
			}
			// Update progress indicator.
			if(!setProgressValueIntermittent(particleIndex))
				return;
		}
	}
	setProgressValue(particleCount);

	// Return the results of the compute engine.
	setResult(std::move(results));
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState CreateBondsModifier::BondsEngineResults::apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	CreateBondsModifier* modifier = static_object_cast<CreateBondsModifier>(modApp->modifier());
	OVITO_ASSERT(modifier);

	// Add our bonds to the system.
	PipelineFlowState output = input;
	ParticleOutputHelper poh(modApp->dataset(), output);
	poh.addBonds(bonds(), modifier->bondsDisplay());

	size_t bondsCount = bonds()->size();
	output.attributes().insert(QStringLiteral("CreateBonds.num_bonds"), QVariant::fromValue(bondsCount));

	// If the number of bonds is unusually high, we better turn off bonds display to prevent the program from freezing.
	if(bondsCount > 1000000) {
		modifier->bondsDisplay()->setEnabled(false);		
		output.setStatus(PipelineStatus(PipelineStatus::Warning, tr("Created %1 bonds, which is a lot. As a precaution, the display of bonds has been disabled. You can manually enable it again if needed.").arg(bondsCount)));
	}
	else {
		output.setStatus(PipelineStatus(PipelineStatus::Success, tr("Created %1 bonds.").arg(bondsCount)));
	}

	return output;	
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
