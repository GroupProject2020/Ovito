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
#include <ovito/particles/import/lammps/LAMMPSBinaryDumpImporter.h>
#include <ovito/particles/gui/import/InputColumnMappingDialog.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include <ovito/gui/mainwin/MainWindow.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include "LAMMPSBinaryDumpImporterEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(LAMMPSBinaryDumpImporterEditor);
SET_OVITO_OBJECT_EDITOR(LAMMPSBinaryDumpImporter, LAMMPSBinaryDumpImporterEditor);

/******************************************************************************
* This is called by the system when the user has selected a new file to import.
******************************************************************************/
bool LAMMPSBinaryDumpImporterEditor::inspectNewFile(FileImporter* importer, const QUrl& sourceFile, QWidget* parent)
{
	// Retrieve column information of input file.
	LAMMPSBinaryDumpImporter* lammpsImporter = static_object_cast<LAMMPSBinaryDumpImporter>(importer);
	Future<InputColumnMapping> inspectFuture = lammpsImporter->inspectFileHeader(FileSourceImporter::Frame(sourceFile));
	if(!importer->dataset()->taskManager().waitForFuture(inspectFuture))
		return false;
	InputColumnMapping mapping = inspectFuture.result();

	if(lammpsImporter->columnMapping().size() != mapping.size()) {
		// If this is a newly created file importer, load old mapping from application settings store.
		if(lammpsImporter->columnMapping().empty()) {
			QSettings settings;
			settings.beginGroup("viz/importer/lammps_binary_dump/");
			if(settings.contains("colmapping")) {
				try {
					InputColumnMapping storedMapping;
					storedMapping.fromByteArray(settings.value("colmapping").toByteArray());
					std::copy_n(storedMapping.begin(), std::min(storedMapping.size(), mapping.size()), mapping.begin());
				}
				catch(Exception& ex) {
					ex.prependGeneralMessage(tr("Failed to load last used column-to-property mapping from application settings store."));
					ex.logError();
				}
			}
		}

		InputColumnMappingDialog dialog(mapping, parent);
		if(dialog.exec() == QDialog::Accepted) {
			lammpsImporter->setColumnMapping(dialog.mapping());
			return true;
		}

		return false;
	}
	else {
		// If number of columns did not change since last time, only update the stored file excerpt.
		InputColumnMapping newMapping = lammpsImporter->columnMapping();
		newMapping.setFileExcerpt(mapping.fileExcerpt());
		lammpsImporter->setColumnMapping(newMapping);
		return true;
	}
}

/******************************************************************************
 * Displays a dialog box that allows the user to edit the custom file column to particle
 * property mapping.
 *****************************************************************************/
bool LAMMPSBinaryDumpImporterEditor::showEditColumnMappingDialog(LAMMPSBinaryDumpImporter* importer, QWidget* parent)
{
	InputColumnMappingDialog dialog(importer->columnMapping(), parent);
	if(dialog.exec() == QDialog::Accepted) {
		importer->setColumnMapping(dialog.mapping());
		// Remember the user-defined mapping for the next time.
		QSettings settings;
		settings.beginGroup("viz/importer/lammps_binary_dump/");
		settings.setValue("colmapping", dialog.mapping().toByteArray());
		settings.endGroup();
		return true;
	}
	return false;
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void LAMMPSBinaryDumpImporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("LAMMPS binary dump reader"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* optionsBox = new QGroupBox(tr("Options"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(optionsBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(optionsBox);

	// Multi-timestep file
	BooleanParameterUI* multitimestepUI = new BooleanParameterUI(this, PROPERTY_FIELD(ParticleImporter::isMultiTimestepFile));
	sublayout->addWidget(multitimestepUI->checkBox());

	// Sort particles
	BooleanParameterUI* sortParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(ParticleImporter::sortParticles));
	sublayout->addWidget(sortParticlesUI->checkBox());

	QGroupBox* columnMappingBox = new QGroupBox(tr("File columns"), rollout);
	sublayout = new QVBoxLayout(columnMappingBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(columnMappingBox);

	QPushButton* editMappingButton = new QPushButton(tr("Edit column mapping..."));
	sublayout->addWidget(editMappingButton);
	connect(editMappingButton, &QPushButton::clicked, this, &LAMMPSBinaryDumpImporterEditor::onEditColumnMapping);
}

/******************************************************************************
* Is called when the user pressed the "Edit column mapping" button.
******************************************************************************/
void LAMMPSBinaryDumpImporterEditor::onEditColumnMapping()
{
	if(LAMMPSBinaryDumpImporter* importer = static_object_cast<LAMMPSBinaryDumpImporter>(editObject())) {
		UndoableTransaction::handleExceptions(importer->dataset()->undoStack(), tr("Change file column mapping"), [this, importer]() {
			if(showEditColumnMappingDialog(importer, mainWindow())) {
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
