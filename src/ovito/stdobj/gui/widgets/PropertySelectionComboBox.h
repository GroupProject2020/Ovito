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

#pragma once

#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/stdobj/properties/PropertyReference.h>
#include <ovito/stdobj/properties/PropertyObject.h>

namespace Ovito { namespace StdObj {

/**
 * \brief Widget that allows the user to select a property from a list (or enter a custom property name).
 */
class OVITO_STDOBJGUI_EXPORT PropertySelectionComboBox : public QComboBox
{
	Q_OBJECT

public:

	/// \brief Constructor.
	PropertySelectionComboBox(PropertyContainerClassPtr containerClass = nullptr, QWidget* parent = nullptr) : QComboBox(parent), _containerClass(containerClass) {}

	/// \brief Adds a property to the end of the list.
	/// \param property The property to add.
	void addItem(const PropertyReference& property, const QString& label = QString()) {
		OVITO_ASSERT(property.isNull() || containerClass() == property.containerClass());
		QComboBox::addItem(label.isEmpty() ? property.name() : label, QVariant::fromValue(property));
	}

	/// \brief Adds a property to the end of the list.
	/// \param property The property to add.
	void addItem(const PropertyObject* property, int vectorComponent = -1) {
		OVITO_ASSERT(property != nullptr);
		OVITO_ASSERT(containerClass() != nullptr);
		QString label = property->nameWithComponent(vectorComponent);
		if(QComboBox::findText(label) == -1) {
			QComboBox::addItem(label, QVariant::fromValue(PropertyReference(containerClass(), property, vectorComponent)));
		}
	}

	/// \brief Adds multiple properties to the combo box.
	/// \param list The list of properties to add.
	void addItems(const QVector<PropertyObject*>& list) {
		for(PropertyObject* p : list)
			addItem(p);
	}

	/// \brief Returns the particle property that is currently selected in the
	///        combo box.
	/// \return The selected item. The returned reference can be null if
	///         no item is currently selected.
	PropertyReference currentProperty() const;

	/// \brief Sets the selection of the combo box to the given particle property.
	/// \param property The particle property to be selected.
	void setCurrentProperty(const PropertyReference& property);

	/// \brief Returns the list index of the given property, or -1 if not found.
	int propertyIndex(const PropertyReference& property) const {
		for(int index = 0; index < count(); index++) {
			if(property == itemData(index).value<PropertyReference>())
				return index;
		}
		return -1;
	}

	/// \brief Returns the property at the given index.
	PropertyReference property(int index) const {
		return itemData(index).value<PropertyReference>();
	}

	/// Returns the class of properties that can be selected with this combo box.
	PropertyContainerClassPtr containerClass() const { return _containerClass; }

	/// Sets the class of properties that can be selected with this combo box.
	void setContainerClass(PropertyContainerClassPtr containerClass) {
		if(_containerClass != containerClass) {
			_containerClass = containerClass;
			clear();
		}
	}

protected:

	/// Is called when the widget loses the input focus.
	virtual void focusOutEvent(QFocusEvent* event) override;

private:

	/// Specifies the class of properties that can be selected in this combo box.
	PropertyContainerClassPtr _containerClass;
};

}	// End of namespace
}	// End of namespace
