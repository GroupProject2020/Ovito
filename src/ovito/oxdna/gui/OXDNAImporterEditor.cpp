////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/gui/desktop/dialogs/HistoryFileDialog.h>
#include "OXDNAImporterEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(OXDNAImporterEditor);
SET_OVITO_OBJECT_EDITOR(OXDNAImporter, OXDNAImporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void OXDNAImporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("oxDNA"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* topologyBox = new QGroupBox(tr("Topology file"), rollout);
	layout->addWidget(topologyBox);
	QGridLayout* gridlayout = new QGridLayout(topologyBox);
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1,1);
	gridlayout->setVerticalSpacing(2);
	gridlayout->setHorizontalSpacing(6);

	_topologyFileField = new QLineEdit();
	_topologyFileField->setReadOnly(true);
	_topologyFileField->setFrame(false);
	_topologyFileField->setPlaceholderText(tr("Using automatic discovery"));
	gridlayout->addWidget(_topologyFileField, 0, 0, 1, 2);

	_pickTopologyFileBtn = new QPushButton(tr("Pick..."));
	_pickTopologyFileBtn->setEnabled(false);
	connect(_pickTopologyFileBtn, &QPushButton::clicked, this, &OXDNAImporterEditor::onChooseTopologyFile);
	gridlayout->addWidget(_pickTopologyFileBtn, 1, 0);

	connect(this, &PropertiesEditor::contentsChanged, this, &OXDNAImporterEditor::importerChanged);
}

/******************************************************************************
* Is called by the system when the importer changes.
******************************************************************************/
void OXDNAImporterEditor::importerChanged(RefTarget* editObject)
{
	if(OXDNAImporter* importer = static_object_cast<OXDNAImporter>(editObject)) {
		_pickTopologyFileBtn->setEnabled(true);
		if(importer->topologyFileUrl().isValid())
			_topologyFileField->setText(importer->topologyFileUrl().toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded));
		else
			_topologyFileField->clear();
	}
	else {
		_pickTopologyFileBtn->setEnabled(false);
		_topologyFileField->clear();
	}
}

/******************************************************************************
* Lets the user choose a oxDNA topology file.
******************************************************************************/
void OXDNAImporterEditor::onChooseTopologyFile()
{
	OXDNAImporter* importer = static_object_cast<OXDNAImporter>(editObject());
	if(!importer) return;

	HistoryFileDialog fileDialog(QStringLiteral("import"), container(), tr("Pick oxDNA topology file"));
	fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	fileDialog.setFileMode(QFileDialog::ExistingFile);

	if(importer->topologyFileUrl().isValid() && importer->topologyFileUrl().isLocalFile())
		fileDialog.selectFile(importer->topologyFileUrl().toLocalFile());

	if(fileDialog.exec() == QDialog::Accepted) {
		undoableTransaction(tr("Set topology file"), [importer, &fileDialog]() {
			const QStringList& selectedFiles = fileDialog.selectedFiles();
			if(!selectedFiles.empty()) {
				importer->setTopologyFileUrl(QUrl::fromLocalFile(selectedFiles.front()));
				importer->requestReload();
			}
		});
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
