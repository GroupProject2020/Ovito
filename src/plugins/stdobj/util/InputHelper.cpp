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
#include <core/dataset/DataSet.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include <plugins/stdobj/properties/PropertyClass.h>
#include "InputHelper.h"

namespace Ovito { namespace StdObj {
	
/******************************************************************************
* Returns a standard particle property from the input state.
******************************************************************************/
PropertyObject* InputHelper::inputStandardProperty(const PropertyClass& propertyClass, int typeId, const QString& bundle) const
{
	OVITO_ASSERT(typeId != 0);
	return propertyClass.findInState(input(), typeId, bundle);
}

/******************************************************************************
* Returns the property with the given identifier from the input object.
******************************************************************************/
PropertyObject* InputHelper::expectCustomProperty(const PropertyClass& propertyClass, const QString& propertyName, int dataType, size_t componentCount, const QString& bundle) const
{
	for(DataObject* o : input().objects()) {
		PropertyObject* property = dynamic_object_cast<PropertyObject>(o);
		if(property && propertyClass.isMember(property) && property->name() == propertyName && property->bundle() == bundle) {
			if(property->dataType() != dataType)
				dataset()->throwException(PropertyObject::tr("Property '%1' does not have the required data type.").arg(property->name()));
			if(property->componentCount() != componentCount)
				dataset()->throwException(PropertyObject::tr("Property '%1' does not have the required number of components.").arg(property->name()));
			return property;
		}
	}
	dataset()->throwException(PropertyObject::tr("Modifier requires input property '%1', which is not defined for '%2' data elements.").arg(propertyName).arg(propertyClass.pythonName()));
}

/******************************************************************************
* Returns the given standard particle property from the input object.
* The returned property may not be modified. If they input object does
* not contain the standard property then an exception is thrown.
******************************************************************************/
PropertyObject* InputHelper::expectStandardProperty(const PropertyClass& propertyClass, int typeId, const QString& bundle) const
{
	PropertyObject* property = inputStandardProperty(propertyClass, typeId, bundle);
	if(!property)
		dataset()->throwException(PropertyObject::tr("Modifier requires input property '%1', which is not defined for '%2' data elements.").arg(propertyClass.standardPropertyName(typeId)).arg(propertyClass.pythonName()));
	return property;
}

/******************************************************************************
* Returns the input simulation cell.
******************************************************************************/
SimulationCellObject* InputHelper::expectSimulationCell() const
{
	SimulationCellObject* cell = input().findObject<SimulationCellObject>();
	if(!cell)
		dataset()->throwException(SimulationCellObject::tr("Modifier requires an input simulation cell."));
	return cell;
}

}	// End of namespace
}	// End of namespace
