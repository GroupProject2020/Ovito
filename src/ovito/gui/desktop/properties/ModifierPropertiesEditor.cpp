////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "ModifierPropertiesEditor.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(ModifierPropertiesEditor);
DEFINE_REFERENCE_FIELD(ModifierPropertiesEditor, modifierApplication);

/******************************************************************************
* Constructor.
******************************************************************************/
ModifierPropertiesEditor::ModifierPropertiesEditor()
{
	connect(this, &ModifierPropertiesEditor::contentsReplaced, this, &ModifierPropertiesEditor::updateStatusLabel);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool ModifierPropertiesEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == modifierApplication() && event.type() == ReferenceEvent::ObjectStatusChanged) {
		updateStatusLabel();
		Q_EMIT modifierStatusChanged();
	}
	else if(source == modifierApplication() && event.type() == ReferenceEvent::PipelineCacheUpdated) {
		Q_EMIT modifierEvaluated();
	}
	return PropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void ModifierPropertiesEditor::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	// Whenever a new Modifier is being loaded into the editor,
	// update our reference to the current ModifierApplication.
	if(field == PROPERTY_FIELD(editObject)) {
		ModifierApplication* newModApp = nullptr;
		if(Modifier* modifier = dynamic_object_cast<Modifier>(newTarget)) {
			// Look up the ModifierApplication that is currently open in the parent editor of this modifier's editor.
			if(parentEditor())
				newModApp = dynamic_object_cast<ModifierApplication>(parentEditor()->editObject());
		}
		else if(ModifierPropertiesEditor* parent = dynamic_object_cast<ModifierPropertiesEditor>(parentEditor())) {
			newModApp = parent->modifierApplication();
		}
		_modifierApplication.set(this, PROPERTY_FIELD(modifierApplication), newModApp);
	}

	PropertiesEditor::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* Updates the text of the result label.
******************************************************************************/
void ModifierPropertiesEditor::updateStatusLabel()
{
	if(!_statusLabel) return;

	if(ModifierApplication* modApp = modifierApplication()) {
		_statusLabel->setStatus(modApp->status());
	}
	else {
		_statusLabel->clearStatus();
	}
}

/******************************************************************************
* Returns a widget that displays status messages of the modifier.
******************************************************************************/
StatusWidget* ModifierPropertiesEditor::statusLabel()
{
	if(!_statusLabel) {
		_statusLabel = new StatusWidget();
		updateStatusLabel();
	}
	return _statusLabel;
}

/******************************************************************************
* Returns the list of ModifierApplications of the modifier currently being edited.
******************************************************************************/
QVector<ModifierApplication*> ModifierPropertiesEditor::modifierApplications()
{
	if(Modifier* modifier = dynamic_object_cast<Modifier>(editObject()))
		return modifier->modifierApplications();
	else if(ModifierPropertiesEditor* parent = dynamic_object_cast<ModifierPropertiesEditor>(parentEditor()))
		return parent->modifierApplications();
	else
		return {};
}

/******************************************************************************
* Return the input data of the Modifier being edited (for the current
* ModifierApplication).
******************************************************************************/
PipelineFlowState ModifierPropertiesEditor::getModifierInput()
{
	if(ModifierApplication* modApp = modifierApplication()) {
		return modApp->evaluateInputSynchronous(dataset()->animationSettings()->time());
	}
	return {};
}

/******************************************************************************
* Return the output data of the Modifier being edited (for the current
* ModifierApplication).
******************************************************************************/
PipelineFlowState ModifierPropertiesEditor::getModifierOutput()
{
	if(ModifierApplication* modApp = modifierApplication()) {
		return modApp->evaluateSynchronous(dataset()->animationSettings()->time());
	}
	return {};
}

}	// End of namespace
