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

#pragma once


#include <gui/GUI.h>
#include <core/oo/RefMaker.h>
#include <core/oo/RefTarget.h>
#include <core/viewport/overlays/ViewportOverlay.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * An item of the OverlayListModel representing a ViewportOverlay attached to a Viewport.
 */
class OverlayListItem : public RefMaker
{
	Q_OBJECT
	OVITO_CLASS(OverlayListItem)

public:

	/// Constructor.
	OverlayListItem(ViewportOverlay* overlay);

	/// Returns the status of the object represented by the list item.
	PipelineStatus status() const;

	/// Returns the title text for this list item.
	QString title() const;

Q_SIGNALS:

	/// This signal is emitted when this item has changed.
	void itemChanged(OverlayListItem* item);

protected:

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

private:

	/// The overlay represented by this item in the list box.
	DECLARE_REFERENCE_FIELD_FLAGS(ViewportOverlay, overlay, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_WEAK_REF | PROPERTY_FIELD_NO_CHANGE_MESSAGE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
