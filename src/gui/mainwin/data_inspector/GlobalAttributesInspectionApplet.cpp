///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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
#include <core/dataset/pipeline/PipelineFlowState.h>
#include <core/dataset/data/AttributeDataObject.h>
#include <core/dataset/io/AttributeFileExporter.h>
#include <gui/dialogs/FileExporterSettingsDialog.h>
#include <gui/dialogs/HistoryFileDialog.h>
#include <gui/utilities/concurrent/ProgressDialog.h>
#include <gui/mainwin/MainWindow.h>
#include "GlobalAttributesInspectionApplet.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

IMPLEMENT_OVITO_CLASS(GlobalAttributesInspectionApplet);

/******************************************************************************
* Determines whether the given pipeline dataset contains data that can be
* displayed by this applet.
******************************************************************************/
bool GlobalAttributesInspectionApplet::appliesTo(const DataCollection& data)
{
	return data.containsObject<AttributeDataObject>();
}

/******************************************************************************
* Lets the applet create the UI widget that is to be placed into the data
* inspector panel.
******************************************************************************/
QWidget* GlobalAttributesInspectionApplet::createWidget(MainWindow* mainWindow)
{
	_mainWindow = mainWindow;

	QWidget* panel = new QWidget();
	QHBoxLayout* layout = new QHBoxLayout(panel);
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(0);

	QToolBar* toolbar = new QToolBar();
	toolbar->setOrientation(Qt::Vertical);
	toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	toolbar->setIconSize(QSize(22,22));
	toolbar->setStyleSheet("QToolBar { padding: 0px; margin: 0px; border: 0px none black; spacing: 0px; }");

	QAction* exportToFileAction = new QAction(QIcon(":/gui/actions/file/file_save_as.bw.svg"), tr("Export attributes to text file"), this);
	connect(exportToFileAction, &QAction::triggered, this, &GlobalAttributesInspectionApplet::exportToFile);
	toolbar->addAction(exportToFileAction);

	_tableView = new TableView();
	_tableModel = new AttributeTableModel(_tableView);
	_tableView->setModel(_tableModel);
	_tableView->verticalHeader()->hide();
	_tableView->horizontalHeader()->resizeSection(0, 180);
	_tableView->horizontalHeader()->setStretchLastSection(true);

	layout->addWidget(_tableView, 1);
	layout->addWidget(toolbar, 0);

	return panel;
}

/******************************************************************************
* Updates the contents displayed in the inspector.
******************************************************************************/
void GlobalAttributesInspectionApplet::updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode)
{
	_sceneNode = sceneNode;
	_tableModel->setContents(state.data());
}

/******************************************************************************
* Exports the global attributes to a text file.
******************************************************************************/
void GlobalAttributesInspectionApplet::exportToFile()
{
	if(!_sceneNode)
		return;

	// Let the user select a destination file.
	HistoryFileDialog dialog("export", _mainWindow, tr("Export Attributes"));
#ifndef Q_OS_WIN
	QString filterString = QStringLiteral("%1 (%2)").arg(AttributeFileExporter::OOClass().fileFilterDescription(), AttributeFileExporter::OOClass().fileFilter());
#else
		// Workaround for bug in Windows file selection dialog (https://bugreports.qt.io/browse/QTBUG-45759)
	QString filterString = QStringLiteral("%1 (*)").arg(AttributeFileExporter::OOClass().fileFilterDescription());
#endif
	dialog.setNameFilter(filterString);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setConfirmOverwrite(true);

	// Go to the last directory used.
	QSettings settings;
	settings.beginGroup("file/export");
	QString lastExportDirectory = settings.value("last_export_dir").toString();
	if(!lastExportDirectory.isEmpty())
		dialog.setDirectory(lastExportDirectory);

	if(!dialog.exec() || dialog.selectedFiles().empty())
		return;
	QString exportFile = dialog.selectedFiles().front();

	// Remember directory for the next time...
	settings.setValue("last_export_dir", dialog.directory().absolutePath());

	// Export to selected file.
	try {
		// Create exporter service.
		OORef<AttributeFileExporter> exporter = new AttributeFileExporter(_sceneNode->dataset());

		// Load user-defined default settings.
		exporter->loadUserDefaults();

		// Pass output filename to exporter.
		exporter->setOutputFilename(exportFile);

		// Set scene node to be exported.
		exporter->setNodeToExport(_sceneNode);

		// Let the user adjust the export settings.
		FileExporterSettingsDialog settingsDialog(_mainWindow, exporter);
		if(settingsDialog.exec() != QDialog::Accepted)
			return;

		// Show progress dialog.
		ProgressDialog progressDialog(_mainWindow, tr("File export"));

		// Let the exporter do its job.
		exporter->doExport(progressDialog.taskManager().createMainThreadOperation<>(true));
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
