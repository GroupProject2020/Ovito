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

#include <gui/GUI.h>
#include <gui/properties/PropertyClassParameterUI.h>
#include "ClearSelectionModifierEditor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene) OVITO_BEGIN_INLINE_NAMESPACE(StdObj) OVITO_BEGIN_INLINE_NAMESPACE(Internal)
	
IMPLEMENT_OVITO_CLASS(ClearSelectionModifierEditor);
SET_OVITO_OBJECT_EDITOR(ClearSelectionModifier, ClearSelectionModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ClearSelectionModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Operate on"), rolloutParams);

	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(8,8,8,8);
	layout->setSpacing(4);

	PropertyClassParameterUI* pclassUI = new PropertyClassParameterUI(this, PROPERTY_FIELD(GenericPropertyModifier::propertyClass));
	layout->addWidget(new QLabel(tr("Operate on:")));
	layout->addWidget(pclassUI->comboBox());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
