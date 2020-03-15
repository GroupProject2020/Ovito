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
#include <ovito/gui/web/properties/ParameterUI.h>
#include <ovito/core/dataset/DataSet.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(ParameterUI);
DEFINE_REFERENCE_FIELD(ParameterUI, editObject);

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void ParameterUI::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(editObject)) {
		updatePropertyField();
		updateUI();
	}
	RefMaker::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool ParameterUI::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject() && event.type() == ReferenceEvent::TargetChanged) {
		// The edited object has changed -> update value shown in UI.
		updateUI();
	}

#if 0
	if(isReferenceFieldUI()) {
		if(source == editObject() && event.type() == ReferenceEvent::ReferenceChanged) {
			if(propertyField() == static_cast<const ReferenceFieldEvent&>(event).field()) {
				// The parameter value object stored in the reference field of the edited object
				// has been replaced by another one, so update our own reference to the parameter value object.
				if(editObject()->getReferenceField(*propertyField()) != parameterObject())
					resetUI();
			}
		}
		else if(source == parameterObject() && event.type() == ReferenceEvent::TargetChanged) {
			// The parameter value object has changed -> update value shown in UI.
			updateUI();
		}
	}
	else if(source == editObject() && event.type() == ReferenceEvent::TargetChanged) {
		// The edited object has changed -> update value shown in UI.
		updateUI();
	}
#endif
	return RefMaker::referenceEvent(source, event);
}

/******************************************************************************
* Updates the internal pointer to the RefMaker property or reference field.
******************************************************************************/
void ParameterUI::updatePropertyField()
{
	if(editObject() && !propertyName().isEmpty()) {
		_propertyField = editObject()->getOOMetaClass().findPropertyField(qPrintable(propertyName()), true);
	}
	else {
		_propertyField = nullptr;
	}
}

/******************************************************************************
* Obtains the current value of the parameter from the C++ object.
******************************************************************************/
QVariant ParameterUI::getCurrentValue() const
{
	if(editObject()) {
		if(propertyField()) {
			return editObject()->getPropertyFieldValue(*propertyField());
		}
		else if(!propertyName().isEmpty()) {
			QVariant val = editObject()->property(qPrintable(propertyName()));
			OVITO_ASSERT_MSG(val.isValid(), "BooleanParameterUI::updateUI()", qPrintable(QString("The object class %1 does not define a property with the name %2 that can be cast to boolean type.").arg(editObject()->metaObject()->className(), propertyName())));
			if(!val.isValid())
				editObject()->throwException(tr("The object class %1 does not define a property with the name %2 that can be cast to boolean type.").arg(editObject()->metaObject()->className(), propertyName()));
			return val;
		}
	}
	return {};
}

/******************************************************************************
* Changes the current value of the C++ object parameter.
******************************************************************************/
void ParameterUI::setCurrentValue(const QVariant& val)
{
	if(editObject()) {
		UndoableTransaction::handleExceptions(editObject()->dataset()->undoStack(), tr("Change parameter"), [&]() {
			if(propertyField()) {
				editObject()->setPropertyFieldValue(*propertyField(), val);
			}
			else if(!propertyName().isEmpty()) {
				if(!editObject()->setProperty(qPrintable(propertyName()), val)) {
					OVITO_ASSERT_MSG(false, "ParameterUI::setCurrentValue()", qPrintable(QString("The value of property %1 of object class %2 could not be set.").arg(propertyName()).arg(editObject()->metaObject()->className())));
				}
			}
		});
	}
}

/******************************************************************************
* Updates the displayed value in the UI.
******************************************************************************/
void ParameterUI::updateUI()
{
	if(qmlProperty().isValid()) {
		qmlProperty().write(getCurrentValue());
	}
}

}	// End of namespace
