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
#include <core/dataset/pipeline/PipelineFlowState.h>
#include "DataSeriesProperty.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(DataSeriesProperty);

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void DataSeriesProperty::OOMetaClass::initialize()
{
	PropertyClass::initialize();

	// Enable automatic conversion of a DataSeriesPropertyReference to a generic PropertyReference and vice versa.
	QMetaType::registerConverter<DataSeriesPropertyReference, PropertyReference>();
	QMetaType::registerConverter<PropertyReference, DataSeriesPropertyReference>();		

	setPropertyClassDisplayName(tr("Data series"));
	setElementDescriptionName(QStringLiteral("points"));
	setPythonName(QStringLiteral("series"));
}

/******************************************************************************
* Creates a storage object for standard data series properties.
******************************************************************************/
PropertyPtr DataSeriesProperty::OOMetaClass::createStandardStorage(size_t elementCount, int type, bool initializeMemory) const
{
	int dataType;
	size_t componentCount;
	size_t stride;

	switch(type) {
	case XProperty:
	case YProperty:
		dataType = PropertyStorage::Float;
		componentCount = 1;
		stride = sizeof(FloatType);
		break;
	default:
		OVITO_ASSERT_MSG(false, "DataSeriesProperty::createStandardStorage()", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard property type: %1").arg(type));
	}

	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));
	
	return std::make_shared<PropertyStorage>(elementCount, dataType, componentCount, stride, 
								propertyName, initializeMemory, type, componentNames);
}

/******************************************************************************
* Returns the data grid size in the given data state.
******************************************************************************/
size_t DataSeriesProperty::OOMetaClass::elementCount(const PipelineFlowState& state) const
{
	for(DataObject* obj : state.objects()) {
		if(DataSeriesProperty* property = dynamic_object_cast<DataSeriesProperty>(obj)) {
			return property->size();
		}
	}
	return 0;
}

/******************************************************************************
* Determines if the data elements which this property class applies to are 
* present for the given data state.
******************************************************************************/
bool DataSeriesProperty::OOMetaClass::isDataPresent(const PipelineFlowState& state) const
{
	return state.findObject<DataSeriesProperty>() != nullptr;
}

}	// End of namespace
}	// End of namespace
