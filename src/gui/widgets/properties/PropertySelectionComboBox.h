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

#pragma once

#include <gui/GUI.h>
#include <core/dataset/data/properties/PropertyReference.h>
#include <core/dataset/data/properties/PropertyObject.h>
#include <core/dataset/data/properties/PropertyClass.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Widgets)
	
/**
 * \brief Widget that allows the user to select a property from a list (or enter a custom property name).
 */
class OVITO_GUI_EXPORT PropertySelectionComboBox : public QComboBox
{
	Q_OBJECT

public:

	/// \brief Constructor.
	PropertySelectionComboBox(const PropertyClass* propertyClass = nullptr, QWidget* parent = nullptr) : QComboBox(parent), _propertyClass(propertyClass) {}

	/// \brief Adds a property to the end of the list.
	/// \param property The property to add.
	void addItem(const PropertyReference& property, const QString& label = QString()) {
		OVITO_ASSERT(property.isNull() || propertyClass() == property.propertyClass());
		QComboBox::addItem(label.isEmpty() ? property.name() : label, QVariant::fromValue(property));
	}

	/// \brief Adds a property to the end of the list.
	/// \param property The property to add.
	void addItem(PropertyObject* property, int vectorComponent = -1) {
		OVITO_ASSERT(property != nullptr);
		OVITO_ASSERT(propertyClass()->isMember(property));
		QString label = property->nameWithComponent(vectorComponent);
		if(QComboBox::findText(label) == -1) {
			QComboBox::addItem(label, QVariant::fromValue(PropertyReference(property, vectorComponent)));
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
	const PropertyClass* propertyClass() const { return _propertyClass; }
		
	/// Sets the class of properties that can be selected with this combo box.
	void setPropertyClass(const PropertyClass* propertyClass) {
		if(_propertyClass != propertyClass) {
			_propertyClass = propertyClass;
			clear();
		}
	}
	
protected:

	/// Is called when the widget loses the input focus.
	virtual void focusOutEvent(QFocusEvent* event) override;

private:

	/// The class of properties that can be selected with this combo box.
	const PropertyClass* _propertyClass;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
