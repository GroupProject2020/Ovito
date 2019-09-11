///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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

#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/stdobj/gui/widgets/PropertyContainerParameterUI.h>
#include "InvertSelectionModifierEditor.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(InvertSelectionModifierEditor);
SET_OVITO_OBJECT_EDITOR(InvertSelectionModifier, InvertSelectionModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void InvertSelectionModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Invert selection"), rolloutParams, "particles.modifiers.invert_selection.html");

	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(8,8,8,8);
	layout->setSpacing(4);

	PropertyContainerParameterUI* pclassUI = new PropertyContainerParameterUI(this, PROPERTY_FIELD(GenericPropertyModifier::subject));
	layout->addWidget(new QLabel(tr("Operate on:")));
	layout->addWidget(pclassUI->comboBox());

	// List only property container that support element selection.
	pclassUI->setContainerFilter([](const PropertyContainer* container) {
		return container->getOOMetaClass().isValidStandardPropertyId(PropertyStorage::GenericSelectionProperty);
	});
}

}	// End of namespace
}	// End of namespace
