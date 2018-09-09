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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/modifier/dxa/DislocationAnalysisModifier.h>
#include <plugins/particles/gui/modifier/analysis/StructureListParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/BooleanGroupBoxParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <gui/properties/SubObjectParameterUI.h>
#include "DislocationAnalysisModifierEditor.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocationAnalysisModifierEditor);
SET_OVITO_OBJECT_EDITOR(DislocationAnalysisModifier, DislocationAnalysisModifierEditor);

IMPLEMENT_OVITO_CLASS(DislocationTypeListParameterUI);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void DislocationAnalysisModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Dislocation analysis"), rolloutParams, "particles.modifiers.dislocation_analysis.html");

    QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);

	QGroupBox* structureBox = new QGroupBox(tr("Input crystal type"));
	layout->addWidget(structureBox);
	QVBoxLayout* sublayout1 = new QVBoxLayout(structureBox);
	sublayout1->setContentsMargins(4,4,4,4);
	VariantComboBoxParameterUI* crystalStructureUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::inputCrystalStructure));

	crystalStructureUI->comboBox()->addItem(tr("Face-centered cubic (FCC)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_FCC));
	crystalStructureUI->comboBox()->addItem(tr("Hexagonal close-packed (HCP)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_HCP));
	crystalStructureUI->comboBox()->addItem(tr("Body-centered cubic (BCC)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_BCC));
	crystalStructureUI->comboBox()->addItem(tr("Diamond cubic / Zinc blende"), QVariant::fromValue((int)StructureAnalysis::LATTICE_CUBIC_DIAMOND));
	crystalStructureUI->comboBox()->addItem(tr("Diamond hexagonal / Wurtzite"), QVariant::fromValue((int)StructureAnalysis::LATTICE_HEX_DIAMOND));
	sublayout1->addWidget(crystalStructureUI->comboBox());

	QGroupBox* dxaParamsBox = new QGroupBox(tr("DXA parameters"));
	layout->addWidget(dxaParamsBox);
	QGridLayout* sublayout = new QGridLayout(dxaParamsBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);

	IntegerParameterUI* maxTrialCircuitSizeUI = new IntegerParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::maxTrialCircuitSize));
	sublayout->addWidget(maxTrialCircuitSizeUI->label(), 0, 0);
	sublayout->addLayout(maxTrialCircuitSizeUI->createFieldLayout(), 0, 1);

	IntegerParameterUI* circuitStretchabilityUI = new IntegerParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::circuitStretchability));
	sublayout->addWidget(circuitStretchabilityUI->label(), 1, 0);
	sublayout->addLayout(circuitStretchabilityUI->createFieldLayout(), 1, 1);

	QGroupBox* advancedParamsBox = new QGroupBox(tr("Advanced settings"));
	layout->addWidget(advancedParamsBox);
	sublayout = new QGridLayout(advancedParamsBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(0, 1);

	// Color by type
	BooleanParameterUI* colorByTypeUI = new BooleanParameterUI(this, PROPERTY_FIELD(StructureIdentificationModifier::colorByType));
	sublayout->addWidget(colorByTypeUI->checkBox(), 0, 0);

	BooleanParameterUI* onlySelectedParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(StructureIdentificationModifier::onlySelectedParticles));
	sublayout->addWidget(onlySelectedParticlesUI->checkBox(), 1, 0);

	BooleanParameterUI* outputInterfaceMeshUI = new BooleanParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::outputInterfaceMesh));
	sublayout->addWidget(outputInterfaceMeshUI->checkBox(), 2, 0);

	BooleanParameterUI* onlyPerfectDislocationsUI = new BooleanParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::onlyPerfectDislocations));
	sublayout->addWidget(onlyPerfectDislocationsUI->checkBox(), 3, 0);

	// Status label.
	layout->addWidget(statusLabel());	

	// Structure list.
	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this);
	layout->addSpacing(10);
	layout->addWidget(new QLabel(tr("Structure analysis results:")));
	layout->addWidget(structureTypesPUI->tableWidget());

	// Burgers vector list.
	_burgersFamilyListUI.reset(new DislocationTypeListParameterUI());
	layout->addSpacing(10);
	layout->addWidget(new QLabel(tr("Dislocation analysis results:")));
	layout->addWidget(_burgersFamilyListUI->tableWidget());
	connect(this, &ModifierPropertiesEditor::modifierEvaluated, [this]() {
		// Get the current data pipeline output generated by the modifier.
		_burgersFamilyListUI->updateDislocationCounts(getModifierOutput(), modifierApplication());
	});
	connect(this, &ModifierPropertiesEditor::contentsReplaced, [this]() {
		// Get the current data pipeline output generated by the modifier.
		_burgersFamilyListUI->updateDislocationCounts(getModifierOutput(), modifierApplication());
	});

	// Line postprocessing.
	rollout = createRollout(tr("Line post-processing"), rolloutParams.after(rollout), "particles.modifiers.dislocation_analysis.html");

	layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);

	BooleanGroupBoxParameterUI* lineSmoothingEnabledUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::lineSmoothingEnabled));
	lineSmoothingEnabledUI->groupBox()->setTitle(tr("Line smoothing"));
	sublayout = new QGridLayout(lineSmoothingEnabledUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(lineSmoothingEnabledUI->groupBox());

	IntegerParameterUI* lineSmoothingLevelUI = new IntegerParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::lineSmoothingLevel));
	sublayout->addWidget(lineSmoothingLevelUI->label(), 0, 0);
	sublayout->addLayout(lineSmoothingLevelUI->createFieldLayout(), 0, 1);

	BooleanGroupBoxParameterUI* lineCoarseningEnabledUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::lineCoarseningEnabled));
	lineCoarseningEnabledUI->groupBox()->setTitle(tr("Line coarsening"));
	sublayout = new QGridLayout(lineCoarseningEnabledUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(lineCoarseningEnabledUI->groupBox());

	FloatParameterUI* linePointIntervalUI = new FloatParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::linePointInterval));
	sublayout->addWidget(linePointIntervalUI->label(), 0, 0);
	sublayout->addLayout(linePointIntervalUI->createFieldLayout(), 0, 1);

	// Surface post-processing.
	rollout = createRollout(tr("Surface post-processing"), rolloutParams.after(rollout), "particles.modifiers.dislocation_analysis.html");
	
	QGridLayout* gridlayout = new QGridLayout(rollout);
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setSpacing(6);
	gridlayout->setColumnStretch(1, 1);

	IntegerParameterUI* defectMeshSmoothingLevelUI = new IntegerParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::defectMeshSmoothingLevel));
	gridlayout->addWidget(defectMeshSmoothingLevelUI->label(), 0, 0);
	gridlayout->addLayout(defectMeshSmoothingLevelUI->createFieldLayout(), 0, 1);
}

/******************************************************************************
* Constructor.
******************************************************************************/
DislocationTypeListParameterUI::DislocationTypeListParameterUI(QObject* parent)
	: RefTargetListParameterUI(parent, PROPERTY_FIELD(StructurePattern::burgersVectorFamilies))
{
	connect(tableWidget(220), &QTableWidget::doubleClicked, this, &DislocationTypeListParameterUI::onDoubleClickDislocationType);
	tableWidget()->setAutoScroll(false);
}

/******************************************************************************
* Obtains the current statistics from the pipeline. 
******************************************************************************/
void DislocationTypeListParameterUI::updateDislocationCounts(const PipelineFlowState& state, ModifierApplication* modApp)
{
	// Access the data series in the pipeline state containing the dislocation counts and lengths.
	_dislocationCounts = modApp ? state.getObjectBy<DataSeriesObject>(modApp, QStringLiteral("disloc-counts")) : nullptr;
	_dislocationLengths = modApp ? state.getObjectBy<DataSeriesObject>(modApp, QStringLiteral("disloc-lengths")) : nullptr;
	const PatternCatalog* patternCatalog = state.getObject<PatternCatalog>();
	int crystalStructure = 0;
	if(modApp) {
		if(DislocationAnalysisModifier* modifier = dynamic_object_cast<DislocationAnalysisModifier>(modApp->modifier()))
			crystalStructure = modifier->inputCrystalStructure();
	}
	setEditObject(patternCatalog ? patternCatalog->structureById(crystalStructure) : nullptr);
}

/******************************************************************************
* Returns a data item from the list data model.
******************************************************************************/
QVariant DislocationTypeListParameterUI::getItemData(RefTarget* target, const QModelIndex& index, int role)
{
	const BurgersVectorFamily* family = dynamic_object_cast<BurgersVectorFamily>(target);
	if(family) {
		if(role == Qt::DisplayRole) {
			if(index.column() == 1) {
				return family->name();
			}
			else if(index.column() == 2 && _dislocationCounts) {
				if(const PropertyObject* yprop = _dislocationCounts->getY()) {
					if(yprop->size() > family->id() && yprop->dataType() == PropertyStorage::Int)
						return yprop->getInt(family->id());
				}
			}
			else if(index.column() == 3 && _dislocationLengths) {
				if(const PropertyObject* yprop = _dislocationLengths->getY()) {
					if(yprop->size() > family->id() && yprop->dataType() == PropertyStorage::Float)
						return QString::number(yprop->getFloat(family->id()));
				}
			}
		}
		else if(role == Qt::DecorationRole) {
			if(index.column() == 0)
				return (QColor)family->color();
		}
	}
	return QVariant();
}

/******************************************************************************
* Is called when the user has double-clicked on one of the dislocation
* types in the list widget.
******************************************************************************/
void DislocationTypeListParameterUI::onDoubleClickDislocationType(const QModelIndex& index)
{
	// Let the user select a color for the structure type.
	BurgersVectorFamily* family = static_object_cast<BurgersVectorFamily>(selectedObject());
	if(!family) return;

	QColor oldColor = (QColor)family->color();
	QColor newColor = QColorDialog::getColor(oldColor, _viewWidget);
	if(!newColor.isValid() || newColor == oldColor) return;

	undoableTransaction(tr("Change dislocation type color"), [family, newColor]() {
		family->setColor(Color(newColor));
	});
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
