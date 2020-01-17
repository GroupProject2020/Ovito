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

/******************************************************************************
* Returns the default color for a numeric type ID.
******************************************************************************/
const Color& ElementType::getDefaultColorForId(int typeClass, int typeId)
{
	// Initial standard colors assigned to new element types:
	static const Color defaultTypeColors[] = {
		Color(0.4,  1.0,  0.2),
		Color(1.0,  0.4,  0.4),
		Color(0.4,  0.4,  1.0),
		Color(0.8,  1.0,  0.7),
		Color(0.97, 0.97, 0.97),
		Color(1.0,  1.0,  0.0),
		Color(1.0,  0.4,  1.0),
		Color(0.7,  0.0,  1.0),
		Color(0.2,  1.0,  1.0),
	};
	return defaultTypeColors[std::abs(typeId) % (sizeof(defaultTypeColors) / sizeof(defaultTypeColors[0]))];
}

/******************************************************************************
* Returns the default color for a element type name.
******************************************************************************/
Color ElementType::getDefaultColor(int typeClass, const QString& typeName, int typeId, bool useUserDefaults)
{
	if(useUserDefaults) {
		QSettings settings;
		settings.beginGroup("defaults/color");
		settings.beginGroup(QString::number(typeClass));
		QVariant v = settings.value(typeName);
		if(v.isValid() && v.type() == QVariant::Color)
			return v.value<Color>();
	}

	return getDefaultColorForId(typeClass, typeId);
}

/******************************************************************************
* Changes the default color for an element type name.
******************************************************************************/
void ElementType::setDefaultColor(int typeClass, const QString& typeName, const Color& color)
{
	QSettings settings;
	settings.beginGroup("defaults/color");
	settings.beginGroup(QString::number(typeClass));

	if(getDefaultColor(typeClass, typeName, 0, false) != color)
		settings.setValue(typeName, QVariant::fromValue((QColor)color));
	else
		settings.remove(typeName);
}

}	// End of namespace
}	// End of namespace
