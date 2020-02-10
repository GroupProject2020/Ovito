////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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

#include <ovito/vorotop/VoroTopPlugin.h>
#include <ovito/particles/gui/modifier/analysis/StructureListParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/FilenameParameterUI.h>
#include <ovito/gui/desktop/dialogs/HistoryFileDialog.h>
#include <ovito/gui/desktop/utilities/concurrent/ProgressDialog.h>
#include <ovito/core/utilities/concurrent/AsyncOperation.h>
#include "VoroTopModifierEditor.h"

namespace Ovito { namespace VoroTop {

IMPLEMENT_OVITO_CLASS(VoroTopModifierEditor);
SET_OVITO_OBJECT_EDITOR(VoroTopModifier, VoroTopModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void VoroTopModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("VoroTop analysis"), rolloutParams, "particles.modifiers.vorotop_analysis.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setSpacing(4);
	gridlayout->setColumnStretch(1, 1);
	int row = 0;

	// Filter filename.
	gridlayout->addWidget(new QLabel(tr("Filter:")), row++, 0, 1, 2);
	FilenameParameterUI* fileFileUI = new FilenameParameterUI(this, PROPERTY_FIELD(VoroTopModifier::filterFile));
	gridlayout->addWidget(fileFileUI->selectorWidget(), row++, 0, 1, 2);
	connect(fileFileUI, &FilenameParameterUI::showSelectionDialog, this, &VoroTopModifierEditor::onLoadFilter);

	QLabel* label = new QLabel(tr("Filter definition files available from the <a href=\"https://www.seas.upenn.edu/~mlazar/VoroTop/filters.html\">VoroTop website</a>."));
	label->setWordWrap(true);
	label->setOpenExternalLinks(true);
	gridlayout->addWidget(label, row++, 0, 1, 2);

	// Atomic radii.
	BooleanParameterUI* useRadiiPUI = new BooleanParameterUI(this, PROPERTY_FIELD(VoroTopModifier::useRadii));
	gridlayout->addWidget(useRadiiPUI->checkBox(), row++, 0, 1, 2);

	// Only selected particles.
	BooleanParameterUI* onlySelectedPUI = new BooleanParameterUI(this, PROPERTY_FIELD(StructureIdentificationModifier::onlySelectedParticles));
	gridlayout->addWidget(onlySelectedPUI->checkBox(), row++, 0, 1, 2);

	layout->addLayout(gridlayout);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());

	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this, false);
	layout->addSpacing(10);
	layout->addWidget(new QLabel(tr("Structure types:")));
	layout->addWidget(structureTypesPUI->tableWidget());
	label = new QLabel(tr("<p style=\"font-size: small;\">Double-click to change colors.</p>"));
	label->setWordWrap(true);
	layout->addWidget(label);
}

/******************************************************************************
* Is called when the user presses the 'Load filter' button.
******************************************************************************/
void VoroTopModifierEditor::onLoadFilter()
{
	VoroTopModifier* mod = static_object_cast<VoroTopModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Load VoroTop filter"), [this, mod]() {

		HistoryFileDialog fileDialog(QStringLiteral("vorotop_filter"), container(), tr("Pick VoroTop filter file"),
			QString(), tr("VoroTop filter definition file (*)"));
		fileDialog.setFileMode(QFileDialog::ExistingFile);

		if(fileDialog.exec()) {
			QStringList selectedFiles = fileDialog.selectedFiles();
			if(!selectedFiles.empty()) {
				AsyncOperation loadOperation(mod->dataset()->taskManager());
				ProgressDialog progressDialog(container(), loadOperation.task(), tr("Loading filter"));
				mod->loadFilterDefinition(selectedFiles.front(), std::move(loadOperation));
			}
		}
	});
}


}	// End of namespace
}	// End of namespace
