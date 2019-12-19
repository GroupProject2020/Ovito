////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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

#include <ovito/gui/GUI.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include <ovito/gui/properties/BooleanActionParameterUI.h>
#include <ovito/gui/properties/IntegerParameterUI.h>
#include <ovito/gui/properties/SubObjectParameterUI.h>
#include <ovito/gui/dialogs/ImportFileDialog.h>
#include <ovito/gui/dialogs/ImportRemoteFileDialog.h>
#include <ovito/gui/dataset/io/FileImporterEditor.h>
#include <ovito/gui/mainwin/MainWindow.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/io/FileSource.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/app/PluginManager.h>
#include "FileSourceEditor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(FileSourceEditor);
SET_OVITO_OBJECT_EDITOR(FileSource, FileSourceEditor);

/******************************************************************************
* Sets up the UI of the editor.
******************************************************************************/
void FileSourceEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("External file"), rolloutParams, "data_sources.html");

	// Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QVBoxLayout* sublayout;

	QToolBar* toolbar = new QToolBar(rollout);
	toolbar->setStyleSheet("QToolBar { padding: 0px; margin: 0px; border: 0px none black; }");
	layout->addWidget(toolbar);

	toolbar->addAction(QIcon(":/gui/actions/file/import_object_changefile.bw.svg"), tr("Pick new file"), this, SLOT(onPickLocalInputFile()));
	toolbar->addAction(QIcon(":/gui/actions/file/file_import_remote.bw.svg"), tr("Pick new remote file"), this, SLOT(onPickRemoteInputFile()));
	toolbar->addAction(QIcon(":/gui/actions/file/import_object_reload.bw.svg"), tr("Reload data from external file"), this, SLOT(onReloadFrame()));
	toolbar->addAction(QIcon(":/gui/actions/file/import_object_refresh_animation.bw.svg"), tr("Update time series"), this, SLOT(onReloadAnimation()));

	QGroupBox* sourceBox = new QGroupBox(tr("Data source"), rollout);
	layout->addWidget(sourceBox);
	QGridLayout* gridlayout = new QGridLayout(sourceBox);
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1,1);
	gridlayout->setVerticalSpacing(2);
	gridlayout->setHorizontalSpacing(6);
	_filenameLabel = new QLineEdit();
	_filenameLabel->setReadOnly(true);
	_filenameLabel->setFrame(false);
	gridlayout->addWidget(new QLabel(tr("Current file:")), 0, 0);
	gridlayout->addWidget(_filenameLabel, 0, 1);
	_sourcePathLabel = new QLineEdit();
	_sourcePathLabel->setReadOnly(true);
	_sourcePathLabel->setFrame(false);
	gridlayout->addWidget(new QLabel(tr("Directory:")), 1, 0);
	gridlayout->addWidget(_sourcePathLabel, 1, 1);

	QGroupBox* wildcardBox = new QGroupBox(tr("Time series"), rollout);
	layout->addWidget(wildcardBox);
	gridlayout = new QGridLayout(wildcardBox);
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setVerticalSpacing(2);
	gridlayout->setColumnStretch(1, 1);
	_wildcardPatternTextbox = new QLineEdit();
	connect(_wildcardPatternTextbox, &QLineEdit::returnPressed, this, &FileSourceEditor::onWildcardPatternEntered);
	gridlayout->addWidget(new QLabel(tr("File pattern:")), 0, 0);
	gridlayout->addWidget(_wildcardPatternTextbox, 0, 1);
	_fileSeriesLabel = new QLabel();
	QFont smallFont = _fileSeriesLabel->font();
#ifdef Q_OS_MAC
	smallFont.setPointSize(std::max(6, smallFont.pointSize() - 3));
#elif defined(Q_OS_LINUX)
	smallFont.setPointSize(std::max(6, smallFont.pointSize() - 2));
#else
	smallFont.setPointSize(std::max(6, smallFont.pointSize() - 1));
#endif
	_fileSeriesLabel->setFont(smallFont);
	gridlayout->addWidget(_fileSeriesLabel, 1, 1);

	if(!parentEditor()) {
		gridlayout->addWidget(new QLabel(tr("Current frame:")), 2, 0);
		_framesListBox = new QComboBox();
		_framesListBox->setEditable(false);
		_framesListBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
		connect(_framesListBox, (void (QComboBox::*)(int))&QComboBox::activated, this, &FileSourceEditor::onFrameSelected);
		gridlayout->addWidget(_framesListBox, 2, 1);
		_timeSeriesLabel = new QLabel();
		_timeSeriesLabel->setFont(smallFont);
		gridlayout->addWidget(_timeSeriesLabel, 3, 1);
	}

	QGroupBox* statusBox = new QGroupBox(tr("Status"), rollout);
	layout->addWidget(statusBox);
	sublayout = new QVBoxLayout(statusBox);
	sublayout->setContentsMargins(4,4,4,4);
	_statusLabel = new StatusWidget(rollout);
	sublayout->addWidget(_statusLabel);

	// Create another rollout for animation control.
	rollout = createRollout(tr("Animation"), rolloutParams.after(rollout).collapse(), "data_sources.html");

	// Create the rollout contents.
	layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QHBoxLayout* subsublayout = new QHBoxLayout();
	subsublayout->setContentsMargins(0,0,0,0);
	subsublayout->setSpacing(2);
	IntegerParameterUI* playbackSpeedNumeratorUI = new IntegerParameterUI(this, PROPERTY_FIELD(FileSource::playbackSpeedNumerator));
	subsublayout->addWidget(new QLabel(tr("Playback rate:")));
	subsublayout->addLayout(playbackSpeedNumeratorUI->createFieldLayout());
	subsublayout->addWidget(new QLabel(tr("/")));
	IntegerParameterUI* playbackSpeedDenominatorUI = new IntegerParameterUI(this, PROPERTY_FIELD(FileSource::playbackSpeedDenominator));
	subsublayout->addLayout(playbackSpeedDenominatorUI->createFieldLayout());
	layout->addLayout(subsublayout);

	subsublayout = new QHBoxLayout();
	subsublayout->setContentsMargins(0,0,0,0);
	IntegerParameterUI* playbackStartUI = new IntegerParameterUI(this, PROPERTY_FIELD(FileSource::playbackStartTime));
	subsublayout->addWidget(new QLabel(tr("Start at animation frame:")));
	subsublayout->addLayout(playbackStartUI->createFieldLayout());
	layout->addLayout(subsublayout);

	// Show settings editor of importer class.
	new SubObjectParameterUI(this, PROPERTY_FIELD(FileSource::importer), rolloutParams.after(rollout));
}

/******************************************************************************
* Is called when a new object has been loaded into the editor.
******************************************************************************/
void FileSourceEditor::onEditorContentsReplaced(RefTarget* newObject)
{
	updateInformationLabel();
}

/******************************************************************************
* Is called when the user presses the "Pick local input file" button.
******************************************************************************/
void FileSourceEditor::onPickLocalInputFile()
{
	FileSource* fileSource = static_object_cast<FileSource>(editObject());
	if(!fileSource) return;

	try {
		QUrl newSourceUrl;
		OvitoClassPtr importerType;

		// Put code in a block: Need to release dialog before loading new input file.
		{
			// Offer only file importer types that are compatible with a FileSource.
			auto importerClasses = PluginManager::instance().metaclassMembers<FileImporter>(FileSourceImporter::OOClass());

			// Let the user select a file.
			ImportFileDialog dialog(importerClasses, dataset(), container()->window(), tr("Pick input file"));
			if(!fileSource->sourceUrls().empty() && fileSource->sourceUrls().front().isLocalFile())
				dialog.selectFile(fileSource->sourceUrls().front().toLocalFile());
			if(dialog.exec() != QDialog::Accepted)
				return;

			newSourceUrl = QUrl::fromLocalFile(dialog.fileToImport());
			importerType = dialog.selectedFileImporterType();
		}

		// Set the new input location.
		importNewFile(fileSource, newSourceUrl, importerType);
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

/******************************************************************************
* Is called when the user presses the "Pick remote input file" button.
******************************************************************************/
void FileSourceEditor::onPickRemoteInputFile()
{
	FileSource* fileSource = static_object_cast<FileSource>(editObject());
	if(!fileSource) return;

	try {
		QUrl newSourceUrl;
		OvitoClassPtr importerType;

		// Put code in a block: Need to release dialog before loading new input file.
		{
			// Offer only file importer types that are compatible with a FileSource.
			auto importerClasses = PluginManager::instance().metaclassMembers<FileImporter>(FileSourceImporter::OOClass());

			// Let the user select a new URL.
			ImportRemoteFileDialog dialog(importerClasses, dataset(), container()->window(), tr("Pick source"));
			QUrl oldUrl;
			if(fileSource->storedFrameIndex() >= 0 && fileSource->storedFrameIndex() < fileSource->frames().size())
				oldUrl = fileSource->frames()[fileSource->storedFrameIndex()].sourceFile;
			else if(!fileSource->sourceUrls().empty())
				oldUrl = fileSource->sourceUrls().front();
			dialog.selectFile(oldUrl);
			if(dialog.exec() != QDialog::Accepted)
				return;

			newSourceUrl = dialog.fileToImport();
			importerType = dialog.selectedFileImporterType();
		}

		// Set the new input location.
		importNewFile(fileSource, newSourceUrl, importerType);
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

/******************************************************************************
* Loads a new file into the FileSource.
******************************************************************************/
bool FileSourceEditor::importNewFile(FileSource* fileSource, const QUrl& url, OvitoClassPtr importerType)
{
	OORef<FileImporter> fileimporter;

	// Create file importer instance.
	if(!importerType) {

		// Download file so we can determine its format.
		TaskManager& taskManager = fileSource->dataset()->taskManager();
		SharedFuture<QString> fetchFileFuture = Application::instance()->fileManager()->fetchUrl(taskManager, url);
		if(!fileSource->dataset()->taskManager().waitForFuture(fetchFileFuture))
			return false;

		// Inspect file to detect its format.
		fileimporter = FileImporter::autodetectFileFormat(fileSource->dataset(), fetchFileFuture.result(), url.path());
		if(!fileimporter)
			fileSource->throwException(tr("Could not detect the format of the file to be imported. The format might not be supported."));
	}
	else {
		// Caller has provided a specific importer type.
		fileimporter = static_object_cast<FileImporter>(importerType->createInstance(fileSource->dataset()));
		if(!fileimporter)
			return false;
	}

	// The importer must be a FileSourceImporter.
	OORef<FileSourceImporter> newImporter = dynamic_object_cast<FileSourceImporter>(fileimporter);
	if(!newImporter)
		fileSource->throwException(tr("The selected file type is not compatible."));

	// Temporarily suppress viewport updates while setting up the newly imported data.
	ViewportSuspender noVPUpdate(fileSource->dataset()->viewportConfig());

	// Load user-defined default import settings.
	newImporter->loadUserDefaults();

	// Show the optional user interface (which is provided by the corresponding FileImporterEditor class) for the new importer.
	for(OvitoClassPtr clazz = &newImporter->getOOClass(); clazz != nullptr; clazz = clazz->superClass()) {
		OvitoClassPtr editorClass = PropertiesEditor::registry().getEditorClass(clazz);
		if(editorClass && editorClass->isDerivedFrom(FileImporterEditor::OOClass())) {
			OORef<FileImporterEditor> editor = dynamic_object_cast<FileImporterEditor>(editorClass->createInstance(nullptr));
			if(editor) {
				if(!editor->inspectNewFile(newImporter, url, mainWindow()))
					return false;
			}
		}
	}

	// Set the new input location.
	return fileSource->setSource({url}, newImporter, false);
}

/******************************************************************************
* Is called when the user presses the Reload frame button.
******************************************************************************/
void FileSourceEditor::onReloadFrame()
{
	if(FileSource* fileSource = static_object_cast<FileSource>(editObject())) {
		fileSource->reloadFrame(fileSource->storedFrameIndex());
	}
}

/******************************************************************************
* Is called when the user presses the Reload animation button.
******************************************************************************/
void FileSourceEditor::onReloadAnimation()
{
	if(FileSource* fileSource = static_object_cast<FileSource>(editObject())) {
		fileSource->updateListOfFrames();
	}
}

/******************************************************************************
* This is called when the user has changed the source URL.
******************************************************************************/
void FileSourceEditor::onWildcardPatternEntered()
{
	FileSource* fileSource = static_object_cast<FileSource>(editObject());
	OVITO_CHECK_OBJECT_POINTER(fileSource);

	undoableTransaction(tr("Change wildcard pattern"), [this, fileSource]() {
		if(!fileSource->importer())
			return;

		QString pattern = _wildcardPatternTextbox->text().trimmed();
		if(pattern.isEmpty())
			return;

		QUrl newUrl;
		if(!fileSource->sourceUrls().empty()) newUrl = fileSource->sourceUrls().front();
		QFileInfo fileInfo(newUrl.path());
		fileInfo.setFile(fileInfo.dir(), pattern);
		newUrl.setPath(fileInfo.filePath());
		if(!newUrl.isValid())
			throwException(tr("URL is not valid."));

		fileSource->setSource({newUrl}, fileSource->importer(), false);
	});
	updateInformationLabel();
}

/******************************************************************************
* Updates the displayed status informations.
******************************************************************************/
void FileSourceEditor::updateInformationLabel()
{
	FileSource* fileSource = static_object_cast<FileSource>(editObject());
	if(!fileSource) {
		_wildcardPatternTextbox->clear();
		_wildcardPatternTextbox->setEnabled(false);
		_sourcePathLabel->setText(QString());
		_filenameLabel->setText(QString());
		_statusLabel->clearStatus();
		if(_framesListBox) {
			_framesListBox->clear();
			_framesListBox->setEnabled(false);
		}
		if(_fileSeriesLabel)
			_fileSeriesLabel->setText(QString());
		return;
	}

	QString wildcardPattern;
	if(!fileSource->sourceUrls().empty()) {
		if(fileSource->sourceUrls().front().isLocalFile()) {
			QFileInfo fileInfo(fileSource->sourceUrls().front().toLocalFile());
			_sourcePathLabel->setText(fileInfo.dir().path());
			wildcardPattern = fileInfo.fileName();
		}
		else {
			QFileInfo fileInfo(fileSource->sourceUrls().front().path());
			QUrl url = fileSource->sourceUrls().front();
			url.setPath(fileInfo.path());
			_sourcePathLabel->setText(url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded));
			wildcardPattern = fileInfo.fileName();
		}
	}

	_wildcardPatternTextbox->setText(wildcardPattern);
	_wildcardPatternTextbox->setEnabled(true);

	int frameIndex = fileSource->storedFrameIndex();
	if(frameIndex >= 0) {
		const FileSourceImporter::Frame& frameInfo = fileSource->frames()[frameIndex];
		if(frameInfo.sourceFile.isLocalFile()) {
			_filenameLabel->setText(QFileInfo(frameInfo.sourceFile.toLocalFile()).fileName());
		}
		else {
			_filenameLabel->setText(QFileInfo(frameInfo.sourceFile.path()).fileName());
		}
	}
	else {
		_filenameLabel->setText(QString());
	}

	// Count the number of files matching the wildcard pattern.
	int fileSeriesCount = 0;
	QUrl lastUrl;
	for(const FileSourceImporter::Frame& frame : fileSource->frames()) {
		if(frame.sourceFile != lastUrl) {
			fileSeriesCount++;
			lastUrl = frame.sourceFile;
		}
	}
	if(fileSeriesCount == 0)
		_fileSeriesLabel->setText(tr("Found no matching file"));
	else if(fileSeriesCount == 1)
		_fileSeriesLabel->setText(tr("Found %1 matching file").arg(fileSeriesCount));
	else
		_fileSeriesLabel->setText(tr("Found %1 matching files").arg(fileSeriesCount));

	if(_timeSeriesLabel) {
		if(!fileSource->frames().empty())
			_timeSeriesLabel->setText(tr("Showing frame %1 of %2").arg(fileSource->storedFrameIndex()+1).arg(fileSource->frames().count()));
		else
			_timeSeriesLabel->setText(tr("No frames available"));
	}

	if(_framesListBox) {
		for(int index = 0; index < fileSource->frames().size(); index++) {
			if(_framesListBox->count() <= index) {
				_framesListBox->addItem(fileSource->frames()[index].label);
			}
			else {
				if(_framesListBox->itemText(index) != fileSource->frames()[index].label)
					_framesListBox->setItemText(index, fileSource->frames()[index].label);
			}
		}
		for(int index = _framesListBox->count() - 1; index >= fileSource->frames().size(); index--) {
			_framesListBox->removeItem(index);
		}
		_framesListBox->setCurrentIndex(frameIndex);
		_framesListBox->setEnabled(_framesListBox->count() > 1);
	}

	_statusLabel->setStatus(fileSource->status());
}

/******************************************************************************
* Is called when the user has selected a certain frame in the frame list box.
******************************************************************************/
void FileSourceEditor::onFrameSelected(int index)
{
	FileSource* fileSource = static_object_cast<FileSource>(editObject());
	if(!fileSource) return;

	dataset()->animationSettings()->setTime(fileSource->sourceFrameToAnimationTime(index));
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool FileSourceEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject()) {
		if(event.type() == ReferenceEvent::ObjectStatusChanged || event.type() == ReferenceEvent::TitleChanged) {
			updateInformationLabel();
		}
	}
	return PropertiesEditor::referenceEvent(source, event);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
