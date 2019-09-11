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

#include <ovito/stdobj/StdObj.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include "PropertyObject.h"
#include "PropertyReference.h"
#include "PropertyContainerClass.h"

namespace Ovito { namespace StdObj {

/******************************************************************************
* This helper method returns a standard property (if present) from the
* given pipeline state.
******************************************************************************/
void PropertyContainerClass::registerStandardProperty(int typeId, QString name, int dataType, QStringList componentNames, QString title)
{
	OVITO_ASSERT_MSG(typeId > 0, "PropertyContainerClass::registerStandardProperty", "Invalid standard property type ID");
	OVITO_ASSERT_MSG(!_standardPropertyIds.contains(name), "PropertyContainerClass::registerStandardProperty", "Duplicate standard property name");
	OVITO_ASSERT_MSG(!_standardPropertyNames.contains(typeId), "PropertyContainerClass::registerStandardProperty", "Duplicate standard property type ID");
	OVITO_ASSERT_MSG(dataType == PropertyStorage::Int || dataType == PropertyStorage::Int64 || dataType == PropertyStorage::Float, "PropertyContainerClass::registerStandardProperty", "Invalid standard property data type");

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
OORef<PropertyObject> PropertyContainerClass::createFromStorage(DataSet* dataset, const PropertyPtr& storage) const
{
	OORef<PropertyObject> property = new PropertyObject(dataset, storage);
	if(property->type() != 0)
		property->setTitle(standardPropertyTitle(property->type()));
	prepareNewProperty(property);
	return property;
}

}	// End of namespace
}	// End of namespace
