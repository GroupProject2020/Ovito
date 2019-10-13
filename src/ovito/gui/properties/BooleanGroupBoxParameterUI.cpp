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

#include <ovito/gui/GUI.h>
#include <ovito/gui/properties/BooleanGroupBoxParameterUI.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

IMPLEMENT_OVITO_CLASS(BooleanGroupBoxParameterUI);

/******************************************************************************
* Constructor for a Qt property.
******************************************************************************/
BooleanGroupBoxParameterUI::BooleanGroupBoxParameterUI(QObject* parentEditor, const char* propertyName, const QString& label) :
	PropertyParameterUI(parentEditor, propertyName)
{
	// Create UI widget.
	_groupBox = new QGroupBox(label);
	_groupBox->setCheckable(true);
	_childContainer = new QWidget(_groupBox);
	QVBoxLayout* layout = new QVBoxLayout(_groupBox);
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(0);
	layout->addWidget(_childContainer, 1);
	connect(_groupBox.data(), &QGroupBox::clicked, this, &BooleanGroupBoxParameterUI::updatePropertyValue);
}

/******************************************************************************
* Constructor for a PropertyField property.
******************************************************************************/
BooleanGroupBoxParameterUI::BooleanGroupBoxParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField) :
	PropertyParameterUI(parentEditor, propField)
{
	// Create UI widget.
	_groupBox = new QGroupBox(propField.displayName());
	_groupBox->setCheckable(true);
	_childContainer = new QWidget(_groupBox);
	QVBoxLayout* layout = new QVBoxLayout(_groupBox);
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(0);
	layout->addWidget(_childContainer, 1);
	connect(_groupBox.data(), &QGroupBox::clicked, this, &BooleanGroupBoxParameterUI::updatePropertyValue);
}

/******************************************************************************
* Destructor.
******************************************************************************/
BooleanGroupBoxParameterUI::~BooleanGroupBoxParameterUI()
{
	// Release GUI controls.
	delete groupBox();
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void BooleanGroupBoxParameterUI::resetUI()
{
	PropertyParameterUI::resetUI();

	if(groupBox()) {
		if(isReferenceFieldUI())
			groupBox()->setEnabled(parameterObject() != NULL && isEnabled());
		else
			groupBox()->setEnabled(editObject() != NULL && isEnabled());
	}
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void BooleanGroupBoxParameterUI::updateUI()
{
	PropertyParameterUI::updateUI();

	if(groupBox() && editObject()) {
		if(!isReferenceFieldUI()) {
			QVariant val(false);
			if(isQtPropertyUI()) {
				val = editObject()->property(propertyName());
				OVITO_ASSERT_MSG(val.isValid(), "BooleanGroupBoxParameterUI::updateUI()", QString("The object class %1 does not define a property with the name %2 that can be cast to bool type.").arg(editObject()->metaObject()->className(), QString(propertyName())).toLocal8Bit().constData());
				if(!val.isValid()) {
					editObject()->throwException(tr("The object class %1 does not define a property with the name %2 that can be cast to bool type.").arg(editObject()->metaObject()->className(), QString(propertyName())));
				}
			}
			else if(isPropertyFieldUI()) {
				val = editObject()->getPropertyFieldValue(*propertyField());
				OVITO_ASSERT(val.isValid());
			}
			groupBox()->setChecked(val.toBool());
		}
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void BooleanGroupBoxParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	PropertyParameterUI::setEnabled(enabled);
	if(groupBox()) {
		if(isReferenceFieldUI())
			groupBox()->setEnabled(parameterObject() != NULL && isEnabled());
		else
			groupBox()->setEnabled(editObject() != NULL && isEnabled());
	}
}

/******************************************************************************
* Takes the value entered by the user and stores it in the property field
* this property UI is bound to.
******************************************************************************/
void BooleanGroupBoxParameterUI::updatePropertyValue()
{
	if(groupBox() && editObject()) {
		undoableTransaction(tr("Change parameter"), [this]() {
			if(isQtPropertyUI()) {
				if(!editObject()->setProperty(propertyName(), groupBox()->isChecked())) {
					OVITO_ASSERT_MSG(false, "BooleanGroupBoxParameterUI::updatePropertyValue()", QString("The value of property %1 of object class %2 could not be set.").arg(QString(propertyName()), editObject()->metaObject()->className()).toLocal8Bit().constData());
				}
			}
			else if(isPropertyFieldUI()) {
				editObject()->setPropertyFieldValue(*propertyField(), groupBox()->isChecked());
			}
			Q_EMIT valueEntered();
		});
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
