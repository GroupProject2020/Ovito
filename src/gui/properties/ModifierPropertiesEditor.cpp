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
#include "ModifierPropertiesEditor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

IMPLEMENT_OVITO_CLASS(ModifierPropertiesEditor);

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
	if(event.type() == ReferenceEvent::ObjectStatusChanged) {
		updateStatusLabel();
	}
	return PropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the text of the result label.
******************************************************************************/
void ModifierPropertiesEditor::updateStatusLabel()
{
	if(!_statusLabel) return;

	if(Modifier* modifier = static_object_cast<Modifier>(editObject())) {
		_statusLabel->setStatus(modifier->globalStatus());
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
	Modifier* mod = static_object_cast<Modifier>(editObject());
	if(!mod) return {};
	return mod->modifierApplications();
}

/******************************************************************************
* Returns one of the ModifierApplications of the modifier currently being edited.
******************************************************************************/
ModifierApplication* ModifierPropertiesEditor::someModifierApplication()
{
	if(Modifier* modifier = static_object_cast<Modifier>(editObject()))
		return modifier->someModifierApplication();
	else
		return nullptr;
}

/******************************************************************************
* Return the input data of the Modifier being edited for one of its 
* ModifierApplications.
******************************************************************************/
PipelineFlowState ModifierPropertiesEditor::getSomeModifierInput()
{
	if(Modifier* modifier = static_object_cast<Modifier>(editObject())) {
		if(ModifierApplication* modApp = modifier->someModifierApplication()) {
			return modApp->evaluateInputPreliminary();
		}
	}
	return {};
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
