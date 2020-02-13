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
#include <ovito/gui/desktop/properties/BooleanActionParameterUI.h>
#include <ovito/core/dataset/UndoStack.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(BooleanActionParameterUI);

/******************************************************************************
* Constructor for a Qt property.
******************************************************************************/
BooleanActionParameterUI::BooleanActionParameterUI(QObject* parentEditor, const char* propertyName, QAction* action) :
	PropertyParameterUI(parentEditor, propertyName), _action(action)
{
	OVITO_ASSERT(action != nullptr);
	action->setCheckable(true);
	connect(action, &QAction::triggered, this, &BooleanActionParameterUI::updatePropertyValue);
}

/******************************************************************************
* Constructor for a PropertyField property.
******************************************************************************/
BooleanActionParameterUI::BooleanActionParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField, QAction* action) :
	PropertyParameterUI(parentEditor, propField), _action(action)
{
	OVITO_ASSERT(isPropertyFieldUI());
	OVITO_ASSERT(action != nullptr);
	action->setCheckable(true);
	connect(action, &QAction::triggered, this, &BooleanActionParameterUI::updatePropertyValue);
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void BooleanActionParameterUI::resetUI()
{
	PropertyParameterUI::resetUI();

	if(action())
		action()->setEnabled(editObject() != NULL && isEnabled());
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void BooleanActionParameterUI::updateUI()
{
	PropertyParameterUI::updateUI();

	if(action() && editObject()) {
		QVariant val(false);
		if(isQtPropertyUI()) {
			val = editObject()->property(propertyName());
			OVITO_ASSERT_MSG(val.isValid(), "BooleanActionParameterUI::updateUI()", QString("The object class %1 does not define a property with the name %2 that can be cast to bool type.").arg(editObject()->metaObject()->className(), QString(propertyName())).toLocal8Bit().constData());
			if(!val.isValid()) {
				editObject()->throwException(tr("The object class %1 does not define a property with the name %2 that can be cast to bool type.").arg(editObject()->metaObject()->className(), QString(propertyName())));
			}
		}
		else if(isPropertyFieldUI()) {
			val = editObject()->getPropertyFieldValue(*propertyField());
			OVITO_ASSERT(val.isValid());
		}
		action()->setChecked(val.toBool());
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void BooleanActionParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	PropertyParameterUI::setEnabled(enabled);
	if(action()) action()->setEnabled(editObject() != NULL && isEnabled());
}

/******************************************************************************
* Takes the value entered by the user and stores it in the property field
* this property UI is bound to.
******************************************************************************/
void BooleanActionParameterUI::updatePropertyValue()
{
	if(action() && editObject()) {
		undoableTransaction(tr("Change parameter"), [this]() {
			if(isQtPropertyUI()) {
				if(!editObject()->setProperty(propertyName(), action()->isChecked())) {
					OVITO_ASSERT_MSG(false, "BooleanActionParameterUI::updatePropertyValue()", QString("The value of property %1 of object class %2 could not be set.").arg(QString(propertyName()), editObject()->metaObject()->className()).toLocal8Bit().constData());
				}
			}
			else if(isPropertyFieldUI()) {
				editObject()->setPropertyFieldValue(*propertyField(), action()->isChecked());
			}
			Q_EMIT valueEntered();
		});
	}
}

}	// End of namespace
