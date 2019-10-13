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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "ParticlesDeleteSelectedModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(ParticlesDeleteSelectedModifierDelegate);
IMPLEMENT_OVITO_CLASS(BondsDeleteSelectedModifierDelegate);

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> ParticlesDeleteSelectedModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
	if(input.containsObject<ParticlesObject>())
		return { DataObjectReference(&ParticlesObject::OOClass()) };
	return {};
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus ParticlesDeleteSelectedModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	size_t numParticles = 0;
	size_t numSelected = 0;

	// Get the particle selection.
	if(const ParticlesObject* inputParticles = state.getObject<ParticlesObject>()) {
		numParticles = inputParticles->elementCount();
		if(const PropertyObject* selProperty = inputParticles->getProperty(ParticlesObject::SelectionProperty)) {

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
				ParticlesObject* outputParticles = state.makeMutable(inputParticles);

				// Remove selection property.
				outputParticles->removeProperty(selProperty);

				// Delete the particles.
				outputParticles->deleteElements(mask);
			}
		}
	}

	// Report some statistics:
	QString statusMessage = tr("%n input particles", 0, numParticles);
	statusMessage += tr("\n%n particles deleted (%1%)", 0, numSelected).arg(numSelected * 100 / std::max(numParticles, (size_t)1));

	return PipelineStatus(PipelineStatus::Success, std::move(statusMessage));
}

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> BondsDeleteSelectedModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
    if(const ParticlesObject* particles = input.getObject<ParticlesObject>()) {
        if(particles->bonds())
       		return { DataObjectReference(&ParticlesObject::OOClass()) };
    }
    return {};
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus BondsDeleteSelectedModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	size_t numBonds = 0;
	size_t numSelected = 0;

	// Get the bond selection.
	if(const ParticlesObject* inputParticles = state.getObject<ParticlesObject>()) {
		if(const BondsObject* inputBonds = inputParticles->bonds()) {
			numBonds = inputBonds->elementCount();
			if(const PropertyObject* selProperty = inputBonds->getProperty(BondsObject::SelectionProperty)) {
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
					// Make sure we can safely modify the particles and the bonds object it contains.
					ParticlesObject* outputParticles = state.makeMutable(inputParticles);
					BondsObject* outputBonds = outputParticles->makeBondsMutable();

					// Remove selection property.
					outputBonds->removeProperty(selProperty);

					// Delete the bonds.
					outputBonds->deleteElements(mask);
				}
			}
		}
	}

	// Report some statistics:
	QString statusMessage = tr("%n input bonds", 0, numBonds);
	statusMessage += tr("\n%n bonds deleted (%1%)", 0, numSelected).arg(numSelected * 100 / std::max(numBonds, (size_t)1));

	return PipelineStatus(PipelineStatus::Success, std::move(statusMessage));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
