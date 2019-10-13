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

#include <ovito/stdobj/StdObj.h>
#include "ElementType.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(ElementType);
DEFINE_PROPERTY_FIELD(ElementType, numericId);
DEFINE_PROPERTY_FIELD(ElementType, color);
DEFINE_PROPERTY_FIELD(ElementType, name);
DEFINE_PROPERTY_FIELD(ElementType, enabled);
SET_PROPERTY_FIELD_LABEL(ElementType, numericId, "Id");
SET_PROPERTY_FIELD_LABEL(ElementType, color, "Color");
SET_PROPERTY_FIELD_LABEL(ElementType, name, "Name");
SET_PROPERTY_FIELD_LABEL(ElementType, enabled, "Enabled");
SET_PROPERTY_FIELD_CHANGE_EVENT(ElementType, name, ReferenceEvent::TitleChanged);
SET_PROPERTY_FIELD_CHANGE_EVENT(ElementType, enabled, ReferenceEvent::TargetEnabledOrDisabled);

/******************************************************************************
* Constructs a new ElementType.
******************************************************************************/
ElementType::ElementType(DataSet* dataset) : DataObject(dataset),
	_numericId(0),
	_color(1,1,1),
	_enabled(true)
{
}

}	// End of namespace
}	// End of namespace
