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

#include <plugins/stdobj/gui/StdObjGui.h>
#include <gui/properties/PropertiesEditor.h>
#include <core/app/PluginManager.h>
#include <core/dataset/pipeline/Modifier.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <plugins/stdobj/properties/PropertyClass.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include "PropertyClassParameterUI.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(PropertyClassParameterUI);

/******************************************************************************
* Constructor.
******************************************************************************/
PropertyClassParameterUI::PropertyClassParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField) : 
	PropertyParameterUI(parentEditor, propField),
	_comboBox(new QComboBox())
{
	connect(comboBox(), static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), this, &PropertyClassParameterUI::updatePropertyValue);

	// Populate combo box with the list of available property classes.
	for(PropertyClassPtr propertyClass : PluginManager::instance().metaclassMembers<PropertyObject>()) {
		comboBox()->addItem(propertyClass->propertyClassDisplayName(), QVariant::fromValue(propertyClass));
	}
}

/******************************************************************************
* Destructor.
******************************************************************************/
PropertyClassParameterUI::~PropertyClassParameterUI()
{
	delete comboBox(); 
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to. 
******************************************************************************/
void PropertyClassParameterUI::resetUI()
{
	PropertyParameterUI::resetUI();	
	
	if(comboBox()) 
		comboBox()->setEnabled(editObject() && isEnabled());
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool PropertyClassParameterUI::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject() && event.type() == ReferenceEvent::ModifierInputChanged) {
		// The modifier's input from the pipeline has changed -> update list of available delegates
		updateUI();
	}
	return PropertyParameterUI::referenceEvent(source, event);
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the 
* properties owner this parameter UI belongs to. 
******************************************************************************/
void PropertyClassParameterUI::updateUI()
{
	PropertyParameterUI::updateUI();
	
	if(comboBox() && editObject()) {

		// Get the current property class.
		QVariant val = editObject()->getPropertyFieldValue(*propertyField());
		OVITO_ASSERT_MSG(val.isValid() && val.canConvert<const PropertyClass*>(), "PropertyClassParameterUI::updateUI()", QString("The property field of object class %1 is not of type <PropertyClassPtr>.").arg(editObject()->metaObject()->className()).toLocal8Bit().constData());
		const PropertyClass* selectedPropertyClass = val.value<const PropertyClass*>();	
		
		// Obtain modifier input data.
		std::vector<PipelineFlowState> modifierInputs;
		if(Modifier* mod = dynamic_object_cast<Modifier>(editObject())) {
			for(ModifierApplication* modApp : mod->modifierApplications()) {
				modifierInputs.push_back(modApp->evaluateInputPreliminary());
			}
		}

		// Update enabled state of the property classes in the list.
		const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(comboBox()->model());
		int enabledCount = 0;
		int selectedIndex = -1;
		for(int i = 0; i < comboBox()->count(); i++) {
			QStandardItem* item = model->item(i);
			const PropertyClass* pclass = item->data(Qt::UserRole).value<const PropertyClass*>();
			if(pclass == selectedPropertyClass) 
				selectedIndex = i;

			item->setEnabled(std::any_of(modifierInputs.begin(), modifierInputs.end(), [pclass](const PipelineFlowState& state) {
				return pclass->isDataPresent(state);
			}));
		}

		comboBox()->setCurrentIndex(selectedIndex);
	}
}

/******************************************************************************
* Takes the value entered by the user and stores it in the property field 
* this property UI is bound to.
******************************************************************************/
void PropertyClassParameterUI::updatePropertyValue()
{
	if(comboBox() && editObject()) {
		undoableTransaction(tr("Change modifier target property class"), [this]() {

			const PropertyClass* pclass = comboBox()->currentData().value<const PropertyClass*>();

			// Check if new value differs from old value.
			QVariant oldval = editObject()->getPropertyFieldValue(*propertyField());
			if(pclass == oldval.value<const PropertyClass*>())
				return;

			editObject()->setPropertyFieldValue(*propertyField(), QVariant::fromValue(pclass));
			
			Q_EMIT valueEntered();
		});
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void PropertyClassParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	PropertyParameterUI::setEnabled(enabled);
	if(comboBox()) comboBox()->setEnabled(editObject() != NULL && isEnabled());
}

}	// End of namespace
}	// End of namespace
