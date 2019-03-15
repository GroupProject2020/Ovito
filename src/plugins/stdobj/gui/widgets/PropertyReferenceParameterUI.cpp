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
#include <core/dataset/UndoStack.h>
#include <core/dataset/pipeline/Modifier.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/animation/AnimationSettings.h>
#include "PropertyReferenceParameterUI.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(PropertyReferenceParameterUI);

/******************************************************************************
* Constructor.
******************************************************************************/
PropertyReferenceParameterUI::PropertyReferenceParameterUI(QObject* parentEditor, const char* propertyName, PropertyContainerClassPtr containerClass, bool showComponents, bool inputProperty) :
	PropertyParameterUI(parentEditor, propertyName),
	_comboBox(new PropertySelectionComboBox(containerClass)),
	_showComponents(showComponents),
	_inputProperty(inputProperty),
	_containerRef(containerClass)
{
	connect(comboBox(), static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), this, &PropertyReferenceParameterUI::updatePropertyValue);

	if(!inputProperty)
		comboBox()->setEditable(true);
}

/******************************************************************************
* Constructor.
******************************************************************************/
PropertyReferenceParameterUI::PropertyReferenceParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField, PropertyContainerClassPtr containerClass, bool showComponents, bool inputProperty) :
	PropertyParameterUI(parentEditor, propField),
	_comboBox(new PropertySelectionComboBox(containerClass)),
	_showComponents(showComponents),
	_inputProperty(inputProperty),
	_containerRef(containerClass)
{
	connect(comboBox(), static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), this, &PropertyReferenceParameterUI::updatePropertyValue);

	if(!inputProperty)
		comboBox()->setEditable(true);
}

/******************************************************************************
* Destructor.
******************************************************************************/
PropertyReferenceParameterUI::~PropertyReferenceParameterUI()
{
	delete comboBox();
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void PropertyReferenceParameterUI::resetUI()
{
	PropertyParameterUI::resetUI();

	if(comboBox())
		comboBox()->setEnabled(editObject() && isEnabled());
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool PropertyReferenceParameterUI::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject() && event.type() == ReferenceEvent::ModifierInputChanged) {
		// The modifier's input from the pipeline has changed -> update value shown in UI.
		updateUI();
	}
	return PropertyParameterUI::referenceEvent(source, event);
}

/******************************************************************************
* Returns the value currently set for the property field.
******************************************************************************/
PropertyReference PropertyReferenceParameterUI::getPropertyReference()
{
	if(editObject()) {
		if(isQtPropertyUI()) {
			QVariant val = editObject()->property(propertyName());
			OVITO_ASSERT_MSG(val.isValid() && val.canConvert<PropertyReference>(), "PropertyReferenceParameterUI::updateUI()", QString("The object class %1 does not define a property with the name %2 of type PropertyReference.").arg(editObject()->metaObject()->className(), QString(propertyName())).toLocal8Bit().constData());
			if(!val.isValid() || !val.canConvert<PropertyReference>()) {
				editObject()->throwException(tr("The object class %1 does not define a property with the name %2 that can be cast to a PropertyReference.").arg(editObject()->metaObject()->className(), QString(propertyName())));
			}
			return val.value<PropertyReference>();
		}
		else if(isPropertyFieldUI()) {
			QVariant val = editObject()->getPropertyFieldValue(*propertyField());
			OVITO_ASSERT_MSG(val.isValid() && val.canConvert<PropertyReference>(), "PropertyReferenceParameterUI::updateUI()", QString("The property field of object class %1 is not of type PropertyReference.").arg(editObject()->metaObject()->className()).toLocal8Bit().constData());
			return val.value<PropertyReference>();
		}
	}
	return {};
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the
* properties owner this parameter UI belongs to.
******************************************************************************/
void PropertyReferenceParameterUI::updateUI()
{
	PropertyParameterUI::updateUI();

	if(comboBox() && editObject() && containerRef()) {

		PropertyReference pref = getPropertyReference();

		if(_inputProperty) {
			_comboBox->clear();

			// Obtain the list of input properties.
			if(Modifier* mod = dynamic_object_cast<Modifier>(editObject())) {
				for(ModifierApplication* modApp : mod->modifierApplications()) {
					// Populate combo box with items from the upstream pipeline.
					addItemsToComboBox(modApp->evaluateInputPreliminary());
				}
			}

			// Select the right item in the list box.
			int selIndex = _comboBox->propertyIndex(pref);
			static QIcon warningIcon(QStringLiteral(":/gui/mainwin/status/status_warning.png"));
			if(selIndex < 0) {
				if(!pref.isNull() && pref.containerClass() == containerRef().dataClass()) {
					// Add a place-holder item if the selected property does not exist anymore.
					_comboBox->addItem(pref, tr("%1 (not available)").arg(pref.name()));
					QStandardItem* item = static_cast<QStandardItemModel*>(_comboBox->model())->item(_comboBox->count()-1);
					item->setIcon(warningIcon);
				}
				else if(_comboBox->count() != 0) {
					_comboBox->addItem({}, tr("<Please select a property>"));
				}
				selIndex = _comboBox->count() - 1;
			}
			if(_comboBox->count() == 0) {
				_comboBox->addItem(PropertyReference(), tr("<No available properties>"));
				QStandardItem* item = static_cast<QStandardItemModel*>(_comboBox->model())->item(0);
				item->setIcon(warningIcon);
				selIndex = 0;
			}
			_comboBox->setCurrentIndex(selIndex);
		}
		else {
			if(_comboBox->count() == 0) {
				for(int typeId : containerRef().dataClass()->standardPropertyIds())
					_comboBox->addItem(PropertyReference(containerRef().dataClass(), typeId));
			}
			_comboBox->setCurrentProperty(pref);
		}
	}
	else if(comboBox())
		comboBox()->clear();
}

/******************************************************************************
* Populates the combox box with items.
******************************************************************************/
void PropertyReferenceParameterUI::addItemsToComboBox(const PipelineFlowState& state)
{
	OVITO_ASSERT(containerRef());
	if(const PropertyContainer* container = !state.isEmpty() ? state.getLeafObject(containerRef()) : nullptr) {
		for(const PropertyObject* property : container->properties()) {

			// The client can apply a filter to the displayed property list.
			if(_propertyFilter && !_propertyFilter(property))
				continue;

			// Properties with a non-numeric data type cannot be used as source properties.
			if(property->dataType() != PropertyStorage::Int && property->dataType() != PropertyStorage::Int64 && property->dataType() != PropertyStorage::Float)
				continue;

			if(property->componentNames().empty() || !_showComponents) {
				// Scalar property:
				_comboBox->addItem(property);
			}
			else {
				// Vector property:
				for(int vectorComponent = 0; vectorComponent < (int)property->componentCount(); vectorComponent++) {
					_comboBox->addItem(property, vectorComponent);
				}
			}
		}
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void PropertyReferenceParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	PropertyParameterUI::setEnabled(enabled);
	if(comboBox()) comboBox()->setEnabled(editObject() != NULL && isEnabled());
}

/******************************************************************************
* Takes the value entered by the user and stores it in the property field
* this property UI is bound to.
******************************************************************************/
void PropertyReferenceParameterUI::updatePropertyValue()
{
	if(comboBox() && editObject() && comboBox()->currentText().isEmpty() == false) {
		undoableTransaction(tr("Change parameter"), [this]() {
			PropertyReference pref = _comboBox->currentProperty();
			if(isQtPropertyUI()) {

				// Check if new value differs from old value.
				QVariant oldval = editObject()->property(propertyName());
				if(pref == oldval.value<PropertyReference>())
					return;

				if(!editObject()->setProperty(propertyName(), QVariant::fromValue(pref))) {
					OVITO_ASSERT_MSG(false, "PropertyReferenceParameterUI::updatePropertyValue()", QString("The value of property %1 of object class %2 could not be set.").arg(QString(propertyName()), editObject()->metaObject()->className()).toLocal8Bit().constData());
				}
			}
			else if(isPropertyFieldUI()) {

				// Check if new value differs from old value.
				QVariant oldval = editObject()->getPropertyFieldValue(*propertyField());
				if(pref == oldval.value<PropertyReference>())
					return;

				editObject()->setPropertyFieldValue(*propertyField(), QVariant::fromValue(pref));
			}
			else return;

			Q_EMIT valueEntered();
		});
	}
}

}	// End of namespace
}	// End of namespace
