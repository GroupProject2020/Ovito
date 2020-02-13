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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/modifier/modify/CreateBondsModifier.h>
#include <ovito/gui/desktop/properties/IntegerRadioButtonParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/SubObjectParameterUI.h>
#include "CreateBondsModifierEditor.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(CreateBondsModifierEditor);
SET_OVITO_OBJECT_EDITOR(CreateBondsModifier, CreateBondsModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CreateBondsModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Create bonds"), rolloutParams, "particles.modifiers.create_bonds.html");

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(6);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(0,0,0,0);
	gridlayout->setColumnStretch(1, 1);

	IntegerRadioButtonParameterUI* cutoffModePUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(CreateBondsModifier::cutoffMode));
	QRadioButton* uniformCutoffModeBtn = cutoffModePUI->addRadioButton(CreateBondsModifier::UniformCutoff, tr("Uniform cutoff radius"));

	// Cutoff parameter.
	FloatParameterUI* cutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(CreateBondsModifier::uniformCutoff));
	gridlayout->addWidget(uniformCutoffModeBtn, 0, 0);
	gridlayout->addLayout(cutoffRadiusPUI->createFieldLayout(), 0, 1);
	cutoffRadiusPUI->setEnabled(false);
	connect(uniformCutoffModeBtn, &QRadioButton::toggled, cutoffRadiusPUI, &FloatParameterUI::setEnabled);

	layout1->addLayout(gridlayout);

	QRadioButton* pairCutoffModeBtn = cutoffModePUI->addRadioButton(CreateBondsModifier::PairCutoff, tr("Pair-wise cutoffs:"));
	layout1->addWidget(pairCutoffModeBtn);

	_pairCutoffTable = new QTableView();
	_pairCutoffTable->verticalHeader()->setVisible(false);
	_pairCutoffTable->setEnabled(false);
	_pairCutoffTableModel = new PairCutoffTableModel(_pairCutoffTable);
	_pairCutoffTable->setModel(_pairCutoffTableModel);
	connect(pairCutoffModeBtn,&QRadioButton::toggled, _pairCutoffTable, &QTableView::setEnabled);
	layout1->addWidget(_pairCutoffTable);

	BooleanParameterUI* onlyIntraMoleculeBondsUI = new BooleanParameterUI(this, PROPERTY_FIELD(CreateBondsModifier::onlyIntraMoleculeBonds));
	layout1->addWidget(onlyIntraMoleculeBondsUI->checkBox());

	// Lower cutoff parameter.
	gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(0,0,0,0);
	gridlayout->setColumnStretch(1, 1);
	FloatParameterUI* minCutoffPUI = new FloatParameterUI(this, PROPERTY_FIELD(CreateBondsModifier::minimumCutoff));
	gridlayout->addWidget(minCutoffPUI->label(), 0, 0);
	gridlayout->addLayout(minCutoffPUI->createFieldLayout(), 0, 1);
	layout1->addLayout(gridlayout);

	// Status label.
	layout1->addSpacing(10);
	layout1->addWidget(statusLabel());

	// Open a sub-editor for the bonds vis element.
	new SubObjectParameterUI(this, PROPERTY_FIELD(CreateBondsModifier::bondsVis), rolloutParams.after(rollout));

	// Open a sub-editor for the bond type.
	new SubObjectParameterUI(this, PROPERTY_FIELD(CreateBondsModifier::bondType), rolloutParams.after(rollout).setTitle(tr("New")));

	// Update pair-wise cutoff table whenever a modifier has been loaded into the editor.
	connect(this, &CreateBondsModifierEditor::contentsReplaced, this, &CreateBondsModifierEditor::updatePairCutoffList);
	connect(this, &CreateBondsModifierEditor::contentsChanged, this, &CreateBondsModifierEditor::updatePairCutoffListValues);
}

/******************************************************************************
* Updates the contents of the pair-wise cutoff table.
******************************************************************************/
void CreateBondsModifierEditor::updatePairCutoffList()
{
	CreateBondsModifier* mod = static_object_cast<CreateBondsModifier>(editObject());
	if(!mod) return;

	// Obtain the list of particle types in the modifier's input.
	PairCutoffTableModel::ContentType pairCutoffs;
	const PipelineFlowState& inputState = getModifierInput();
	if(const ParticlesObject* particles = inputState.getObject<ParticlesObject>()) {
		if(const PropertyObject* typeProperty = particles->getProperty(ParticlesObject::TypeProperty)) {
			for(auto ptype1 = typeProperty->elementTypes().constBegin(); ptype1 != typeProperty->elementTypes().constEnd(); ++ptype1) {
				for(auto ptype2 = ptype1; ptype2 != typeProperty->elementTypes().constEnd(); ++ptype2) {
					pairCutoffs.emplace_back(OORef<ElementType>(*ptype1), OORef<ElementType>(*ptype2));
				}
			}
		}
	}
	_pairCutoffTableModel->setContent(mod, std::move(pairCutoffs));
}

/******************************************************************************
* Updates the cutoff values in the pair-wise cutoff table.
******************************************************************************/
void CreateBondsModifierEditor::updatePairCutoffListValues()
{
	_pairCutoffTableModel->updateContent();
}

/******************************************************************************
* Returns data from the pair-cutoff table model.
******************************************************************************/
QVariant CreateBondsModifierEditor::PairCutoffTableModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::DisplayRole || role == Qt::EditRole) {
		if(index.column() == 0) return _data[index.row()].first->nameOrNumericId();
		else if(index.column() == 1) return _data[index.row()].second->nameOrNumericId();
		else if(index.column() == 2) {
			const auto& type1 = _data[index.row()].first;
			const auto& type2 = _data[index.row()].second;
			FloatType cutoffRadius = _modifier->getPairwiseCutoff(
				type1->name().isEmpty() ? QVariant::fromValue(type1->numericId()) : QVariant::fromValue(type1->name()),
				type2->name().isEmpty() ? QVariant::fromValue(type2->numericId()) : QVariant::fromValue(type2->name()));
			if(cutoffRadius > 0)
				return QString("%1").arg(cutoffRadius);
		}
	}
	else if(role == Qt::DecorationRole) {
		if(index.column() == 0) return (QColor)_data[index.row()].first->color();
		else if(index.column() == 1) return (QColor)_data[index.row()].second->color();
	}
	return QVariant();
}

/******************************************************************************
* Sets data in the pair-cutoff table model.
******************************************************************************/
bool CreateBondsModifierEditor::PairCutoffTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if(role == Qt::EditRole && index.column() == 2) {
		bool ok;
		FloatType cutoff = (FloatType)value.toDouble(&ok);
		if(!ok) cutoff = 0;
		UndoableTransaction::handleExceptions(_modifier->dataset()->undoStack(), tr("Change cutoff"), [this, &index, cutoff]() {
			const auto& type1 = _data[index.row()].first;
			const auto& type2 = _data[index.row()].second;
			_modifier->setPairwiseCutoff(
				type1->name().isEmpty() ? QVariant::fromValue(type1->numericId()) : QVariant::fromValue(type1->name()),
				type2->name().isEmpty() ? QVariant::fromValue(type2->numericId()) : QVariant::fromValue(type2->name()),
				cutoff);
		});
		return true;
	}
	return false;
}

}	// End of namespace
}	// End of namespace
