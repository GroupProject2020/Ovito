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
#include <core/oo/PropertyFieldDescriptor.h>
#include <core/oo/RefMaker.h>
#include <core/oo/OvitoObject.h>
#include <core/dataset/DataSet.h>
#include "RefMakerClass.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/******************************************************************************
* Is called by the system after construction of the meta-class instance.
******************************************************************************/
void RefMakerClass::initialize()
{
	OvitoClass::initialize();

	// Collect all property fields of the class hierarchy in one array.
	for(const RefMakerClass* clazz = this; clazz != &RefMaker::OOClass(); clazz = static_cast<const RefMakerClass*>(clazz->superClass())) {
		for(const PropertyFieldDescriptor* field = clazz->_firstPropertyField; field; field = field->next()) {
			_propertyFields.push_back(field);
		}
	}
}
	
/******************************************************************************
* Searches for a property field defined in this class or one of its super classes.
******************************************************************************/
const PropertyFieldDescriptor* RefMakerClass::findPropertyField(const char* identifier, bool searchSuperClasses) const
{
	if(!searchSuperClasses) {
		for(const PropertyFieldDescriptor* field = _firstPropertyField; field; field = field->next())
			if(qstrcmp(field->identifier(), identifier) == 0) return field;
	}
	else {
		for(const PropertyFieldDescriptor* field : _propertyFields) {
			if(qstrcmp(field->identifier(), identifier) == 0) return field;
		}
	}
	return nullptr;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
