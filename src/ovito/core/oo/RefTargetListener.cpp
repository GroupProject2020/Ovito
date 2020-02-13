////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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
#include <ovito/core/oo/RefTargetListener.h>
#include <ovito/core/dataset/DataSet.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(RefTargetListenerBase);
IMPLEMENT_OVITO_CLASS(VectorRefTargetListenerBase);
DEFINE_REFERENCE_FIELD(RefTargetListenerBase, target);
DEFINE_REFERENCE_FIELD(VectorRefTargetListenerBase, targets);

/******************************************************************************
* Is called when the RefTarget referenced by this listener has sent a message.
******************************************************************************/
bool RefTargetListenerBase::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	// Emit Qt signal.
	Q_EMIT notificationEvent(event);

	return RefMaker::referenceEvent(source, event);
}

/******************************************************************************
* Is called when the RefTarget referenced by this listener has sent a message.
******************************************************************************/
bool VectorRefTargetListenerBase::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	// Emit Qt signal.
	Q_EMIT notificationEvent(source, event);

	return RefMaker::referenceEvent(source, event);
}

}	// End of namespace

