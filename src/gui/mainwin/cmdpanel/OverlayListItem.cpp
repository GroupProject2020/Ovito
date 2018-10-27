///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <gui/GUI.h>
#include "OverlayListItem.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(OverlayListItem);
DEFINE_REFERENCE_FIELD(OverlayListItem, overlay);

/******************************************************************************
* Constructor.
******************************************************************************/
OverlayListItem::OverlayListItem(ViewportOverlay* overlay)
{
	_overlay.set(this, PROPERTY_FIELD(overlay), overlay);
}

/******************************************************************************
* This method is called when the object presented by the modifier
* list item generates a message.
******************************************************************************/
bool OverlayListItem::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	/// Update item if it has been enabled/disabled, its status has changed, or its title has changed.
	if(event.type() == ReferenceEvent::TargetEnabledOrDisabled || event.type() == ReferenceEvent::ObjectStatusChanged || event.type() == ReferenceEvent::TitleChanged) {
		Q_EMIT itemChanged(this);
	}

	return RefMaker::referenceEvent(source, event);
}

/******************************************************************************
* Returns the status of the object represented by the list item.
******************************************************************************/
PipelineStatus OverlayListItem::status() const
{
	if(overlay())
		return overlay()->status();
	else
		return {};
}

/******************************************************************************
* Returns the text for this list item.
******************************************************************************/
QString OverlayListItem::title() const
{ 
	return overlay() ? overlay()->objectTitle() : QString();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
