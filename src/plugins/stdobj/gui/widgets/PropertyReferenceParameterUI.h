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


#include <plugins/stdobj/gui/StdObjGui.h>
#include <gui/properties/ParameterUI.h>
#include <plugins/stdobj/gui/widgets/PropertySelectionComboBox.h>

namespace Ovito { namespace StdObj {
	
/**
 * \brief This parameter UI lets the user select a property.
 */
class OVITO_STDOBJGUI_EXPORT PropertyReferenceParameterUI : public PropertyParameterUI
{
	Q_OBJECT
	OVITO_CLASS(PropertyReferenceParameterUI)
	
public:

	/// Constructor.
	PropertyReferenceParameterUI(QObject* parentEditor, const char* propertyName, const PropertyClass* propertyClass, bool showComponents = true, bool inputProperty = true);

	/// Constructor.
	PropertyReferenceParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField, const PropertyClass* propertyClass, bool showComponents = true, bool inputProperty = true);
	
	/// Destructor.
	virtual ~PropertyReferenceParameterUI();
	
	/// This returns the combo box managed by this ParameterUI.
	QComboBox* comboBox() const { return _comboBox; }
	
	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.  
	virtual void resetUI() override;

	/// This method updates the displayed value of the property UI.
	virtual void updateUI() override;
	
	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;
	
	/// Sets the tooltip text for the combo box widget.
	void setToolTip(const QString& text) const { 
		if(comboBox()) comboBox()->setToolTip(text); 
	}
	
	/// Sets the What's This helper text for the combo box.
	void setWhatsThis(const QString& text) const { 
		if(comboBox()) comboBox()->setWhatsThis(text); 
	}
	
	/// Returns the class of properties that can be selected by the user.
	const PropertyClass* propertyClass() const { return _propertyClass; }

	/// Sets the class of properties that can be selected by the user.
	void setPropertyClass(const PropertyClass* propertyClass) {
		if(_propertyClass != propertyClass) {
			_propertyClass = propertyClass;
			_comboBox->setPropertyClass(propertyClass);
			updateUI();
		}
	}

	/// Installs optional callback function that allows clients to filter the displayed property list.
	void setPropertyFilter(std::function<bool(PropertyObject*)> filter) {
		_propertyFilter = std::move(filter);
	}
		
public:
	
	Q_PROPERTY(QComboBox comboBox READ comboBox);
	
public Q_SLOTS:
	
	/// Takes the value entered by the user and stores it in the property field 
	/// this property UI is bound to. 
	void updatePropertyValue();
	
protected:

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Returns the value currently set for the property field.
	PropertyReference getPropertyReference();

	/// Populates the combox box with items.
	void addItemsToComboBox(const PipelineFlowState& state);

protected:

	/// The combo box of the UI component.
	QPointer<PropertySelectionComboBox> _comboBox;

	/// Controls whether the combo box should display a separate entry for each component of a property.
	bool _showComponents;

	/// Controls whether the combo box should list input or output properties.
	bool _inputProperty;

	/// The class of properties that can be selected.
	const PropertyClass* _propertyClass;

	/// An optional callback function that allows clients to filter the displayed property list.
	std::function<bool(PropertyObject*)> _propertyFilter;
};

}	// End of namespace
}	// End of namespace
