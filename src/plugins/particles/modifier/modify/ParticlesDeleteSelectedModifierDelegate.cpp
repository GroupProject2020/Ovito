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
#include <plugins/particles/objects/ParticlesObject.h>
#include <plugins/particles/objects/BondsObject.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "ParticlesDeleteSelectedModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(ParticlesDeleteSelectedModifierDelegate);
IMPLEMENT_OVITO_CLASS(BondsDeleteSelectedModifierDelegate);

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool ParticlesDeleteSelectedModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObjectOfType<ParticlesObject>() != nullptr;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus ParticlesDeleteSelectedModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	size_t numParticles = 0;
	size_t numSelected = 0;

	// Get the particle selection.
	if(ParticlesObject* inputParticles = output.findObjectOfType<ParticlesObject>()) {
		if(PropertyObject* selProperty = inputParticles->getProperty(ParticleProperty::SelectionProperty)) {

			// Generate filter mask.
			boost::dynamic_bitset<> mask(selProperty->size());
			boost::dynamic_bitset<>::size_type i = 0;
			for(int s : selProperty->constIntRange()) {
				if(s != 0) {
					mask.set(i++);
					numSelected++;
				}
				else {
					mask.reset(i++);
				}
			}

			if(numSelected) {
				// Make sure we can safely modify the particles object.
				ParticlesObject* outputParticles = output.cloneIfNeeded(inputParticles);

				// Remove selection property.
				outputParticles->removeProperty(selProperty);

				// Delete the particles.
				outputParticles->deleteParticles(mask);
			}
		}
	}

	// Do some statistics:
	QString statusMessage = tr("%n input particles", 0, numParticles);
	statusMessage += tr("\n%n particles deleted (%1%)", 0, numSelected).arg(numSelected * 100 / std::max(numParticles, (size_t)1));
	
	return PipelineStatus(PipelineStatus::Success, std::move(statusMessage));
}

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool BondsDeleteSelectedModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	if(ParticlesObject* particles = input.findObjectOfType<ParticlesObject>())
		return particles->bonds() != nullptr;
	return false;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus BondsDeleteSelectedModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	size_t numBonds = 0;
	size_t numSelected = 0;

	// Get the bond selection.
	if(ParticlesObject* inputParticles = output.findObjectOfType<ParticlesObject>()) {
		if(BondsObject* inputBonds = inputParticles->bonds()) {
			if(PropertyObject* selProperty = inputBonds->getProperty(BondProperty::SelectionProperty)) {
				// Generate filter mask.
				boost::dynamic_bitset<> mask(selProperty->size());
				boost::dynamic_bitset<>::size_type i = 0;
				for(int s : selProperty->constIntRange()) {
					if(s != 0) {
						mask.set(i++);
						numSelected++;
					}
					else {
						mask.reset(i++);
					}
				}

				if(numSelected) {
					// Make sure we can safely modify the particles and the bonds object.
					ParticlesObject* outputParticles = output.cloneIfNeeded(outputParticles);
					BondsObject* outputBonds = outputParticles->makeBondsUnique();

					// Remove selection property.
					outputBonds->removeProperty(selProperty);

					// Delete the bonds.
					outputBonds->deleteBonds(mask);
				}
			}
		}
	}

	// Do some statistics:
	QString statusMessage = tr("%n input bonds", 0, numBonds);
	statusMessage += tr("\n%n bonds deleted (%1%)", 0, numSelected).arg(numSelected * 100 / std::max(numBonds, (size_t)1));
	
	return PipelineStatus(PipelineStatus::Success, std::move(statusMessage));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
