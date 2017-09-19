///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include <plugins/particles/util/ParticleSelectionSet.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <core/viewport/ViewportConfiguration.h>
#include "ManualSelectionModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)



DEFINE_FLAGS_REFERENCE_FIELD(ManualSelectionModifierApplication, selectionSet, "SelectionSet", ParticleSelectionSet, PROPERTY_FIELD_ALWAYS_CLONE);
SET_PROPERTY_FIELD_LABEL(ManualSelectionModifierApplication, selectionSet, "Particle selection set");

/******************************************************************************
* Constructor.
******************************************************************************/
ManualSelectionModifierApplication::ManualSelectionModifierApplication(DataSet* dataset) : ModifierApplication(dataset)
{

}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool ManualSelectionModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Create a new modifier application that refers to this modifier instance.
******************************************************************************/
OORef<ModifierApplication> ManualSelectionModifier::createModifierApplication()
{
	OORef<ModifierApplication> modApp = new ManualSelectionModifierApplication(dataset());
	modApp->setModifier(this);
	return modApp;
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState ManualSelectionModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = input;
	ParticleInputHelper pih(dataset(), input);
	ParticleOutputHelper poh(dataset(), output);

	// Retrieve the selection stored in the modifier application.
	ParticleSelectionSet* selectionSet = getSelectionSet(modApp, false);
	if(!selectionSet)
		throwException(tr("No stored selection set available. Please reset the selection state."));

	PipelineStatus status = selectionSet->applySelection(
			poh.outputStandardProperty<ParticleProperty>(ParticleProperty::SelectionProperty),
			pih.inputStandardProperty<ParticleProperty>(ParticleProperty::IdentifierProperty));

	output.setStatus(std::move(status));
	return output;
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void ManualSelectionModifier::initializeModifier(ModifierApplication* modApp)
{
	Modifier::initializeModifier(modApp);

	// Take a snapshot of the existing selection state at the time the modifier is created.
	if(!getSelectionSet(modApp, false)) {
		PipelineFlowState input = modApp->evaluateInputPreliminary();
		resetSelection(modApp, input);
	}
}

/******************************************************************************
* Returns the selection set object stored in the ModifierApplication, or, if
* it does not exist, creates one.
******************************************************************************/
ParticleSelectionSet* ManualSelectionModifier::getSelectionSet(ModifierApplication* modApp, bool createIfNotExist)
{
	ManualSelectionModifierApplication* myModApp = dynamic_object_cast<ManualSelectionModifierApplication>(modApp);
	if(!myModApp)
		throwException(tr("Manual selection modifier is not referenced by a ManualSelectionModifierApplication."));

	ParticleSelectionSet* selectionSet = myModApp->selectionSet();
	if(!selectionSet && createIfNotExist)
		myModApp->setSelectionSet(selectionSet = new ParticleSelectionSet(dataset()));

	return selectionSet;
}

/******************************************************************************
* Adopts the selection state from the modifier's input.
******************************************************************************/
void ManualSelectionModifier::resetSelection(ModifierApplication* modApp, const PipelineFlowState& state)
{
	getSelectionSet(modApp, true)->resetSelection(state);
}

/******************************************************************************
* Selects all particles.
******************************************************************************/
void ManualSelectionModifier::selectAll(ModifierApplication* modApp, const PipelineFlowState& state)
{
	getSelectionSet(modApp, true)->selectAll(state);
}

/******************************************************************************
* Deselects all particles.
******************************************************************************/
void ManualSelectionModifier::clearSelection(ModifierApplication* modApp, const PipelineFlowState& state)
{
	getSelectionSet(modApp, true)->clearSelection(state);
}

/******************************************************************************
* Toggles the selection state of a single particle.
******************************************************************************/
void ManualSelectionModifier::toggleParticleSelection(ModifierApplication* modApp, const PipelineFlowState& state, size_t particleIndex)
{
	ParticleSelectionSet* selectionSet = getSelectionSet(modApp, false);
	if(!selectionSet)
		throwException(tr("No stored selection set available. Please reset the selection state."));
	selectionSet->toggleParticle(state, particleIndex);
}

/******************************************************************************
* Replaces the particle selection.
******************************************************************************/
void ManualSelectionModifier::setParticleSelection(ModifierApplication* modApp, const PipelineFlowState& state, const QBitArray& selection, ParticleSelectionSet::SelectionMode mode)
{
	getSelectionSet(modApp, true)->setParticleSelection(state, selection, mode);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
