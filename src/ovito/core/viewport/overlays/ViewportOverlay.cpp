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

#include <ovito/core/Core.h>
#include <ovito/core/viewport/overlays/ViewportOverlay.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View)

IMPLEMENT_OVITO_CLASS(ViewportOverlay);
DEFINE_PROPERTY_FIELD(ViewportOverlay, renderBehindScene);
DEFINE_PROPERTY_FIELD(ViewportOverlay, isEnabled);
DEFINE_PROPERTY_FIELD(ViewportOverlay, status);
SET_PROPERTY_FIELD_LABEL(ViewportOverlay, renderBehindScene, "Draw behind scene");
SET_PROPERTY_FIELD_LABEL(ViewportOverlay, isEnabled, "Enabled");
SET_PROPERTY_FIELD_LABEL(ViewportOverlay, status, "Status");
SET_PROPERTY_FIELD_CHANGE_EVENT(ViewportOverlay, isEnabled, ReferenceEvent::TargetEnabledOrDisabled);
SET_PROPERTY_FIELD_CHANGE_EVENT(ViewportOverlay, status, ReferenceEvent::ObjectStatusChanged);

/******************************************************************************
* Constructor.
******************************************************************************/
ViewportOverlay::ViewportOverlay(DataSet* dataset) : RefTarget(dataset),
    _renderBehindScene(false),
    _isEnabled(true)
{
}

/******************************************************************************
* Is called when the value of a non-animatable property field of this RefMaker has changed.
******************************************************************************/
void ViewportOverlay::propertyChanged(const PropertyFieldDescriptor& field)
{
    // If the overlay gets disabled, reset the status it indicates.
	if(field == PROPERTY_FIELD(isEnabled) && !isEnabled())
		setStatus(PipelineStatus::Success);

    RefTarget::propertyChanged(field);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
