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

#include <plugins/stdobj/StdObj.h>
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
