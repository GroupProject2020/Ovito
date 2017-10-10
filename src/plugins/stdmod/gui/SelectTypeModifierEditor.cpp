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

#include <plugins/stdmod/gui/StdModGui.h>
#include <plugins/stdobj/gui/widgets/PropertySelectionComboBox.h>
#include <plugins/stdobj/gui/widgets/PropertyClassParameterUI.h>
#include <plugins/stdmod/modifiers/SelectTypeModifier.h>
#include "SelectTypeModifierEditor.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(SelectTypeModifierEditor);
SET_OVITO_OBJECT_EDITOR(SelectTypeModifier, SelectTypeModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SelectTypeModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Select type"), rolloutParams, "particles.modifiers.select_particle_type.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	PropertyClassParameterUI* pclassUI = new PropertyClassParameterUI(this, PROPERTY_FIELD(GenericPropertyModifier::propertyClass));
	layout->addWidget(new QLabel(tr("Operate on:")));
	layout->addWidget(pclassUI->comboBox());
	
	_sourcePropertyUI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(SelectTypeModifier::sourceProperty), nullptr);
	layout->addWidget(new QLabel(tr("Property:")));
	layout->addWidget(_sourcePropertyUI->comboBox());
	connect(this, &PropertiesEditor::contentsChanged, this, [this](RefTarget* editObject) {
		_sourcePropertyUI->setPropertyClass(editObject ? static_object_cast<GenericPropertyModifier>(editObject)->propertyClass() : nullptr);
		updateElementTypeList();
	});
	// Show only type properties in the list that have some element types attached to them.
	_sourcePropertyUI->setPropertyFilter([](PropertyObject* property) {
		return property->elementTypes().empty() == false && property->componentCount() == 1 && property->dataType() == PropertyStorage::Int;
	});

	class MyListWidget : public QListWidget {
	public:
		MyListWidget() : QListWidget() {}
		virtual QSize sizeHint() const { return QSize(256, 192); }
	};
	_elementTypesBox = new MyListWidget();
	_elementTypesBox->setSelectionMode(QAbstractItemView::ExtendedSelection);
	layout->addWidget(new QLabel(tr("Types:"), rollout));
	layout->addWidget(_elementTypesBox);

	// Status label.
	layout->addSpacing(12);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* Updates the contents of the list box.
******************************************************************************/
void SelectTypeModifierEditor::updateElementTypeList()
{
	// Temporarily disable notification signals and display updates for the types list box.
	// Will be enabled again at the end of the function.
	disconnect(_elementTypesBox, &QListWidget::itemChanged, this, &SelectTypeModifierEditor::onElementTypeSelected);
	_elementTypesBox->setUpdatesEnabled(false);
	_elementTypesBox->clear();

	SelectTypeModifier* mod = static_object_cast<SelectTypeModifier>(editObject());
	if(!mod || !mod->propertyClass() || mod->sourceProperty().isNull()) {
		_elementTypesBox->setEnabled(false);
	}
	else {
		_elementTypesBox->setEnabled(true);
		
		// Populate types list based on the selected input property.
		for(ModifierApplication* modApp : modifierApplications()) {
			PipelineFlowState inputState = modApp->evaluateInputPreliminary();
			PropertyObject* inputProperty = mod->sourceProperty().findInState(inputState);
			if(inputProperty) {
				for(ElementType* type : inputProperty->elementTypes()) {
					if(!type) continue;					
					
					// Make sure we don't add two items with the same type ID.
					bool duplicate = false;
					for(int i = 0; i < _elementTypesBox->count(); i++) {
						if(_elementTypesBox->item(i)->data(Qt::UserRole).toInt() == type->id()) {
							duplicate = true;
							break;
						}
					}
					if(duplicate) continue;

					// Add a new list item for the element type.
					QListWidgetItem* item = new QListWidgetItem(type->name(), _elementTypesBox);
					item->setData(Qt::UserRole, type->id());
					item->setData(Qt::DecorationRole, (QColor)type->color());
					if(mod->selectedTypeIDs().contains(type->id()))
						item->setCheckState(Qt::Checked);
					else
						item->setCheckState(Qt::Unchecked);
					item->setFlags(Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren));
				}
			}
		}
	}

	// Re-enable updates.
	connect(_elementTypesBox, &QListWidget::itemChanged, this, &SelectTypeModifierEditor::onElementTypeSelected);
	_elementTypesBox->setUpdatesEnabled(true);
}

/******************************************************************************
* This is called when the user has selected another element type.
******************************************************************************/
void SelectTypeModifierEditor::onElementTypeSelected(QListWidgetItem* item)
{
	SelectTypeModifier* mod = static_object_cast<SelectTypeModifier>(editObject());
	if(!mod) return;

	QSet<int> types = mod->selectedTypeIDs();
	if(item->checkState() == Qt::Checked)
		types.insert(item->data(Qt::UserRole).toInt());
	else
		types.remove(item->data(Qt::UserRole).toInt());

	undoableTransaction(tr("Select type"), [mod, &types]() {
		mod->setSelectedTypeIDs(std::move(types));
	});
}

}	// End of namespace
}	// End of namespace
