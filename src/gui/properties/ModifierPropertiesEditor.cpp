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

#include <gui/GUI.h>
#include <core/dataset/pipeline/Modifier.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "ModifierPropertiesEditor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

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
		return modApp->evaluateInputPreliminary();
	}
	return {};
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
