////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/animation/controller/Controller.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(BooleanParameterUI);

/******************************************************************************
* Constructor for a Qt property.
******************************************************************************/
BooleanParameterUI::BooleanParameterUI(QObject* parentEditor, const char* propertyName, const QString& checkBoxLabel) :
	PropertyParameterUI(parentEditor, propertyName)
{
	// Create UI widget.
	_checkBox = new QCheckBox(checkBoxLabel);
	connect(_checkBox.data(), &QCheckBox::clicked, this, &BooleanParameterUI::updatePropertyValue);
}

/******************************************************************************
* Constructor for a PropertyField property.
******************************************************************************/
BooleanParameterUI::BooleanParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField) :
	PropertyParameterUI(parentEditor, propField)
{
	// Create UI widget.
	_checkBox = new QCheckBox(propField.displayName());
	connect(_checkBox.data(), &QCheckBox::clicked, this, &BooleanParameterUI::updatePropertyValue);
}

/******************************************************************************
* Destructor.
******************************************************************************/
BooleanParameterUI::~BooleanParameterUI()
{
	// Release widget.
	delete checkBox();
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void BooleanParameterUI::resetUI()
{
	PropertyParameterUI::resetUI();

	if(checkBox()) {
		if(isReferenceFieldUI())
			checkBox()->setEnabled(parameterObject() != NULL && isEnabled());
		else
			checkBox()->setEnabled(editObject() != NULL && isEnabled());
	}

	if(isReferenceFieldUI() && editObject()) {
		// Update the displayed value when the animation time has changed.
		connect(dataset()->container(), &DataSetContainer::timeChanged, this, &BooleanParameterUI::updateUI, Qt::UniqueConnection);
	}
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void BooleanParameterUI::updateUI()
{
	PropertyParameterUI::updateUI();

	if(checkBox() && editObject()) {
		if(!isReferenceFieldUI()) {
			QVariant val(false);
			if(propertyName()) {
				val = editObject()->property(propertyName());
				OVITO_ASSERT_MSG(val.isValid(), "BooleanParameterUI::updateUI()", QString("The object class %1 does not define a property with the name %2 that can be cast to bool type.").arg(editObject()->metaObject()->className(), QString(propertyName())).toLocal8Bit().constData());
				if(!val.isValid()) {
					editObject()->throwException(tr("The object class %1 does not define a property with the name %2 that can be cast to bool type.").arg(editObject()->metaObject()->className(), QString(propertyName())));
				}
			}
			else if(propertyField()) {
				val = editObject()->getPropertyFieldValue(*propertyField());
				OVITO_ASSERT(val.isValid());
			}
			checkBox()->setChecked(val.toBool());
		}
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void BooleanParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	PropertyParameterUI::setEnabled(enabled);
	if(checkBox()) {
		if(isReferenceFieldUI())
			checkBox()->setEnabled(parameterObject() != NULL && isEnabled());
		else
			checkBox()->setEnabled(editObject() != NULL && isEnabled());
	}
}

/******************************************************************************
* Takes the value entered by the user and stores it in the property field
* this property UI is bound to.
******************************************************************************/
void BooleanParameterUI::updatePropertyValue()
{
	if(checkBox() && editObject()) {
		undoableTransaction(tr("Change parameter"), [this]() {
			if(isQtPropertyUI()) {
				if(!editObject()->setProperty(propertyName(), checkBox()->isChecked())) {
					OVITO_ASSERT_MSG(false, "BooleanParameterUI::updatePropertyValue()", QString("The value of property %1 of object class %2 could not be set.").arg(QString(propertyName()), editObject()->metaObject()->className()).toLocal8Bit().constData());
				}
			}
			else if(isPropertyFieldUI()) {
				editObject()->setPropertyFieldValue(*propertyField(), checkBox()->isChecked());
			}
			Q_EMIT valueEntered();
		});
	}
}

}	// End of namespace
