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
#include <ovito/gui/desktop/properties/SubObjectParameterUI.h>
#include <ovito/core/dataset/UndoStack.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

IMPLEMENT_OVITO_CLASS(SubObjectParameterUI);

/******************************************************************************
* The constructor.
******************************************************************************/
SubObjectParameterUI::SubObjectParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& refField, const RolloutInsertionParameters& rolloutParams)
	: PropertyParameterUI(parentEditor, refField), _rolloutParams(rolloutParams)
{
}

/******************************************************************************
* This method is called when a new sub-object has been assigned to the reference field of the editable object
* this parameter UI is bound to. It is also called when the editable object itself has
* been replaced in the editor.
******************************************************************************/
void SubObjectParameterUI::resetUI()
{
	PropertyParameterUI::resetUI();

	try {

		// Close editor if it is no longer needed.
		if(subEditor()) {
			if(!parameterObject() || subEditor()->editObject() == nullptr ||
					subEditor()->editObject()->getOOClass() != parameterObject()->getOOClass() ||
					!isEnabled()) {

				_subEditor = nullptr;
			}
		}
		if(!parameterObject() || !isEnabled()) return;
		if(!subEditor()) {
			_subEditor = PropertiesEditor::create(parameterObject());
			if(subEditor()) {
				subEditor()->initialize(editor()->container(), editor()->mainWindow(), _rolloutParams, editor());
			}
		}

		if(subEditor()) {
			subEditor()->setEditObject(parameterObject());
		}
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void SubObjectParameterUI::setEnabled(bool enabled)
{
	if(enabled != isEnabled()) {
		PropertyParameterUI::setEnabled(enabled);
		if(editObject())
			resetUI();
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
