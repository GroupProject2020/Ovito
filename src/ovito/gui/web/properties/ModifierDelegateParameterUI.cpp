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

#include <ovito/gui/web/GUIWeb.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/pipeline/DelegatingModifier.h>
#include <ovito/core/dataset/pipeline/AsynchronousDelegatingModifier.h>
#include <ovito/core/app/PluginManager.h>
#include "ModifierDelegateParameterUI.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(ModifierDelegateParameterUI);

/******************************************************************************
* Sets the class of delegates the user can choose from.
******************************************************************************/
void ModifierDelegateParameterUI::setDelegateType(const QString& typeName)
{
	_delegateType = PluginManager::instance().findClass(QString(), typeName);
	if(!_delegateType) {
		qWarning() << "ModifierDelegateParameterUI: Delegate class" << typeName << "does not exist";
		return;
	}
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool ModifierDelegateParameterUI::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject() && event.type() == ReferenceEvent::ModifierInputChanged) {
		// The modifier's input from the pipeline has changed -> update list of available delegates
		Q_EMIT delegateListChanged();
	}
	else if(source == editObject() && event.type() == ReferenceEvent::ReferenceChanged && static_cast<const ReferenceFieldEvent&>(event).field() == propertyField()) {
		// The modifier has been assigned a new delegate -> update list of delegates and selected entry.
		Q_EMIT delegateListChanged();
		updateUI();
	}
	return ParameterUI::referenceEvent(source, event);
}

/******************************************************************************
* Returns the list of available delegate types.
******************************************************************************/
QStringList ModifierDelegateParameterUI::delegateList()
{
	_delegateList.clear();
	Modifier* mod = dynamic_object_cast<Modifier>(editObject());
	if(!mod || !_delegateType) return {};

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
	else {
		OVITO_ASSERT(false);
	}
	OVITO_ASSERT(!delegate || _delegateType->isMember(delegate));

	QStringList itemList;
	int indexToBeSelected = -1;

	// Obtain modifier inputs.
	std::vector<OORef<DataCollection>> modifierInputs;
	for(ModifierApplication* modApp : mod->modifierApplications()) {
		const PipelineFlowState& state = modApp->evaluateInputSynchronous(editObject()->dataset()->animationSettings()->time());
		if(state.data())
			modifierInputs.push_back(state.data());
	}

	// Add list items for the registered delegate classes.
	for(const OvitoClassPtr& clazz : PluginManager::instance().listClasses(*_delegateType)) {

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
				itemList.push_back(ref.dataTitle().isEmpty() ? clazz->displayName() : ref.dataTitle());
				_delegateList.push_back({clazz, ref});
				if(delegate && &delegate->getOOClass() == clazz && (inputDataObject == ref || !inputDataObject)) {
					indexToBeSelected = itemList.size() - 1;
				}
			}
		}
		else {
			// Even if this delegate cannot handle the input data, still show it in the list box as a disabled item.
//			comboBox()->addItem(clazz->displayName(), QVariant::fromValue(clazz));
//			itemList.push_back(clazz->displayName());
//			_delegateList.push_back(clazz, {});
//			if(delegate && &delegate->getOOClass() == clazz)
//				indexToBeSelected = comboBox()->count() - 1;
//			model->item(comboBox()->count() - 1)->setEnabled(false);
		}
	}

	// Select the right item in the list box.
	if(delegate) {
		if(indexToBeSelected < 0) {
			if(delegate && inputDataObject) {
				// Add a placeholder item if the selected data object does not exist anymore.
				QString title = inputDataObject.dataTitle();
				if(title.isEmpty() && inputDataObject.dataClass())
					title = inputDataObject.dataClass()->displayName();
				title += tr(" (not available)");
				itemList.push_back(title);
				_delegateList.push_back({&delegate->getOOClass(), {}});
			}
			else if(!itemList.empty()) {
				itemList.push_back(tr("<Please select a data object>"));
				_delegateList.push_back({nullptr, {}});
			}
		}
		if(itemList.empty()) {
			itemList.push_back(tr("<No inputs available>"));
			_delegateList.push_back({nullptr, {}});
		}
	}
	else {
		itemList.push_back(tr("<None>"));
		_delegateList.push_back({nullptr, {}});
	}

	return itemList;
}

/******************************************************************************
* Obtains the current value of the parameter from the C++ object.
******************************************************************************/
QVariant ModifierDelegateParameterUI::getCurrentValue() const
{
	Modifier* mod = dynamic_object_cast<Modifier>(editObject());
	if(!mod || !_delegateType) return -1;

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
	else {
		OVITO_ASSERT(false);
	}
	OVITO_ASSERT(!delegate || _delegateType->isMember(delegate));

	// Update the list of available delegates.
	if(_delegateList.empty()) {
		const_cast<ModifierDelegateParameterUI*>(this)->delegateList();
	}

	int index = 0;
	for(const auto& item : _delegateList) {
		if(delegate && &delegate->getOOClass() == item.first && (inputDataObject == item.second || !inputDataObject)) {
			return index;
		}
		index++;
	}

	return -1;
}

/******************************************************************************
* Changes the current value of the C++ object parameter.
******************************************************************************/
void ModifierDelegateParameterUI::setCurrentValue(const QVariant& val)
{
	if(Modifier* mod = dynamic_object_cast<Modifier>(editObject())) {
		int index = val.toInt();
		if(index >= 0 && index < _delegateList.size()) {
			UndoableTransaction::handleExceptions(editObject()->dataset()->undoStack(), tr("Change input type"), [this,mod,index]() {
				if(OvitoClassPtr delegateType = _delegateList[index].first) {
					const DataObjectReference& ref = _delegateList[index].second;
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
			});
		}
	}
}

}	// End of namespace
