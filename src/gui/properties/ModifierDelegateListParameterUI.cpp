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
#include <core/dataset/pipeline/DelegatingModifier.h>
#include <gui/properties/ModifierPropertiesEditor.h>
#include "ModifierDelegateListParameterUI.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

IMPLEMENT_OVITO_CLASS(ModifierDelegateListParameterUI);

/******************************************************************************
* The constructor.
******************************************************************************/
ModifierDelegateListParameterUI::ModifierDelegateListParameterUI(QObject* parentEditor, const RolloutInsertionParameters& rolloutParams, OvitoClassPtr defaultEditorClass)
	: RefTargetListParameterUI(parentEditor, PROPERTY_FIELD(MultiDelegatingModifier::delegates), rolloutParams, defaultEditorClass)
{
}

/******************************************************************************
* Returns a data item from the list data model.
******************************************************************************/
QVariant ModifierDelegateListParameterUI::getItemData(RefTarget* target, const QModelIndex& index, int role)
{
	if(role == Qt::DisplayRole) {
		if(index.column() == 0 && target) {
			return target->objectTitle();
		}
	}
	else if(role == Qt::CheckStateRole) {
		if(index.column() == 0) {
			if(ModifierDelegate* delegate = dynamic_object_cast<ModifierDelegate>(target))
				return delegate->isEnabled() ? Qt::Checked : Qt::Unchecked;
		}
	}
	return {};
}

/******************************************************************************
* Sets the role data for the item at index to value.
******************************************************************************/
bool ModifierDelegateListParameterUI::setItemData(RefTarget* target, const QModelIndex& index, const QVariant& value, int role)
{
	if(index.column() == 0 && role == Qt::CheckStateRole) {
		if(ModifierDelegate* delegate = dynamic_object_cast<ModifierDelegate>(target)) {
			bool enabled = (value.toInt() == Qt::Checked);
			undoableTransaction(tr("Enable/disable data element"), [delegate, enabled]() {
				delegate->setEnabled(enabled);
			});
			return true;
		}
	}

	return RefTargetListParameterUI::setItemData(target, index, value, role);
}

/******************************************************************************
* Returns the model/view item flags for the given entry.
******************************************************************************/
Qt::ItemFlags ModifierDelegateListParameterUI::getItemFlags(RefTarget* target, const QModelIndex& index)
{
	Qt::ItemFlags flags = RefTargetListParameterUI::getItemFlags(target, index);
	if(index.column() == 0) {
		if(ModifierDelegate* delegate = dynamic_object_cast<ModifierDelegate>(target)) {
			if(ModifierPropertiesEditor* editor = dynamic_object_cast<ModifierPropertiesEditor>(this->editor())) {
				const PipelineFlowState& input = editor->getModifierInput();
				if(input.isEmpty() || !delegate->getOOMetaClass().isApplicableTo(*input.data())) {
					flags &= ~Qt::ItemIsEnabled;
				}
			}
		}		
		return flags | Qt::ItemIsUserCheckable;
	}
	return flags;
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool ModifierDelegateListParameterUI::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::ModifierInputChanged) {
		updateColumns(0, 0);
	}
	return RefTargetListParameterUI::referenceEvent(source, event);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
