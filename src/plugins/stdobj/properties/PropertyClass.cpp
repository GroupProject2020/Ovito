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
#include <core/dataset/pipeline/PipelineFlowState.h>
#include "PropertyObject.h"
#include "PropertyClass.h"
#include "PropertyReference.h"

namespace Ovito { namespace StdObj {
	
/******************************************************************************
* This helper method returns a standard property (if present) from the
* given pipeline state.
******************************************************************************/
void PropertyClass::registerStandardProperty(int typeId, QString name, int dataType, QStringList componentNames, QString title)
{
	OVITO_ASSERT_MSG(typeId > 0, "PropertyClass::registerStandardProperty", "Invalid standard property type ID");
	OVITO_ASSERT_MSG(!_standardPropertyIds.contains(name), "PropertyClass::registerStandardProperty", "Duplicate standard property name");
	OVITO_ASSERT_MSG(!_standardPropertyNames.contains(typeId), "PropertyClass::registerStandardProperty", "Duplicate standard property type ID");
	OVITO_ASSERT_MSG(dataType == qMetaTypeId<int>() || dataType == qMetaTypeId<qlonglong>() || dataType == qMetaTypeId<FloatType>(), "PropertyClass::registerStandardProperty", "Invalid standard property data type");

	_standardPropertyList.push_back(typeId);
	_standardPropertyIds.insert(name, typeId);
	_standardPropertyNames.insert(typeId, std::move(name));
	_standardPropertyTitles.insert(typeId, std::move(title));
	_standardPropertyComponents.insert(typeId, std::move(componentNames));
	_standardPropertyDataTypes.insert(typeId, dataType);
}

/******************************************************************************
* Factory function that creates a property object based on an existing storage.
******************************************************************************/
OORef<PropertyObject> PropertyClass::createFromStorage(DataSet* dataset, const PropertyPtr& storage) const
{
	OORef<PropertyObject> property = static_object_cast<PropertyObject>(createInstance(dataset));
	property->setStorage(storage);
	prepareNewProperty(property);
	return property;
}

/******************************************************************************
* This helper method returns a standard property (if present) from the
* given pipeline state.
******************************************************************************/
PropertyObject* PropertyClass::findInState(const PipelineFlowState& state, int typeId) const
{
	for(DataObject* o : state.objects()) {
		PropertyObject* property = dynamic_object_cast<PropertyObject>(o);
		if(property && isMember(property) && property->type() == typeId)
			return property;
	}
	return nullptr;
}

/******************************************************************************
* This helper method returns a specific user-defined property (if present) from the
* given pipeline state.
******************************************************************************/
PropertyObject* PropertyClass::findInState(const PipelineFlowState& state, const QString& name) const
{
	for(DataObject* o : state.objects()) {
		PropertyObject* property = dynamic_object_cast<PropertyObject>(o);
		if(property && isMember(property) && property->type() == 0 && property->name() == name)
			return property;
	}
	return nullptr;
}

}	// End of namespace
}	// End of namespace
