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
#include <ovito/gui/desktop/properties/ParameterUI.h>
#include <ovito/gui/desktop/properties/PropertiesEditor.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/dataset/animation/controller/KeyframeController.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/gui/desktop/dialogs/AnimationKeyEditorDialog.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

IMPLEMENT_OVITO_CLASS(ParameterUI);
IMPLEMENT_OVITO_CLASS(PropertyParameterUI);
DEFINE_REFERENCE_FIELD(ParameterUI, editObject);
DEFINE_REFERENCE_FIELD(PropertyParameterUI, parameterObject);

///////////////////////////////////// ParameterUI /////////////////////////////////////////

/******************************************************************************
* The constructor.
******************************************************************************/
ParameterUI::ParameterUI(QObject* parent) : RefMaker(nullptr), _enabled(true)
{
	setParent(parent);

	PropertiesEditor* editor = this->editor();
	if(editor) {
		if(editor->editObject())
			setEditObject(editor->editObject());

		// Connect to the contentsReplaced() signal of the editor to synchronize the
		// parameter UI's edit object with the editor's edit object.
		connect(editor, &PropertiesEditor::contentsReplaced, this, &ParameterUI::setEditObject);
	}
}

///////////////////////////////////// PropertyParameterUI /////////////////////////////////////////

/******************************************************************************
* Constructor for a Qt property.
******************************************************************************/
PropertyParameterUI::PropertyParameterUI(QObject* parent, const char* propertyName) :
	ParameterUI(parent), _propertyName(propertyName), _propField(nullptr)
{
	OVITO_ASSERT(propertyName);
}

/******************************************************************************
* Constructor for a PropertyField or ReferenceField property.
******************************************************************************/
PropertyParameterUI::PropertyParameterUI(QObject* parent, const PropertyFieldDescriptor& propField) :
	ParameterUI(parent), _propertyName(nullptr), _propField(&propField)
{
	// If requested, save parameter value to application's settings store each time the user changes it.
	if(propField.flags().testFlag(PROPERTY_FIELD_MEMORIZE))
		connect(this, &PropertyParameterUI::valueEntered, this, &PropertyParameterUI::memorizeDefaultParameterValue);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool PropertyParameterUI::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
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
	return ParameterUI::referenceEvent(source, event);
}

/******************************************************************************
* This method is called when parameter object has been assigned to the reference field of the editable object
* this parameter UI is bound to. It is also called when the editable object itself has
* been replaced in the editor.
******************************************************************************/
void PropertyParameterUI::resetUI()
{
	if(editObject() && isReferenceFieldUI()) {
		OVITO_CHECK_OBJECT_POINTER(editObject());
		OVITO_ASSERT(editObject() == NULL || editObject()->getOOClass().isDerivedFrom(*propertyField()->definingClass()));

		// Bind this parameter UI to the parameter object of the new edited object.
		setParameterObject(editObject()->getReferenceField(*propertyField()));
	}
	else {
		setParameterObject(nullptr);
	}

	ParameterUI::resetUI();
}

/******************************************************************************
* This slot is called when the user has changed the value of the parameter.
* It stores the new value in the application's settings store so that it can be used
* as the default initialization value next time when a new object of the same class is created.
******************************************************************************/
void PropertyParameterUI::memorizeDefaultParameterValue()
{
	if(!editObject())
		return;

	if(isPropertyFieldUI()) {
		propertyField()->memorizeDefaultValue(editObject());
	}
	else if(isReferenceFieldUI() && !propertyField()->isVector()) {
		Controller* ctrl = dynamic_object_cast<Controller>(parameterObject());
		if(ctrl) {
			QSettings settings;
			settings.beginGroup(editObject()->getOOClass().plugin()->pluginId());
			settings.beginGroup(editObject()->getOOClass().name());
			if(ctrl->controllerType() == Controller::ControllerTypeFloat) {
				settings.setValue(propertyField()->identifier(), QVariant::fromValue(ctrl->currentFloatValue()));
			}
			else if(ctrl->controllerType() == Controller::ControllerTypeInt) {
				settings.setValue(propertyField()->identifier(), QVariant::fromValue(ctrl->currentIntValue()));
			}
			else if(ctrl->controllerType() == Controller::ControllerTypeVector3) {
				settings.setValue(propertyField()->identifier(), QVariant::fromValue(ctrl->currentVector3Value()));
			}
		}
	}
}

/******************************************************************************
* Opens the animation key editor if the parameter managed by this UI class
* is animatable.
******************************************************************************/
void PropertyParameterUI::openAnimationKeyEditor()
{
	OVITO_ASSERT(editor() != nullptr);

	KeyframeController* ctrl = dynamic_object_cast<KeyframeController>(parameterObject());
	if(!ctrl) return;

	AnimationKeyEditorDialog dlg(ctrl, propertyField(), editor()->container(), editor()->mainWindow());
	dlg.exec();
}


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
