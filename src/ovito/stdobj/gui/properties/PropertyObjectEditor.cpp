////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
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

#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/stdobj/properties/ElementType.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/gui/properties/RefTargetListParameterUI.h>
#include "PropertyObjectEditor.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(PropertyObjectEditor);
SET_OVITO_OBJECT_EDITOR(PropertyObject, PropertyObjectEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void PropertyObjectEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(QString(), rolloutParams, "scene_objects.particles.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(0);

	// Derive a custom class from the list parameter UI to display the particle type colors.
	class CustomRefTargetListParameterUI : public RefTargetListParameterUI {
	public:
		using RefTargetListParameterUI::RefTargetListParameterUI;
	protected:
		virtual QVariant getItemData(RefTarget* target, const QModelIndex& index, int role) override {
			if(role == Qt::DecorationRole && target != NULL) {
				return (QColor)static_object_cast<ElementType>(target)->color();
			}
			else return RefTargetListParameterUI::getItemData(target, index, role);
		}
		/// Opens a sub-editor for the object that is selected in the list view.
		virtual void openSubEditor() override {
			RefTargetListParameterUI::openSubEditor();
			editor()->container()->updateRollouts();
		}
	};

	QWidget* subEditorContainer = new QWidget(rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(subEditorContainer);
	sublayout->setContentsMargins(0,0,0,0);
	layout->addWidget(subEditorContainer);

	RefTargetListParameterUI* elementTypesListUI = new CustomRefTargetListParameterUI(this, PROPERTY_FIELD(PropertyObject::elementTypes), RolloutInsertionParameters().insertInto(subEditorContainer));
	layout->insertWidget(0, elementTypesListUI->listWidget());
}

}	// End of namespace
}	// End of namespace
