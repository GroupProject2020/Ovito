///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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

#include <core/Core.h>
#include <core/dataset/scene/PipelineSceneNode.h>
#include <core/dataset/scene/RootSceneNode.h>
#include <core/dataset/data/DataObject.h>
#include <core/dataset/scene/SelectionSet.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/utilities/io/FileManager.h>
#include <core/app/Application.h>
#include "FileSourceImporter.h"
#include "FileSource.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

IMPLEMENT_OVITO_CLASS(FileSourceImporter);

/******************************************************************************
* Sends a request to the FileSource owning this importer to reload
* the input file.
******************************************************************************/
void FileSourceImporter::requestReload(int frame)
{
	// Retrieve the FileSource that owns this importer by looking it up in the list of dependents.
	for(RefMaker* refmaker : dependents()) {
		if(FileSource* fileSource = dynamic_object_cast<FileSource>(refmaker)) {
			try {
				fileSource->reloadFrame(frame);
			}
			catch(const Exception& ex) {
				ex.reportError();
			}
		}
	}
}

/******************************************************************************
* Sends a request to the FileSource owning this importer to refresh the
* animation frame sequence.
******************************************************************************/
void FileSourceImporter::requestFramesUpdate()
{
	// Retrieve the FileSource that owns this importer by looking it up in the list of dependents.
	for(RefMaker* refmaker : dependents()) {
		if(FileSource* fileSource = dynamic_object_cast<FileSource>(refmaker)) {
			try {
#if 0
				// If wildcard pattern search has been disabled, replace
				// wildcard pattern URLs with an actual filename.
				if(!autoGenerateWildcardPattern()) {
					if(std::any_of(fileSource->sourceUrls().begin(), fileSource->sourceUrls().end(), [](const QUrl& url) { 
							return isWildcardPattern(url);
						})) {
						std::vector<QUrl> urls = fileSource->sourceUrls();
						for(QUrl& url : urls) {
							QFileInfo fileInfo(fileSource->sourceUrl().path());
							if(isWildcardPattern(url)) {
								if(fileSource->storedFrameIndex() >= 0 && fileSource->storedFrameIndex() < fileSource->frames().size()) {
									QUrl currentUrl = fileSource->frames()[fileSource->storedFrameIndex()].sourceFile;
									if(currentUrl != fileSource->sourceUrl()) {
										fileSource->setSource(currentUrl, this, true);
										continue;
									}
								}
							}
						}
						fileSource->setSource(std::move(urls), this, true);
						continue;
					}
				}
#endif

				// Scan input source for animation frames.
				fileSource->updateListOfFrames();
			}
			catch(const Exception& ex) {
				ex.reportError();
			}
		}
	}
}

/******************************************************************************
* Determines if the option to replace the currently selected object
* with the new file is available.
******************************************************************************/
bool FileSourceImporter::isReplaceExistingPossible(const QUrl& sourceUrl)
{
	// Look for an existing FileSource in the scene whose
	// data source we can replace with the new file.
	for(SceneNode* node : dataset()->selection()->nodes()) {
		if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(node)) {
			if(dynamic_object_cast<FileSource>(pipeline->pipelineSource()))
				return true;
		}
	}
	return false;
}

/******************************************************************************
* Imports the given file into the scene.
* Return true if the file has been imported.
* Return false if the import has been aborted by the user.
* Throws an exception when the import has failed.
******************************************************************************/
bool FileSourceImporter::importFile(std::vector<QUrl> sourceUrls, ImportMode importMode, bool autodetectFileSequences)
{
	OORef<FileSource> existingFileSource;
	PipelineSceneNode* existingPipeline = nullptr;

	if(importMode == ReplaceSelected) {
		// Look for an existing FileSource in the scene whose
		// data source we can replace with the newly imported file.
		for(SceneNode* node : dataset()->selection()->nodes()) {
			if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(node)) {
				existingFileSource = dynamic_object_cast<FileSource>(pipeline->pipelineSource());
				if(existingFileSource) {
					existingPipeline = pipeline;
					break;
				}
			}
		}
	}
	else if(importMode == ResetScene) {
		dataset()->clearScene();
		if(!dataset()->undoStack().isRecording())
			dataset()->undoStack().clear();
		dataset()->setFilePath(QString());
	}
	else {
		if(dataset()->sceneRoot()->children().empty())
			importMode = ResetScene;
	}

	QString filename;
	if(!sourceUrls.empty()) filename = QFileInfo(sourceUrls.front().path()).fileName();
	UndoableTransaction transaction(dataset()->undoStack(), tr("Import"));

	// Do not create any animation keys during import.
	AnimationSuspender animSuspender(this);

	OORef<FileSource> fileSource;

	// Create the object that will insert the imported data into the scene.
	if(existingFileSource == nullptr) {
		fileSource = new FileSource(dataset());

		// When adding the imported data to an existing scene,
		// do not auto-adjust animation interval.
		if(importMode == AddToScene)
			fileSource->setAdjustAnimationIntervalEnabled(false);
	}
	else {
		fileSource = existingFileSource;
	}

	// Set the input location and importer.
	if(!fileSource->setSource(std::move(sourceUrls), this, autodetectFileSequences)) {
		return false;
	}

	// Create a new object node in the scene for the linked data.
	RootSceneNode* scene = dataset()->sceneRoot();
	OORef<PipelineSceneNode> pipeline;
	if(existingPipeline == nullptr) {
		{
			UndoSuspender unsoSuspender(this);	// Do not create undo records for this part.

			// Add object to scene.
			pipeline = new PipelineSceneNode(dataset());
			pipeline->setDataProvider(fileSource);

			// Let the importer subclass customize the pipeline scene node.
			setupPipeline(pipeline, fileSource);
		}

		// Insert pipeline into scene.
		scene->addChildNode(pipeline);
	}
	else pipeline = existingPipeline;

	// Select import node.
	dataset()->selection()->setNode(pipeline);

	if(importMode != ReplaceSelected) {
		// Adjust viewports to completely show the newly imported object.
		// This needs to be done after the data has been completely loaded.
		dataset()->whenSceneReady().finally(dataset()->executor(), [dataset = dataset()]() {
			if(dataset->viewportConfig()) 
				dataset->viewportConfig()->zoomToSelectionExtents();
		});
	}

	transaction.commit();
	return true;
}

/******************************************************************************
* Determines whether the URL contains a wildcard pattern.
******************************************************************************/
bool FileSourceImporter::isWildcardPattern(const QUrl& sourceUrl)
{
	return QFileInfo(sourceUrl.path()).fileName().contains('*');
}

/******************************************************************************
* Scans the given external path(s) (which may be a directory and a wild-card pattern,
* or a single file containing multiple frames) to find all available animation frames.
******************************************************************************/
Future<QVector<FileSourceImporter::Frame>> FileSourceImporter::discoverFrames(const std::vector<QUrl>& sourceUrls) 
{
	// No output if there is no input.
	if(sourceUrls.empty()) 
		return QVector<Frame>();
	
	// If there is only a single input path, call sub-routine handling single paths.
	if(sourceUrls.size() == 1)
		return discoverFrames(sourceUrls.front());
	
	// Sequentually invoke single-path routine for each input path and compile results
	// into one big list that is returned to the caller.
	auto combinedList = std::make_shared<QVector<Frame>>();
	Future<QVector<Frame>> future;
	for(const QUrl& url : sourceUrls) {
		if(!future.isValid()) {
			future = discoverFrames(url);
		}
		else {
			future = future.then(executor(), [this, combinedList, url](const QVector<Frame>& frames) {
				*combinedList += frames;
				return discoverFrames(url);
			});
		}
	}
	return future.then([combinedList](const QVector<Frame>& frames) {
		*combinedList += frames;
		return std::move(*combinedList);
	});
}


/******************************************************************************
* Scans the given external path (which may be a directory and a wild-card pattern,
* or a single file containing multiple frames) to find all available animation frames.
******************************************************************************/
Future<QVector<FileSourceImporter::Frame>> FileSourceImporter::discoverFrames(const QUrl& sourceUrl) 
{
	if(shouldScanFileForFrames(sourceUrl)) {

		// Check if filename is a wildcard pattern.
		// If yes, find all matching files and scan each one of them.
		if(isWildcardPattern(sourceUrl)) {
			return findWildcardMatches(sourceUrl, dataset())
				.then(executor(), [this](const std::vector<QUrl>& fileList) {
					return discoverFrames(fileList);
				});
		}

		// Fetch file.
		return Application::instance()->fileManager()->fetchUrl(dataset()->taskManager(), sourceUrl)
			.then(executor(), [this, sourceUrl](const QString& filename) {
				// Scan file.
				if(FrameFinderPtr frameFinder = createFrameFinder(sourceUrl, filename))
					return dataset()->taskManager().runTaskAsync(frameFinder);
				else
					return Future<QVector<Frame>>::createImmediateEmplace();
			});
	}
	else {
		if(isWildcardPattern(sourceUrl)) {
			// Find all files matching the file pattern.
			return findWildcardMatches(sourceUrl, dataset())
				.then(executor(), [](const std::vector<QUrl>& fileList) {
					// Turn the file list into a frame list.
					QVector<Frame> frames;
					frames.reserve(fileList.size());
					for(const QUrl& url : fileList) {
						QFileInfo fileInfo(url.path());
						QDateTime dateTime = url.isLocalFile() ? fileInfo.lastModified() : QDateTime();
						frames.push_back(Frame(url, 0, 1, dateTime, fileInfo.fileName()));
					}
					return frames;
				});
		}
		else {
			// Build just a single frame from the source URL.
			QFileInfo fileInfo(sourceUrl.path());
			QDateTime dateTime = sourceUrl.isLocalFile() ? fileInfo.lastModified() : QDateTime();
			return QVector<Frame>{{ Frame(sourceUrl, 0, 1, dateTime, fileInfo.fileName()) }};
		}
	}
}

/******************************************************************************
* Scans the source URL for input frames.
******************************************************************************/
void FileSourceImporter::FrameFinder::perform()
{
	QVector<Frame> frameList;

	// Scan file.
	try {
		QFile file(_localFilename);
		discoverFramesInFile(file, _sourceUrl, frameList);
	}
	catch(const Exception&) {
		// Silently ignore parsing and I/O errors if at least two frames have been read.
		// Keep all frames read up to where the error occurred.
		if(frameList.size() <= 1)
			throw;
		else
			frameList.pop_back();		// Remove last discovered frame because it may be corrupted or only partially written.
	}

	setResult(std::move(frameList));
}

/******************************************************************************
* Scans the given file for source frames
******************************************************************************/
void FileSourceImporter::FrameFinder::discoverFramesInFile(QFile& file, const QUrl& sourceUrl, QVector<FileSourceImporter::Frame>& frames)
{
	// By default, register a single frame.
	QFileInfo fileInfo(file.fileName());
	frames.push_back({ sourceUrl, 0, 1, fileInfo.lastModified(), fileInfo.fileName() });
}

/******************************************************************************
* Returns the list of files that match the given wildcard pattern.
******************************************************************************/
Future<std::vector<QUrl>> FileSourceImporter::findWildcardMatches(const QUrl& sourceUrl, DataSet* dataset)
{
	// Determine whether the filename contains a wildcard character.
	if(!isWildcardPattern(sourceUrl)) {

		// It's not a wildcard pattern. Register just a single frame.
		return std::vector<QUrl>{ sourceUrl };
	}
	else {
		QFileInfo fileInfo(sourceUrl.path());
		QString pattern = fileInfo.fileName();

		QDir directory;
		bool isLocalPath = false;
		Future<QStringList> entriesFuture;

		// Scan the directory for files matching the wildcard pattern.
		if(sourceUrl.isLocalFile()) {

			QStringList entries;
			isLocalPath = true;
			directory = QFileInfo(sourceUrl.toLocalFile()).dir();
			for(const QString& filename : directory.entryList(QDir::Files|QDir::NoDotAndDotDot|QDir::Hidden, QDir::Name)) {
				if(matchesWildcardPattern(pattern, filename))
					entries << filename;
			}
			entriesFuture = Future<QStringList>::createImmediate(std::move(entries));
		}
		else {

			directory = fileInfo.dir();
			QUrl directoryUrl = sourceUrl;
			directoryUrl.setPath(fileInfo.path());

			// Retrieve list of files in remote directory.
			Future<QStringList> remoteFileListFuture = Application::instance()->fileManager()->listDirectoryContents(dataset->taskManager(), directoryUrl);

			// Filter file names.
			entriesFuture = remoteFileListFuture.then([pattern](QStringList&& remoteFileList) {
				QStringList entries;
				for(const QString& filename : remoteFileList) {
					if(matchesWildcardPattern(pattern, filename))
						entries << filename;
				}
				return entries;
			});
		}

		// Sort the file list.
		return entriesFuture.then([isLocalPath, sourceUrl, directory](QStringList&& entries) {

			// A file called "abc9.xyz" must come before a file named "abc10.xyz", which is not
			// the default lexicographic ordering.
			QMap<QString, QString> sortedFilenames;
			for(const QString& oldName : entries) {
				// Generate a new name from the original filename that yields the correct ordering.
				QString newName;
				QString number;
				for(QChar c : oldName) {
					if(!c.isDigit()) {
						if(!number.isEmpty()) {
							newName.append(number.rightJustified(10, '0'));
							number.clear();
						}
						newName.append(c);
					}
					else number.append(c);
				}
				if(!number.isEmpty()) newName.append(number.rightJustified(10, '0'));
				sortedFilenames[newName] = oldName;
			}

			// Generate final list of frames.
			std::vector<QUrl> urls;
			urls.reserve(sortedFilenames.size());
			for(const auto& iter : sortedFilenames) {
				QFileInfo fileInfo(directory, iter);
				QUrl url = sourceUrl;
				if(isLocalPath)
					url = QUrl::fromLocalFile(fileInfo.filePath());
				else
					url.setPath(fileInfo.filePath());
				urls.push_back(url);
			}

			return urls;
		});
	}
}

/******************************************************************************
* Checks if a filename matches to the given wildcard pattern.
******************************************************************************/
bool FileSourceImporter::matchesWildcardPattern(const QString& pattern, const QString& filename)
{
	QString::const_iterator p = pattern.constBegin();
	QString::const_iterator f = filename.constBegin();
	while(p != pattern.constEnd() && f != filename.constEnd()) {
		if(*p == QChar('*')) {
			if(!f->isDigit())
				return false;
			do { ++f; }
			while(f != filename.constEnd() && f->isDigit());
			++p;
			continue;
		}
		else if(*p != *f)
			return false;
		++p;
		++f;
	}
	return p == pattern.constEnd() && f == filename.constEnd();
}

/******************************************************************************
* Writes an animation frame information record to a binary output stream.
******************************************************************************/
SaveStream& operator<<(SaveStream& stream, const FileSourceImporter::Frame& frame)
{
	stream.beginChunk(0x03);
	stream << frame.sourceFile << frame.byteOffset << frame.lineNumber << frame.lastModificationTime << frame.label << frame.parserData;
	stream.endChunk();
	return stream;
}

/******************************************************************************
* Reads an animation frame information record from a binary input stream.
******************************************************************************/
LoadStream& operator>>(LoadStream& stream, FileSourceImporter::Frame& frame)
{
	stream.expectChunk(0x03);
	stream >> frame.sourceFile >> frame.byteOffset >> frame.lineNumber >> frame.lastModificationTime >> frame.label >> frame.parserData;
	stream.closeChunk();
	return stream;
}

/******************************************************************************
* Fetches the source URL and calls loadFile().
******************************************************************************/
void FileSourceImporter::FrameLoader::perform()
{
	// Let the subclass implementation parse the file.
	QFile file(_localFilename);
	setResult(loadFile(file));
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
