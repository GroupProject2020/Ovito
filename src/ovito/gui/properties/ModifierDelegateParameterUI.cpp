////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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

#include <ovito/gui/GUI.h>
#include <ovito/gui/properties/PropertiesEditor.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/pipeline/DelegatingModifier.h>
#include <ovito/core/dataset/pipeline/AsynchronousDelegatingModifier.h>
#include <ovito/core/app/PluginManager.h>
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
	DataObjectReference inputDataObject;
	if(DelegatingModifier* delegatingMod = dynamic_object_cast<DelegatingModifier>(mod)) {
		delegate = delegatingMod->delegate();
		if(delegate)
			inputDataObject = delegatingMod->delegate()->inputDataObject();
	}
	else if(AsynchronousDelegatingModifier* delegatingMod = dynamic_object_cast<AsynchronousDelegatingModifier>(mod)) {
		delegate = delegatingMod->delegate();
		if(delegate)
			inputDataObject = delegatingMod->delegate()->inputDataObject();
	}
	else if(mod) {
		OVITO_ASSERT(false);
	}

	OVITO_ASSERT(!delegate || _delegateType.isMember(delegate));

	if(comboBox() && mod) {
		comboBox()->clear();

		// Obtain modifier inputs.
		std::vector<OORef<DataCollection>> modifierInputs;
		for(ModifierApplication* modApp : mod->modifierApplications()) {
			const PipelineFlowState& state = modApp->evaluateInputPreliminary();
			if(state.data())
				modifierInputs.push_back(state.data());
		}

		// Add list items for the registered delegate classes.
		int indexToBeSelected = -1;
		const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(comboBox()->model());
		for(const OvitoClassPtr& clazz : PluginManager::instance().listClasses(_delegateType)) {

			// Collect the set of data objects in the modifier's pipeline input this delegate can handle.
			QVector<DataObjectReference> applicableObjects;
			for(const DataCollection* data : modifierInputs) {

				// Query the delegate for the list of input data objects it can handle.
				QVector<DataObjectReference> objList;
				if(clazz->isDerivedFrom(ModifierDelegate::OOClass()))
					objList = static_cast<const ModifierDelegate::OOMetaClass*>(clazz)->getApplicableObjects(*data);
				else if(clazz->isDerivedFrom(AsynchronousModifierDelegate::OOClass()))
					objList = static_cast<const AsynchronousModifierDelegate::OOMetaClass*>(clazz)->getApplicableObjects(*data);

				// Combine the delegate's list with the existing list.
				// Make sure no data object appears more than once.
				if(applicableObjects.empty()) {
					applicableObjects = std::move(objList);
				}
				else {
					for(const DataObjectReference& ref : objList) {
						if(!applicableObjects.contains(ref))
							applicableObjects.push_back(ref);
					}
				}
			}

			if(!applicableObjects.empty()) {
				// Add an extra item to the list box for every data object that the delegate can handle.
				for(const DataObjectReference& ref : applicableObjects) {
					comboBox()->addItem(ref.dataTitle().isEmpty() ? clazz->displayName() : ref.dataTitle(), QVariant::fromValue(clazz));
					comboBox()->setItemData(comboBox()->count() - 1, QVariant::fromValue(ref), Qt::UserRole + 1);
					if(&delegate->getOOClass() == clazz && (inputDataObject == ref || !inputDataObject)) {
						indexToBeSelected = comboBox()->count() - 1;
					}
				}
			}
			else {
				// Even if this delegate cannot handle the input data, still show it in the list box as a disabled item.
				comboBox()->addItem(clazz->displayName(), QVariant::fromValue(clazz));
				if(&delegate->getOOClass() == clazz)
					indexToBeSelected = comboBox()->count() - 1;
				model->item(comboBox()->count() - 1)->setEnabled(false);
			}
		}

		// Select the right item in the list box.
		static QIcon warningIcon(QStringLiteral(":/gui/mainwin/status/status_warning.png"));
		if(delegate) {
			if(indexToBeSelected < 0) {
				if(delegate && inputDataObject) {
					// Add a place-holder item if the selected data object does not exist anymore.
					QString title = inputDataObject.dataTitle();
					if(title.isEmpty() && inputDataObject.dataClass())
						title = inputDataObject.dataClass()->displayName();
					title += tr(" (not available)");
					comboBox()->addItem(title, QVariant::fromValue(QVariant::fromValue(&delegate->getOOClass())));
					QStandardItem* item = static_cast<QStandardItemModel*>(comboBox()->model())->item(comboBox()->count()-1);
					item->setIcon(warningIcon);
				}
				else if(comboBox()->count() != 0) {
					comboBox()->addItem(tr("<Please select a data object>"));
				}
				indexToBeSelected = comboBox()->count() - 1;
			}
			if(comboBox()->count() == 0) {
				comboBox()->addItem(tr("<No inputs available>"));
				QStandardItem* item = static_cast<QStandardItemModel*>(comboBox()->model())->item(0);
				item->setIcon(warningIcon);
				indexToBeSelected = 0;
			}
		}
		else {
			comboBox()->addItem(tr("<None>"));
			indexToBeSelected = comboBox()->count() - 1;
			QStandardItem* item = static_cast<QStandardItemModel*>(comboBox()->model())->item(indexToBeSelected);
			item->setIcon(warningIcon);
		}
		comboBox()->setCurrentIndex(indexToBeSelected);
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
				DataObjectReference ref = comboBox()->currentData(Qt::UserRole + 1).value<DataObjectReference>();
				if(DelegatingModifier* delegatingMod = dynamic_object_cast<DelegatingModifier>(mod)) {
					if(delegatingMod->delegate() == nullptr || &delegatingMod->delegate()->getOOClass() != delegateType || delegatingMod->delegate()->inputDataObject() != ref) {
						// Create the new delegate object.
						OORef<ModifierDelegate> delegate = static_object_cast<ModifierDelegate>(delegateType->createInstance(mod->dataset()));
						// Set which input data object the delegate should operate on.
						delegate->setInputDataObject(ref);
						// Activate the new delegate.
						delegatingMod->setDelegate(std::move(delegate));
					}
				}
				else if(AsynchronousDelegatingModifier* delegatingMod = dynamic_object_cast<AsynchronousDelegatingModifier>(mod)) {
					if(delegatingMod->delegate() == nullptr || &delegatingMod->delegate()->getOOClass() != delegateType || delegatingMod->delegate()->inputDataObject() != ref) {
						// Create the new delegate object.
						OORef<AsynchronousModifierDelegate> delegate = static_object_cast<AsynchronousModifierDelegate>(delegateType->createInstance(mod->dataset()));
						// Set which input data object the delegate should operate on.
						delegate->setInputDataObject(ref);
						// Activate the new delegate.
						delegatingMod->setDelegate(std::move(delegate));
					}
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
