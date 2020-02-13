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
#include <ovito/gui/desktop/properties/IntegerParameterUI.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/utilities/units/UnitsManager.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(IntegerParameterUI);

/******************************************************************************
* Constructor for a Qt property.
******************************************************************************/
IntegerParameterUI::IntegerParameterUI(QObject* parentEditor, const char* propertyName, const QString& labelText, const QMetaObject* parameterUnitType) :
	NumericalParameterUI(parentEditor, propertyName, parameterUnitType ? parameterUnitType : &IntegerParameterUnit::staticMetaObject, labelText)
{
}

/******************************************************************************
* Constructor for a PropertyField property.
******************************************************************************/
IntegerParameterUI::IntegerParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField) :
		NumericalParameterUI(parentEditor, propField, &IntegerParameterUnit::staticMetaObject)
{
}

/******************************************************************************
* Takes the value entered by the user and stores it in the property field
* this property UI is bound to.
******************************************************************************/
void IntegerParameterUI::updatePropertyValue()
{
	if(editObject() && spinner()) {
		if(isReferenceFieldUI()) {
			if(Controller* ctrl = dynamic_object_cast<Controller>(parameterObject()))
				ctrl->setCurrentIntValue(spinner()->intValue());
		}
		else if(isQtPropertyUI()) {
			if(!editObject()->setProperty(propertyName(), spinner()->intValue())) {
				OVITO_ASSERT_MSG(false, "IntegerParameterUI::updatePropertyValue()", QString("The value of property %1 of object class %2 could not be set.").arg(QString(propertyName()), editObject()->metaObject()->className()).toLocal8Bit().constData());
			}
		}
		else if(isPropertyFieldUI()) {
			editObject()->setPropertyFieldValue(*propertyField(), spinner()->intValue());
		}
		Q_EMIT valueEntered();
	}
}

/******************************************************************************
* This method updates the displayed value of the parameter UI.
******************************************************************************/
void IntegerParameterUI::updateUI()
{
	if(editObject() && spinner() && !spinner()->isDragging()) {
		try {
			if(isReferenceFieldUI()) {
				if(Controller* ctrl = dynamic_object_cast<Controller>(parameterObject()))
					spinner()->setIntValue(ctrl->currentIntValue());
			}
			else {
				QVariant val(0);
				if(isQtPropertyUI()) {
					val = editObject()->property(propertyName());
					OVITO_ASSERT_MSG(val.isValid() && val.canConvert(QVariant::Int), "IntegerParameterUI::updateUI()", QString("The object class %1 does not define a property with the name %2 that can be cast to integer type.").arg(editObject()->metaObject()->className(), QString(propertyName())).toLocal8Bit().constData());
					if(!val.isValid() || !val.canConvert(QVariant::Int)) {
						editObject()->throwException(tr("The object class %1 does not define a property with the name %2 that can be cast to integer type.").arg(editObject()->metaObject()->className(), QString(propertyName())));
					}
				}
				else if(isPropertyFieldUI()) {
					val = editObject()->getPropertyFieldValue(*propertyField());
					OVITO_ASSERT(val.isValid());
				}
				spinner()->setIntValue(val.toInt());
			}
		}
		catch(const Exception& ex) {
			ex.reportError();
		}
	}
}

/******************************************************************************
* Gets the minimum value to be entered.
* This value is in native controller units.
******************************************************************************/
int IntegerParameterUI::minValue() const
{
	return (spinner() ? (int)spinner()->minValue() : std::numeric_limits<int>::lowest());
}

/******************************************************************************
* Sets the minimum value to be entered.
* This value must be specified in native controller units.
******************************************************************************/
void IntegerParameterUI::setMinValue(int minValue)
{
	if(spinner()) spinner()->setMinValue(minValue);
}

/******************************************************************************
* Gets the maximum value to be entered.
* This value is in native controller units.
******************************************************************************/
int IntegerParameterUI::maxValue() const
{
	return (spinner() ? (int)spinner()->maxValue() : std::numeric_limits<int>::max());
}

/******************************************************************************
* Sets the maximum value to be entered.
* This value must be specified in native controller units.
******************************************************************************/
void IntegerParameterUI::setMaxValue(int maxValue)
{
	if(spinner()) spinner()->setMaxValue(maxValue);
}

}	// End of namespace
