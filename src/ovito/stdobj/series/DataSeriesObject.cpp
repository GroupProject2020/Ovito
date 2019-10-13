////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/core/dataset/DataSet.h>
#include "DataSeriesObject.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(DataSeriesObject);
DEFINE_PROPERTY_FIELD(DataSeriesObject, title);
DEFINE_PROPERTY_FIELD(DataSeriesObject, intervalStart);
DEFINE_PROPERTY_FIELD(DataSeriesObject, intervalEnd);
DEFINE_PROPERTY_FIELD(DataSeriesObject, axisLabelX);
DEFINE_PROPERTY_FIELD(DataSeriesObject, axisLabelY);
DEFINE_PROPERTY_FIELD(DataSeriesObject, plotMode);
SET_PROPERTY_FIELD_CHANGE_EVENT(DataSeriesObject, title, ReferenceEvent::TitleChanged);

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void DataSeriesObject::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	// Enable automatic conversion of a DataSeriesPropertyReference to a generic PropertyReference and vice versa.
	QMetaType::registerConverter<DataSeriesPropertyReference, PropertyReference>();
	QMetaType::registerConverter<PropertyReference, DataSeriesPropertyReference>();

	setPropertyClassDisplayName(tr("Data series"));
	setElementDescriptionName(QStringLiteral("points"));
	setPythonName(QStringLiteral("series"));

	const QStringList emptyList;
	registerStandardProperty(XProperty, tr("X"), PropertyStorage::Float, emptyList);
	registerStandardProperty(YProperty, tr("Y"), PropertyStorage::Float, emptyList);
}

/******************************************************************************
* Creates a storage object for standard data series properties.
******************************************************************************/
PropertyPtr DataSeriesObject::OOMetaClass::createStandardStorage(size_t elementCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath) const
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
		OVITO_ASSERT_MSG(false, "DataSeriesObject::createStandardStorage()", "Invalid standard property type");
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
DataSeriesObject::DataSeriesObject(DataSet* dataset, PlotMode plotMode, const QString& title, const PropertyPtr& y, const PropertyPtr& x) : PropertyContainer(dataset),
	_title(title),
	_intervalStart(0),
	_intervalEnd(0),
	_plotMode(plotMode)
{
	OVITO_ASSERT(!x || !y || x->size() == y->size());
	if(x) {
		OVITO_ASSERT(x->type() == XProperty);
		createProperty(x);
	}
	if(y) {
		OVITO_ASSERT(y->type() == YProperty);
		createProperty(y);
	}
}

/******************************************************************************
* Returns the display title of this object in the user interface.
******************************************************************************/
QString DataSeriesObject::objectTitle() const
{
	return !title().isEmpty() ? title() : identifier();
}

/******************************************************************************
* Returns the data array containing the x-coordinates of the data points.
* If no explicit x-coordinate data is available, the array is dynamically generated
* from the x-axis interval set for this data series.
******************************************************************************/
ConstPropertyPtr DataSeriesObject::getXStorage() const
{
	if(ConstPropertyPtr xStorage = getPropertyStorage(XProperty)) {
		return xStorage;
	}
	else if(const PropertyObject* yProperty = getY()) {
		auto xdata = OOClass().createStandardStorage(elementCount(), XProperty, false);
		FloatType binSize = (intervalEnd() - intervalStart()) / xdata->size();
		FloatType x = intervalStart() + binSize * FloatType(0.5);
		for(FloatType& v : xdata->floatRange()) {
			v = x;
			x += binSize;
		}
		return std::move(xdata);
	}
	else {
		return {};
	}
}

}	// End of namespace
}	// End of namespace
