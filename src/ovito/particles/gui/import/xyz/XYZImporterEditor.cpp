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
#include <ovito/particles/import/xyz/XYZImporter.h>
#include <ovito/particles/gui/import/InputColumnMappingDialog.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/core/dataset/io/FileSource.h>
#include "XYZImporterEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(XYZImporterEditor);
SET_OVITO_OBJECT_EDITOR(XYZImporter, XYZImporterEditor);

/******************************************************************************
* This method is called by the FileSource each time a new source
* file has been selected by the user.
******************************************************************************/
bool XYZImporterEditor::inspectNewFile(FileImporter* importer, const QUrl& sourceFile, QWidget* parent)
{
	XYZImporter* xyzImporter = static_object_cast<XYZImporter>(importer);
	Future<InputColumnMapping> inspectFuture = xyzImporter->inspectFileHeader(FileSourceImporter::Frame(sourceFile));
	if(!importer->dataset()->taskManager().waitForFuture(inspectFuture))
		return false;
	InputColumnMapping mapping = inspectFuture.result();

	// If column names were given in the XYZ file, use them rather than popping up a dialog.
	if(mapping.hasFileColumnNames()) {
		return true;
	}

	// If this is a newly created file importer, load old mapping from application settings store.
	if(xyzImporter->columnMapping().empty()) {
		QSettings settings;
		settings.beginGroup("viz/importer/xyz/");
		if(settings.contains("columnmapping")) {
			try {
				InputColumnMapping storedMapping;
				storedMapping.fromByteArray(settings.value("columnmapping").toByteArray());
				std::copy_n(storedMapping.begin(), std::min(storedMapping.size(), mapping.size()), mapping.begin());
			}
			catch(Exception& ex) {
				ex.prependGeneralMessage(tr("Failed to load last used column-to-property mapping from application settings store."));
				ex.logError();
			}
			for(auto& column : mapping)
				column.columnName.clear();
		}
	}

	InputColumnMappingDialog dialog(mapping, parent);
	if(dialog.exec() == QDialog::Accepted) {
		xyzImporter->setColumnMapping(dialog.mapping());

		// Remember the user-defined mapping for the next time.
		QSettings settings;
		settings.beginGroup("viz/importer/xyz/");
		settings.setValue("columnmapping", dialog.mapping().toByteArray());
		settings.endGroup();

		return true;
	}

	return false;
}

/******************************************************************************
 * Displays a dialog box that allows the user to edit the custom file column to particle
 * property mapping.
 *****************************************************************************/
bool XYZImporterEditor::showEditColumnMappingDialog(XYZImporter* importer, const QUrl& sourceFile, QWidget* parent)
{
	Future<InputColumnMapping> inspectFuture = importer->inspectFileHeader(FileSourceImporter::Frame(sourceFile));
	if(!importer->dataset()->taskManager().waitForFuture(inspectFuture))
		return false;
	InputColumnMapping mapping = inspectFuture.result();

	if(!importer->columnMapping().empty()) {
		InputColumnMapping customMapping = importer->columnMapping();
		customMapping.resize(mapping.size());
		for(size_t i = 0; i < customMapping.size(); i++)
			customMapping[i].columnName = mapping[i].columnName;
		mapping = customMapping;
	}

	InputColumnMappingDialog dialog(mapping, parent);
	if(dialog.exec() == QDialog::Accepted) {
		importer->setColumnMapping(dialog.mapping());
		// Remember the user-defined mapping for the next time.
		QSettings settings;
		settings.beginGroup("viz/importer/xyz/");
		settings.setValue("columnmapping", dialog.mapping().toByteArray());
		settings.endGroup();
		return true;
	}
	else {
		return false;
	}
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void XYZImporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("XYZ reader"), rolloutParams);

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

	// Auto-rescale reduced coordinates.
	BooleanParameterUI* rescaleReducedUI = new BooleanParameterUI(this, PROPERTY_FIELD(XYZImporter::autoRescaleCoordinates));
	sublayout->addWidget(rescaleReducedUI->checkBox());

	// Sort particles
	BooleanParameterUI* sortParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(ParticleImporter::sortParticles));
	sublayout->addWidget(sortParticlesUI->checkBox());

	QGroupBox* columnMappingBox = new QGroupBox(tr("File columns"), rollout);
	sublayout = new QVBoxLayout(columnMappingBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(columnMappingBox);

	QPushButton* editMappingButton = new QPushButton(tr("Edit column mapping..."));
	sublayout->addWidget(editMappingButton);
	connect(editMappingButton, &QPushButton::clicked, this, &XYZImporterEditor::onEditColumnMapping);
}

/******************************************************************************
* Is called when the user pressed the "Edit column mapping" button.
******************************************************************************/
void XYZImporterEditor::onEditColumnMapping()
{
	if(XYZImporter* importer = static_object_cast<XYZImporter>(editObject())) {

		// Determine URL of current input file.
		FileSource* fileSource = nullptr;
		for(RefMaker* refmaker : importer->dependents()) {
			fileSource = dynamic_object_cast<FileSource>(refmaker);
			if(fileSource) break;
		}
		if(!fileSource || fileSource->frames().empty()) return;

		QUrl sourceUrl;
		if(fileSource->storedFrameIndex() >= 0)
			sourceUrl = fileSource->frames()[fileSource->storedFrameIndex()].sourceFile;
		else
			sourceUrl = fileSource->frames().front().sourceFile;

		UndoableTransaction::handleExceptions(importer->dataset()->undoStack(), tr("Change file column mapping"), [this, &sourceUrl, importer]() {
			if(showEditColumnMappingDialog(importer, sourceUrl, mainWindow())) {
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
