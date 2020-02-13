////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Alexander Stukowski
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
#include <ovito/gui/desktop/properties/CustomParameterUI.h>
#include <ovito/core/dataset/UndoStack.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(CustomParameterUI);

/******************************************************************************
* Constructor for a Qt property.
******************************************************************************/
CustomParameterUI::CustomParameterUI(QObject* parentEditor, const char* propertyName, QWidget* widget,
		const std::function<void(const QVariant&)>& updateWidgetFunction,
		const std::function<QVariant()>& updatePropertyFunction,
		const std::function<void(RefTarget*)>& resetUIFunction) :
	PropertyParameterUI(parentEditor, propertyName), _widget(widget), _updateWidgetFunction(updateWidgetFunction), _updatePropertyFunction(updatePropertyFunction), _resetUIFunction(resetUIFunction)
{
}

/******************************************************************************
* Constructor for a PropertyField property.
******************************************************************************/
CustomParameterUI::CustomParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField, QWidget* widget,
		const std::function<void(const QVariant&)>& updateWidgetFunction,
		const std::function<QVariant()>& updatePropertyFunction,
		const std::function<void(RefTarget*)>& resetUIFunction) :
	PropertyParameterUI(parentEditor, propField), _widget(widget), _updateWidgetFunction(updateWidgetFunction), _updatePropertyFunction(updatePropertyFunction), _resetUIFunction(resetUIFunction)
{
}

/******************************************************************************
* Destructor.
******************************************************************************/
CustomParameterUI::~CustomParameterUI()
{
	// Release widget.
	delete widget();
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void CustomParameterUI::resetUI()
{
	if(widget()) {
		widget()->setEnabled(editObject() != NULL && isEnabled());
		if(_resetUIFunction)
			_resetUIFunction(editObject());
	}

	PropertyParameterUI::resetUI();
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void CustomParameterUI::updateUI()
{
	PropertyParameterUI::updateUI();

	if(widget() && editObject()) {
		QVariant val;
		if(isQtPropertyUI()) {
			val = editObject()->property(propertyName());
			if(!val.isValid())
				editObject()->throwException(tr("The object class %1 does not define a property with the name %2.").arg(editObject()->metaObject()->className(), QString(propertyName())));
		}
		else if(isPropertyFieldUI()) {
			val = editObject()->getPropertyFieldValue(*propertyField());
			OVITO_ASSERT_MSG(val.isValid(), "CustomParameterUI::updateUI()", QString("The object class %1 does not define a property with the name %2.").arg(editObject()->metaObject()->className(), QString(propertyName())).toLocal8Bit().constData());
		}
		else return;

		_updateWidgetFunction(val);
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void CustomParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	PropertyParameterUI::setEnabled(enabled);
	if(widget()) widget()->setEnabled(editObject() != NULL && isEnabled());
}

/******************************************************************************
* Takes the value entered by the user and stores it in the property field
* this property UI is bound to.
******************************************************************************/
void CustomParameterUI::updatePropertyValue()
{
	if(widget() && editObject()) {
		undoableTransaction(tr("Change parameter"), [this]() {
			QVariant newValue = _updatePropertyFunction();

            if(isQtPropertyUI()) {
                if(!editObject()->setProperty(propertyName(), newValue)) {
                    OVITO_ASSERT_MSG(false, "CustomParameterUI::updatePropertyValue()", QString("The value of property %1 of object class %2 could not be set.").arg(QString(propertyName()), editObject()->metaObject()->className()).toLocal8Bit().constData());
                }
            }
            else if(isPropertyFieldUI()) {
                editObject()->setPropertyFieldValue(*propertyField(), newValue);
            }

			Q_EMIT valueEntered();
		});
	}
}

}	// End of namespace
