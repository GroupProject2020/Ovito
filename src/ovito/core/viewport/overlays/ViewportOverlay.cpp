////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
