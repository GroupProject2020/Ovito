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

#include <plugins/stdobj/StdObj.h>
#include <core/dataset/DataSet.h>
#include "PropertyContainer.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(PropertyContainer);	
DEFINE_REFERENCE_FIELD(PropertyContainer, properties);
SET_PROPERTY_FIELD_LABEL(PropertyContainer, properties, "Properties");

/******************************************************************************
* Constructor.
******************************************************************************/
PropertyContainer::PropertyContainer(DataSet* dataset) : DataObject(dataset)
{
}

/******************************************************************************
* Returns the given standard property. If it does not exist, an exception is thrown.
******************************************************************************/
PropertyObject* PropertyContainer::expectProperty(int typeId) const
{
	PropertyObject* property = getProperty(typeId);
	if(!property)
		throwException(tr("Property '%1' required but does not exist.").arg(propertyClass().standardPropertyName(typeId)));
	return property;
}

}	// End of namespace
}	// End of namespace
