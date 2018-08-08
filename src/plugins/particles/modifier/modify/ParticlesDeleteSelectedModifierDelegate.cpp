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
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <core/dataset/DataSet.h>
#include "ParticlesDeleteSelectedModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(ParticlesDeleteSelectedModifierDelegate);
IMPLEMENT_OVITO_CLASS(BondsDeleteSelectedModifierDelegate);

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool ParticlesDeleteSelectedModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus ParticlesDeleteSelectedModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	ParticleInputHelper pih(dataset(), output); // Note: We treat the current output as input here.
	ParticleOutputHelper poh(dataset(), output);

	// Get the selection.
	if(ParticleProperty* selProperty = pih.inputStandardProperty<ParticleProperty>(ParticleProperty::SelectionProperty)) {

		// Generate filter mask.
		boost::dynamic_bitset<> mask(pih.inputParticleCount());
		boost::dynamic_bitset<>::size_type i = 0;
		for(int s : selProperty->constIntRange()) {
			mask.set(i++, s != 0);
		}

		// Remove selection property.
		output.removeObject(selProperty);

		// Delete the particles.
		poh.deleteParticles(mask);
	}

	// Do some statistics:
	size_t inputCount = ParticleInputHelper(dataset(), input).inputParticleCount();
	size_t numDeleted = inputCount - poh.outputParticleCount();
	QString statusMessage = tr("%n input particles", 0, inputCount);
	statusMessage += tr("\n%n particles deleted (%1%)", 0, numDeleted).arg(numDeleted * 100 / std::max((int)inputCount, 1));
	
	return PipelineStatus(PipelineStatus::Success, std::move(statusMessage));
}

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool BondsDeleteSelectedModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<BondProperty>() != nullptr;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus BondsDeleteSelectedModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	ParticleInputHelper pih(dataset(), output); // Note: We treat the current output as input here.
	ParticleOutputHelper poh(dataset(), output);

	// Get the selection.
	if(BondProperty* selProperty = pih.inputStandardProperty<BondProperty>(BondProperty::SelectionProperty)) {

		// Generate filter mask.
		boost::dynamic_bitset<> mask(pih.inputBondCount());
		boost::dynamic_bitset<>::size_type i = 0;
		for(int s : selProperty->constIntRange()) {
			mask.set(i++, s != 0);
		}

		// Remove selection property.
		output.removeObject(selProperty);

		// Delete the bonds.
		poh.deleteBonds(mask);
	}

	// Do some statistics:
	size_t inputCount = ParticleInputHelper(dataset(), input).inputBondCount();
	size_t numDeleted = inputCount - poh.outputBondCount();
	QString statusMessage = tr("%n input bonds", 0, inputCount);
	statusMessage += tr("\n%n bonds deleted (%1%)", 0, numDeleted).arg(numDeleted * 100 / std::max((int)inputCount, 1));
	
	return PipelineStatus(PipelineStatus::Success, std::move(statusMessage));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
