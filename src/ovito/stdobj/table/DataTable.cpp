////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/stdobj/properties/PropertyAccess.h>
#include "DataTable.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(DataTable);
DEFINE_PROPERTY_FIELD(DataTable, title);
DEFINE_PROPERTY_FIELD(DataTable, intervalStart);
DEFINE_PROPERTY_FIELD(DataTable, intervalEnd);
DEFINE_PROPERTY_FIELD(DataTable, axisLabelX);
DEFINE_PROPERTY_FIELD(DataTable, axisLabelY);
DEFINE_PROPERTY_FIELD(DataTable, plotMode);
SET_PROPERTY_FIELD_CHANGE_EVENT(DataTable, title, ReferenceEvent::TitleChanged);

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void DataTable::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	// Enable automatic conversion of a DataTablePropertyReference to a generic PropertyReference and vice versa.
	QMetaType::registerConverter<DataTablePropertyReference, PropertyReference>();
	QMetaType::registerConverter<PropertyReference, DataTablePropertyReference>();

	setPropertyClassDisplayName(tr("Data table"));
	setElementDescriptionName(QStringLiteral("points"));
	setPythonName(QStringLiteral("table"));

	const QStringList emptyList;
	registerStandardProperty(XProperty, QString(), PropertyStorage::Float, emptyList);
	registerStandardProperty(YProperty, QString(), PropertyStorage::Float, emptyList);
}

/******************************************************************************
* Creates a storage object for standard data table properties.
******************************************************************************/
PropertyPtr DataTable::OOMetaClass::createStandardStorage(size_t elementCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath) const
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
		OVITO_ASSERT_MSG(false, "DataTable::createStandardStorage()", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard property type: %1").arg(type));
	}

	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));

	return std::make_shared<PropertyStorage>(elementCount, dataType, componentCount, stride,
								propertyName, initializeMemory, type, componentNames);
}

/******************************************************************************
* Constructor.
******************************************************************************/
DataTable::DataTable(DataSet* dataset, PlotMode plotMode, const QString& title, PropertyPtr y, PropertyPtr x) : PropertyContainer(dataset),
	_title(title),
	_intervalStart(0),
	_intervalEnd(0),
	_plotMode(plotMode)
{
	OVITO_ASSERT(!x || !y || x->size() == y->size());
	if(x) {
		OVITO_ASSERT(x->type() == XProperty);
		createProperty(std::move(x));
	}
	if(y) {
		OVITO_ASSERT(y->type() == YProperty);
		createProperty(std::move(y));
	}
}

/******************************************************************************
* Returns the display title of this object in the user interface.
******************************************************************************/
QString DataTable::objectTitle() const
{
	return !title().isEmpty() ? title() : identifier();
}

/******************************************************************************
* Returns the data array containing the x-coordinates of the data points.
* If no explicit x-coordinate data is available, the array is dynamically generated
* from the x-axis interval set for this data table.
******************************************************************************/
ConstPropertyPtr DataTable::getXStorage() const
{
	if(ConstPropertyPtr xStorage = getPropertyStorage(XProperty)) {
		return xStorage;
	}
	else if(const PropertyObject* yProperty = getY()) {
		if(intervalStart() != 0 || intervalEnd() != 0) {
			auto xstorage = OOClass().createStandardStorage(elementCount(), XProperty, false);
			xstorage->setName(axisLabelX());
			PropertyAccess<FloatType> xdata(xstorage);
			FloatType binSize = (intervalEnd() - intervalStart()) / xdata.size();
			FloatType x = intervalStart() + binSize * FloatType(0.5);
			for(FloatType& v : xdata) {
				v = x;
				x += binSize;
			}
			return std::move(xstorage);
		}
		else {
			auto xstorage = std::make_shared<PropertyStorage>(elementCount(), PropertyStorage::Int64, 1, 0, axisLabelX(), false, DataTable::XProperty);
			PropertyAccess<qlonglong> xdata(xstorage);
			std::iota(xdata.begin(), xdata.end(), (size_t)0);
			return std::move(xstorage);
		}
	}
	else {
		return {};
	}
}

}	// End of namespace
}	// End of namespace
