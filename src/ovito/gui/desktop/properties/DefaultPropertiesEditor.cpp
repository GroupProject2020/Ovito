////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/gui/desktop/properties/DefaultPropertiesEditor.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(DefaultPropertiesEditor);
SET_OVITO_OBJECT_EDITOR(RefTarget, DefaultPropertiesEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void DefaultPropertiesEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	_rolloutParams = rolloutParams;
}

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void DefaultPropertiesEditor::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	PropertiesEditor::referenceReplaced(field, oldTarget, newTarget);
	if(field == PROPERTY_FIELD(editObject)) {
		updateSubEditors();
	}
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool DefaultPropertiesEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::ReferenceChanged) {
		updateSubEditors();
	}
	return PropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Rebuilds the list of sub-editors for the current edit object.
******************************************************************************/
void DefaultPropertiesEditor::updateSubEditors()
{
	OVITO_ASSERT(mainWindow() != nullptr);

	try {
		auto subEditorIter = _subEditors.begin();
		if(editObject()) {
			// Automatically open sub-editors for reference fields that have the PROPERTY_FIELD_OPEN_SUBEDITOR flag.
			for(auto fieldIter = editObject()->getOOMetaClass().propertyFields().crbegin(); fieldIter != editObject()->getOOMetaClass().propertyFields().crend(); ++fieldIter) {
				const PropertyFieldDescriptor* field = *fieldIter;
				if(!field->isReferenceField()) continue;
				if(field->isVector()) continue;
				if(!field->flags().testFlag(PROPERTY_FIELD_OPEN_SUBEDITOR)) continue;
				if(RefTarget* subobject = editObject()->getReferenceField(*field)) {
					// Open editor for this sub-object.
					if(subEditorIter != _subEditors.end() && (*subEditorIter)->editObject() != nullptr
							&& (*subEditorIter)->editObject()->getOOClass() == subobject->getOOClass()) {
						// Re-use existing editor.
						(*subEditorIter)->setEditObject(subobject);
						++subEditorIter;
					}
					else {
						// Create a new sub-editor for this sub-object.
						OORef<PropertiesEditor> editor = PropertiesEditor::create(subobject);
						if(editor) {
							editor->initialize(container(), mainWindow(), _rolloutParams, this);
							editor->setEditObject(subobject);
							_subEditors.erase(subEditorIter, _subEditors.end());
							_subEditors.push_back(std::move(editor));
							subEditorIter = _subEditors.end();
						}
					}
				}
			}
		}
		// Close excess sub-editors.
		_subEditors.erase(subEditorIter, _subEditors.end());
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

}	// End of namespace
