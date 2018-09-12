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

#include <gui/GUI.h>
#include <gui/properties/PropertiesEditor.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/pipeline/DelegatingModifier.h>
#include <core/dataset/pipeline/AsynchronousDelegatingModifier.h>
#include <core/app/PluginManager.h>
#include "ModifierDelegateParameterUI.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

IMPLEMENT_OVITO_CLASS(ModifierDelegateParameterUI);

/******************************************************************************
* Constructor.
******************************************************************************/
ModifierDelegateParameterUI::ModifierDelegateParameterUI(QObject* parent, const OvitoClass& delegateType) : 
	ParameterUI(parent),
	_comboBox(new QComboBox()),
	_delegateType(delegateType)
{
	connect(comboBox(), static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), this, &ModifierDelegateParameterUI::updatePropertyValue);
}


/******************************************************************************
* Destructor.
******************************************************************************/
ModifierDelegateParameterUI::~ModifierDelegateParameterUI()
{
	delete comboBox(); 
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to. 
******************************************************************************/
void ModifierDelegateParameterUI::resetUI()
{
	ParameterUI::resetUI();	
	
	if(comboBox()) 
		comboBox()->setEnabled(editObject() && isEnabled());
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool ModifierDelegateParameterUI::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject() && event.type() == ReferenceEvent::ModifierInputChanged) {
		// The modifier's input from the pipeline has changed -> update list of available delegates
		updateUI();
	}
	else if(source == editObject() && event.type() == ReferenceEvent::ReferenceChanged && 
			(static_cast<const ReferenceFieldEvent&>(event).field() == &PROPERTY_FIELD(DelegatingModifier::delegate) ||
			static_cast<const ReferenceFieldEvent&>(event).field() == &PROPERTY_FIELD(AsynchronousDelegatingModifier::delegate))) {
		// The modifier has been assigned a new delegate -> update list of delegates
		updateUI();
	}
	return ParameterUI::referenceEvent(source, event);
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the 
* properties owner this parameter UI belongs to. 
******************************************************************************/
void ModifierDelegateParameterUI::updateUI()
{
	ParameterUI::updateUI();
	
	Modifier* mod = dynamic_object_cast<Modifier>(editObject());
	RefTarget* delegate = nullptr;
	if(DelegatingModifier* delegatingMod = dynamic_object_cast<DelegatingModifier>(mod))
		delegate = delegatingMod->delegate();
	else if(AsynchronousDelegatingModifier* delegatingMod = dynamic_object_cast<AsynchronousDelegatingModifier>(mod))
		delegate = delegatingMod->delegate();
	else if(mod)
		OVITO_ASSERT(false);

	OVITO_ASSERT(!delegate || _delegateType.isMember(delegate));

	if(comboBox() && mod) {
		comboBox()->clear();

		// Obtain modifier inputs.
		std::vector<OORef<DataCollection>> modifierInputs;
		for(ModifierApplication* modApp : mod->modifierApplications()) {
			modifierInputs.push_back(modApp->evaluateInputPreliminary().data());
		}
	
		// Add list items for the registered delegate classes.
		const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(comboBox()->model());
		for(const OvitoClassPtr& clazz : PluginManager::instance().listClasses(_delegateType)) {
			comboBox()->addItem(clazz->displayName(), QVariant::fromValue(clazz));

			// Check if this delegate can handle the current modifier input data.
			// If not, disable the list entry for this delegate.
			if(clazz->isDerivedFrom(ModifierDelegate::OOClass())) {
				if(std::any_of(modifierInputs.begin(), modifierInputs.end(), [clazz = static_cast<const ModifierDelegate::OOMetaClass*>(clazz)](const DataCollection* data) { return data && clazz->isApplicableTo(*data); }))
					continue;
			}
			else if(clazz->isDerivedFrom(AsynchronousModifierDelegate::OOClass())) {
				if(std::any_of(modifierInputs.begin(), modifierInputs.end(), [clazz = static_cast<const AsynchronousModifierDelegate::OOMetaClass*>(clazz)](const DataCollection* data) { return data && clazz->isApplicableTo(*data); }))
					continue;
			}

			model->item(comboBox()->count() - 1)->setEnabled(false);
		}

		if(comboBox()->count() == 0)
			comboBox()->addItem(tr("<No input types available>"));

		// Select the right item in the list box.
		if(delegate) {
			int selIndex = comboBox()->findData(QVariant::fromValue(&delegate->getOOClass()));
			comboBox()->setCurrentIndex(selIndex);
		}
		else {
			comboBox()->addItem(tr("<none>"));
			comboBox()->setCurrentIndex(comboBox()->count() - 1);
		}
	}
	else if(comboBox()) {
		comboBox()->clear();
	}
}

/******************************************************************************
* Takes the value entered by the user and stores it in the property field 
* this property UI is bound to.
******************************************************************************/
void ModifierDelegateParameterUI::updatePropertyValue()
{
	Modifier* mod = dynamic_object_cast<Modifier>(editObject());
	if(comboBox() && mod) {
		undoableTransaction(tr("Change input type"), [this,mod]() {
			if(OvitoClassPtr delegateType = comboBox()->currentData().value<OvitoClassPtr>()) {
				if(DelegatingModifier* delegatingMod = dynamic_object_cast<DelegatingModifier>(mod)) {
					if(delegatingMod->delegate() == nullptr || &delegatingMod->delegate()->getOOClass() != delegateType)
						delegatingMod->setDelegate(static_object_cast<ModifierDelegate>(delegateType->createInstance(mod->dataset())));
				}
				else if(AsynchronousDelegatingModifier* delegatingMod = dynamic_object_cast<AsynchronousDelegatingModifier>(mod)) {
					if(delegatingMod->delegate() == nullptr || &delegatingMod->delegate()->getOOClass() != delegateType)
						delegatingMod->setDelegate(static_object_cast<AsynchronousModifierDelegate>(delegateType->createInstance(mod->dataset())));
				}
			}
			Q_EMIT valueEntered();
		});
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void ModifierDelegateParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	ParameterUI::setEnabled(enabled);
	if(comboBox()) comboBox()->setEnabled(editObject() != NULL && isEnabled());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
