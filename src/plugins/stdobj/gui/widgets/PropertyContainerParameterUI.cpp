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
#include <plugins/stdobj/properties/PropertyContainerClass.h>
#include <plugins/stdobj/properties/PropertyContainer.h>
#include "PropertyContainerParameterUI.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(PropertyContainerParameterUI);

/******************************************************************************
* Constructor.
******************************************************************************/
PropertyContainerParameterUI::PropertyContainerParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField) : 
	PropertyParameterUI(parentEditor, propField),
	_comboBox(new QComboBox())
{
	connect(comboBox(), static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), this, &PropertyContainerParameterUI::updatePropertyValue);

	// Populate combo box with the list of available property container types.
	for(PropertyContainerClassPtr clazz : PluginManager::instance().metaclassMembers<PropertyContainer>()) {
		comboBox()->addItem(clazz->propertyClassDisplayName(), QVariant::fromValue(PropertyContainerReference(clazz)));
	}
}

/******************************************************************************
* Destructor.
******************************************************************************/
PropertyContainerParameterUI::~PropertyContainerParameterUI()
{
	delete comboBox(); 
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to. 
******************************************************************************/
void PropertyContainerParameterUI::resetUI()
{
	PropertyParameterUI::resetUI();	
	
	if(comboBox()) 
		comboBox()->setEnabled(editObject() && isEnabled());
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool PropertyContainerParameterUI::referenceEvent(RefTarget* source, const ReferenceEvent& event)
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
void PropertyContainerParameterUI::updateUI()
{
	PropertyParameterUI::updateUI();
	
	if(comboBox() && editObject()) {

		// Get the current property class.
		QVariant val = editObject()->getPropertyFieldValue(*propertyField());
		OVITO_ASSERT_MSG(val.isValid() && val.canConvert<PropertyContainerReference>(), "PropertyContainerParameterUI::updateUI()", QString("The property field of object class %1 is not of type <PropertyContainerClassPtr> or <PropertyContainerReference>.").arg(editObject()->metaObject()->className()).toLocal8Bit().constData());
		PropertyContainerReference selectedPropertyContainer = val.value<PropertyContainerReference>();	
		
		// Obtain modifier input data.
		std::vector<PipelineFlowState> modifierInputs;
		if(Modifier* mod = dynamic_object_cast<Modifier>(editObject())) {
			for(ModifierApplication* modApp : mod->modifierApplications()) {
				modifierInputs.push_back(modApp->evaluateInputPreliminary());
			}
		}

		// Update state of the property containers depending on their presence in the input pipeline data.
		const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(comboBox()->model());
		int enabledCount = 0;
		int selectedIndex = -1;
		for(int i = 0; i < comboBox()->count(); i++) {
			QStandardItem* item = model->item(i);
			PropertyContainerReference containerRef = item->data(Qt::UserRole).value<PropertyContainerReference>();
			if(containerRef == selectedPropertyContainer) 
				selectedIndex = i;

			item->setEnabled(std::any_of(modifierInputs.begin(), modifierInputs.end(), [&containerRef](const PipelineFlowState& state) {
				return state.getLeafObject(containerRef) != nullptr;
			}));
		}

		comboBox()->setCurrentIndex(selectedIndex);
	}
}

/******************************************************************************
* Takes the value entered by the user and stores it in the property field 
* this property UI is bound to.
******************************************************************************/
void PropertyContainerParameterUI::updatePropertyValue()
{
	if(comboBox() && editObject()) {
		undoableTransaction(tr("Change modifier subject"), [this]() {

			PropertyContainerReference containerRef = comboBox()->currentData().value<PropertyContainerReference>();

			// Check if new value differs from old value.
			QVariant oldval = editObject()->getPropertyFieldValue(*propertyField());
			if(containerRef == oldval.value<PropertyContainerReference>())
				return;

			editObject()->setPropertyFieldValue(*propertyField(), QVariant::fromValue(containerRef));
			
			Q_EMIT valueEntered();
		});
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void PropertyContainerParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	PropertyParameterUI::setEnabled(enabled);
	if(comboBox()) comboBox()->setEnabled(editObject() && isEnabled());
}

}	// End of namespace
}	// End of namespace
