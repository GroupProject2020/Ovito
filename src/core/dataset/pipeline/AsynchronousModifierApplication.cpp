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

#include <core/Core.h>
#include <core/dataset/pipeline/AsynchronousModifierApplication.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(AsynchronousModifierApplication);

/******************************************************************************
* Constructor.
******************************************************************************/
AsynchronousModifierApplication::AsynchronousModifierApplication(DataSet* dataset) : ModifierApplication(dataset)
{
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool AsynchronousModifierApplication::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(event->type() == ReferenceEvent::TargetEnabledOrDisabled && source == modifier()) {
		// Throw away cached results when the modifier is disabled.
		_lastComputeResults.reset();
	}
	else if(event->type() == ReferenceEvent::PreliminaryStateAvailable && source == input()) {
		// Throw away cached results when the modifier's input changes.
		if(_lastComputeResults && !_lastComputeResults->isReapplicable())
			_lastComputeResults.reset();
	}
	return ModifierApplication::referenceEvent(source, event);
}

/******************************************************************************
* Gets called when the data object of the node has been replaced.
******************************************************************************/
void AsynchronousModifierApplication::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	// Throw away cached results when the Modifier is detached from the ModifierApplication.
	if(field == PROPERTY_FIELD(modifier)) {
		_lastComputeResults.reset();
	}
	ModifierApplication::referenceReplaced(field, oldTarget, newTarget);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
