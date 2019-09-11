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

#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/stdobj/properties/PropertyContainerClass.h>
#include "PropertySelectionComboBox.h"

namespace Ovito { namespace StdObj {

/******************************************************************************
* Returns the particle property that is currently selected in the combo box.
******************************************************************************/
PropertyReference PropertySelectionComboBox::currentProperty() const
{
	if(!isEditable()) {
		int index = currentIndex();
		if(index < 0)
			return PropertyReference();
		return itemData(index).value<PropertyReference>();
	}
	else {
		QString name = currentText().simplified();
		if(!name.isEmpty()) {
			if(int standardTypeId = containerClass()->standardPropertyTypeId(name))
				return PropertyReference(containerClass(), standardTypeId);
			else
				return PropertyReference(containerClass(), name);
		}
		else {
			return PropertyReference();
		}
	}
}

/******************************************************************************
* Sets the selection of the combo box to the given particle property.
******************************************************************************/
void PropertySelectionComboBox::setCurrentProperty(const PropertyReference& property)
{
	for(int index = 0; index < count(); index++) {
		if(property == itemData(index).value<PropertyReference>()) {
			setCurrentIndex(index);
			return;
		}
	}
	if(isEditable() && !property.isNull()) {
		setCurrentText(property.name());
	}
	else {
		setCurrentIndex(-1);
	}
}

/******************************************************************************
* Is called when the widget loses the input focus.
******************************************************************************/
void PropertySelectionComboBox::focusOutEvent(QFocusEvent* event)
{
	if(isEditable()) {
		int index = findText(currentText());
		if(index == -1 && currentText().isEmpty() == false) {
			addItem(PropertyReference(containerClass(), currentText()));
			index = count() - 1;
		}
		setCurrentIndex(index);
		Q_EMIT activated(index);
		Q_EMIT activated(currentText());
	}
	QComboBox::focusOutEvent(event);
}

}	// End of namespace
}	// End of namespace
