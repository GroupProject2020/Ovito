////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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
