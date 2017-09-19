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

#include <core/Core.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/utilities/io/ObjectLoadStream.h>
#include <core/utilities/io/ObjectSaveStream.h>
#include <core/utilities/io/FileManager.h>
#include <core/app/Application.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/dataset/scene/ObjectNode.h>
#include <core/dataset/io/FileImporter.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/UndoStack.h>
#include "FileSource.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

IMPLEMENT_OVITO_CLASS(FileSource);
DEFINE_REFERENCE_FIELD(FileSource, importer);
DEFINE_PROPERTY_FIELD(FileSource, adjustAnimationIntervalEnabled);
DEFINE_PROPERTY_FIELD(FileSource, sourceUrl);
DEFINE_PROPERTY_FIELD(FileSource, playbackSpeedNumerator);
DEFINE_PROPERTY_FIELD(FileSource, playbackSpeedDenominator);
DEFINE_PROPERTY_FIELD(FileSource, playbackStartTime);
DEFINE_REFERENCE_FIELD(FileSource, dataObjects);
SET_PROPERTY_FIELD_LABEL(FileSource, importer, "File Importer");
SET_PROPERTY_FIELD_LABEL(FileSource, adjustAnimationIntervalEnabled, "Adjust animation length to time series");
SET_PROPERTY_FIELD_LABEL(FileSource, sourceUrl, "Source location");
SET_PROPERTY_FIELD_LABEL(FileSource, playbackSpeedNumerator, "Playback rate numerator");
SET_PROPERTY_FIELD_LABEL(FileSource, playbackSpeedDenominator, "Playback rate denominator");
SET_PROPERTY_FIELD_LABEL(FileSource, playbackStartTime, "Playback start time");
SET_PROPERTY_FIELD_LABEL(FileSource, dataObjects, "Objects");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(FileSource, playbackSpeedNumerator, IntegerParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(FileSource, playbackSpeedDenominator, IntegerParameterUnit, 1);
SET_PROPERTY_FIELD_CHANGE_EVENT(FileSource, sourceUrl, ReferenceEvent::TitleChanged);

/******************************************************************************
* Constructs the object.
******************************************************************************/
FileSource::FileSource(DataSet* dataset) : CachingPipelineObject(dataset),
	_adjustAnimationIntervalEnabled(true), 
	_playbackSpeedNumerator(1), 
	_playbackSpeedDenominator(1), 
	_playbackStartTime(0)
{
}

/******************************************************************************
* Sets the source location for importing data.
******************************************************************************/
bool FileSource::setSource(QUrl sourceUrl, FileSourceImporter* importer, bool autodetectFileSequences)
{
	// Make file paths absolute.
	if(sourceUrl.isLocalFile()) {
		QFileInfo fileInfo(sourceUrl.toLocalFile());
		if(fileInfo.isRelative())
			sourceUrl = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
	}

	if(this->sourceUrl() == sourceUrl && this->importer() == importer)
		return true;

	QFileInfo fileInfo(sourceUrl.path());
	_originallySelectedFilename = fileInfo.fileName();

	if(importer) {
		// If URL is not already a wildcard pattern, generate a default pattern by
		// replacing last sequence of numbers in the filename with a wildcard character.
		if(autodetectFileSequences && importer->autoGenerateWildcardPattern() && !_originallySelectedFilename.contains('*') && !_originallySelectedFilename.contains('?')) {
			int startIndex, endIndex;
			for(endIndex = _originallySelectedFilename.length() - 1; endIndex >= 0; endIndex--)
				if(_originallySelectedFilename.at(endIndex).isNumber()) break;
			if(endIndex >= 0) {
				for(startIndex = endIndex-1; startIndex >= 0; startIndex--)
					if(!_originallySelectedFilename.at(startIndex).isNumber()) break;
				QString wildcardPattern = _originallySelectedFilename.left(startIndex+1) + '*' + _originallySelectedFilename.mid(endIndex+1);
				fileInfo.setFile(fileInfo.dir(), wildcardPattern);
				sourceUrl.setPath(fileInfo.filePath());
				OVITO_ASSERT(sourceUrl.isValid());
			}
		}

		if(this->sourceUrl() == sourceUrl && this->importer() == importer)
			return true;
	}

	// Make the import process reversible.
	UndoableTransaction transaction(dataset()->undoStack(), tr("Set input file"));

	// Make the call to setSource() undoable.
	class SetSourceOperation : public UndoableOperation {
	public:
		SetSourceOperation(FileSource* obj) : _obj(obj), _oldUrl(obj->sourceUrl()), _oldImporter(obj->importer()) {}
		virtual void undo() override {
			QUrl url = _obj->sourceUrl();
			OORef<FileSourceImporter> importer = _obj->importer();
			_obj->setSource(_oldUrl, _oldImporter, false);
			_oldUrl = url;
			_oldImporter = importer;
		}
		virtual QString displayName() const override { 
			return QStringLiteral("Set file source url"); 
		}		
	private:
		QUrl _oldUrl;
		OORef<FileSourceImporter> _oldImporter;
		OORef<FileSource> _obj;
	};
	dataset()->undoStack().pushIfRecording<SetSourceOperation>(this);

	_sourceUrl.set(this, PROPERTY_FIELD(sourceUrl), sourceUrl);
	_importer.set(this, PROPERTY_FIELD(importer), importer);

	// Cancel any frame loading operations currently in progress.
	//cancelLoadOperations();

	// Set flag which indicates that the file being loaded is a newly selected one.
	_isNewFile = true;

	// Trigger a reload of the current frame.
	invalidateFrameCache();
	_frames.clear();

	// Scan the input source for animation frames.
	updateListOfFrames();

	transaction.commit();

	notifyDependents(ReferenceEvent::TitleChanged);

	return true;
}

/******************************************************************************
* Scans the input source for animation frames and updates the internal list of frames.
******************************************************************************/
void FileSource::updateListOfFrames()
{
	// Update the list of frames.
	SharedFuture<QVector<FileSourceImporter::Frame>> framesFuture = requestFrameList(true);

	// Show progress in the main window.
	dataset()->container()->taskManager().registerTask(framesFuture);

	// Catch exceptions and display error messages.
	framesFuture.finally_future(executor(), [](SharedFuture<QVector<FileSourceImporter::Frame>> future) {
		try {
			future.results();
		}
		catch(const Exception& ex) {
			ex.reportError();
		}
	});
}

/******************************************************************************
* Updates the internal list of input frames. 
* Invalidates cached frames in case they did change.
******************************************************************************/
void FileSource::setListOfFrames(const QVector<FileSourceImporter::Frame>& frames)
{
	_framesListFuture.reset();

	// Invalidate all cached frames that are no longer present.
	for(int frameIndex = frames.size(); frameIndex < _frames.size(); frameIndex++)
		invalidateFrameCache(frameIndex);

	// Invalidate the last frame of the old list, because its validity interval is reduced when additional frames are added.
	if(frames.size() > _frames.size())
		invalidateFrameCache(_frames.size() - 1);

	// Invalidate all cached frames that have changed.
	for(int frameIndex = 0; frameIndex < _frames.size() && frameIndex < frames.size(); frameIndex++) {
		if(frames[frameIndex] != _frames[frameIndex])
			invalidateFrameCache(frameIndex);
	}
	
	// Replace our internal list of frames.
	_frames = frames;

	// When loading a new file sequence, jump to the frame initially selected by the user in the
	// file selection dialog.
	int jumpToFrame = -1;
	if(_isNewFile) {
		for(int frameIndex = 0; frameIndex < _frames.size(); frameIndex++) {
			QFileInfo fileInfo(_frames[frameIndex].sourceFile.path());
			if(fileInfo.fileName() == _originallySelectedFilename) {
				jumpToFrame = frameIndex;
				break;
			}
		}
	}

	// Adjust the animation length to match the number of source frames.
	adjustAnimationInterval(jumpToFrame);

	// Notify dependents that the list of source frames has changed.
	notifyDependents(ReferenceEvent::ObjectStatusChanged);
}

/******************************************************************************
* Given an animation time, computes the source frame to show.
******************************************************************************/
int FileSource::animationTimeToSourceFrame(TimePoint time) const
{
	int animFrame = dataset()->animationSettings()->timeToFrame(time);
	return (animFrame - playbackStartTime()) *
			std::max(1, playbackSpeedNumerator()) /
			std::max(1, playbackSpeedDenominator());
}

/******************************************************************************
* Given a source frame index, returns the animation time at which it is shown.
******************************************************************************/
TimePoint FileSource::sourceFrameToAnimationTime(int frame) const
{
	int animFrame = frame *
			std::max(1, playbackSpeedDenominator()) /
			std::max(1, playbackSpeedNumerator()) +
			playbackStartTime();
	return dataset()->animationSettings()->frameToTime(animFrame);
}

/******************************************************************************
* Returns the current status of the pipeline object.
******************************************************************************/
PipelineStatus FileSource::status() const
{
	PipelineStatus status = CachingPipelineObject::status();
	if(_framesListFuture.isValid() || _numActiveFrameLoaders > 0) status.setType(PipelineStatus::Pending);
	return status;
}

/******************************************************************************
* Asks the object for the result of the data pipeline at the given time.
******************************************************************************/
Future<PipelineFlowState> FileSource::evaluateInternal(TimePoint time)
{
	// Convert the animation time to a frame number.
	int frame = animationTimeToSourceFrame(time);

	// Clamp to frame range.
	if(frame < 0) frame = 0;
	else if(frame >= numberOfFrames() && numberOfFrames() > 0) frame = numberOfFrames() - 1;

	// Call implementation routine.
	return requestFrameInternal(frame);
}

/******************************************************************************
* Scans the external data file and returns the list of discovered input frames.
******************************************************************************/
SharedFuture<QVector<FileSourceImporter::Frame>> FileSource::requestFrameList(bool forceReload)
{
	// Without an importer object the list of frames is empty.
	if(!importer())
		return Future<QVector<FileSourceImporter::Frame>>::createImmediateEmplace();

	// Return the active future when the frame loading process is currently in progress.
	if(_framesListFuture.isValid()) {
		if(!forceReload || !_framesListFuture.isFinished())
			return _framesListFuture;
		_framesListFuture.reset();
	}

	// Return the cached frames list if available.
	if(!_frames.empty() && !forceReload) {
		return _frames;
	}

	// Forward request to the importer object.
	// Intercept future results when they become available and cache them.
	_framesListFuture = importer()->discoverFrames(sourceUrl())
		.then(executor(), [this, forceReload](QVector<FileSourceImporter::Frame>&& frameList) {
//			qDebug() << "FileSource::requestFrameList: received frames list (" << frameList.size() << "frames). Storing it";
			setListOfFrames(frameList);

			// If update was triggered by user, also reload the current frame.
			if(forceReload)
				notifyDependents(ReferenceEvent::TargetChanged);

			// Simply forward the frame list to the caller.
			return std::move(frameList);
		});

	// Are we already done with loading?
	if(_framesListFuture.isFinished())
		return std::move(_framesListFuture);

	// The status of this pipeline object changes while loading is in progress.
	notifyDependents(ReferenceEvent::ObjectStatusChanged);

	// Reset the status after the Future is fulfilled.
	_framesListFuture.finally(executor(), [this]() {
		_framesListFuture.reset();
		notifyDependents(ReferenceEvent::ObjectStatusChanged);
	});

	return _framesListFuture;
}

/******************************************************************************
* Requests a source frame from the input sequence.
******************************************************************************/
SharedFuture<PipelineFlowState> FileSource::requestFrame(int frame)
{
	return evaluate(sourceFrameToAnimationTime(frame));
}

/******************************************************************************
* Requests a source frame from the input sequence.
******************************************************************************/
Future<PipelineFlowState> FileSource::requestFrameInternal(int frame)
{
//	qDebug() << "FileSource::requestFrameInternal: called for frame" << frame;

	// First request the list of source frames and wait until it becomes available.
	return requestFrameList()
		.then(executor(), [this,frame](const QVector<FileSourceImporter::Frame>& sourceFrames) -> Future<PipelineFlowState> {
//			qDebug() << "FileSource::requestFrameInternal: received frames list";

			// Is the requested frame out of range?
			if(frame >= sourceFrames.size()) {

				TimeInterval interval = TimeInterval::infinite();
				if(frame < 0) interval.setEnd(sourceFrameToAnimationTime(0) - 1);
				else if(frame >= sourceFrames.size() && sourceFrames.size() > 0) interval.setStart(sourceFrameToAnimationTime(sourceFrames.size()));

				return PipelineFlowState(PipelineStatus(PipelineStatus::Error, tr("The file source path is empty or has not been set (no files found).")),
						dataObjects(), interval);
			}
			else if(frame < 0) {
				return PipelineFlowState(PipelineStatus(PipelineStatus::Error, tr("The requested source frame is out of range.")),
						dataObjects(), TimeInterval::infinite());
			}

			// Compute validity interval of the returned state.
			TimeInterval interval = TimeInterval::infinite();
			if(frame > 0)
				interval.setStart(sourceFrameToAnimationTime(frame));
			if(frame < sourceFrames.size() - 1)
				interval.setEnd(std::max(sourceFrameToAnimationTime(frame + 1) - 1, sourceFrameToAnimationTime(frame)));
			OVITO_ASSERT(frame >= 0);
			OVITO_ASSERT(!interval.isEmpty());

			const FileSourceImporter::Frame& frameInfo = sourceFrames[frame];

			// Retrieve the file.
			Future<PipelineFlowState> loadFrameFuture = Application::instance()->fileManager()->fetchUrl(dataset()->container()->taskManager(), frameInfo.sourceFile)
				.then(executor(), [this, frameInfo, frame, interval](const QString& filename) -> Future<PipelineFlowState> {
					
					// Without an importer object we have to give up immediately.
					if(!importer()) {
						// Just return what we have stored.
						return PipelineFlowState(PipelineStatus(PipelineStatus::Error, tr("The file source path has not been set.")),
								dataObjects(), TimeInterval::infinite());
					}

					// Create the frame loader for the requested frame.
					FileSourceImporter::FrameLoaderPtr frameLoader = importer()->createFrameLoader(frameInfo, filename);
					OVITO_ASSERT(frameLoader);
//					qDebug() << "FileSource::requestFrameInternal: created frame loader " << frameLoader.get();
					
					// Execute the loader in a background thread.
					// Collect results from the loader in the UI thread once it has finished running.
					return dataset()->container()->taskManager().runTaskAsync(frameLoader)
						.then(executor(), [this, frame, interval](FileSourceImporter::FrameDataPtr&& frameData) mutable {
//							qDebug() << "FileSource::requestFrameInternal: frame loader finished -> handing over data";

							UndoSuspender noUndo(this);
							PipelineFlowState existingState;

							// Re-use existing data objects if possible.
							for(DataObject* o : dataObjects()) {
								existingState.addObject(o);
							}
							// Do not modify the subobjects if we are not loading the current animation frame.
							if(!interval.contains(dataset()->animationSettings()->time())) {
								existingState.cloneObjectsIfNeeded(false);
							}

							// Let the data container insert its data into the pipeline state.
							_handOverInProgress = true;
							try {
								PipelineFlowState output = frameData->handOver(dataset(), existingState, _isNewFile);
								_isNewFile = false;
								_handOverInProgress = false;
								existingState.clear();
								output.setStateValidity(interval);
								output.setSourceFrame(frame);

								// When loading the current frame, turn the data objects into sub-objects of this
								// FileSource so that they appear in the pipeline viewer.
								if(interval.contains(dataset()->animationSettings()->time())) {
									QVector<DataObject*> dataObjects;
									for(const auto& o : output.objects()) {
										dataObjects.push_back(o);
									}
									_dataObjects.set(this, PROPERTY_FIELD(dataObjects), std::move(dataObjects));
									_attributes = output.attributes();
									setStoredFrameIndex(frame);
								}

								// Never directly output the current subobjects to the pipeline.
								output.cloneObjectsIfNeeded(false);

								return output;
							}
							catch(...) {
								_handOverInProgress = false;
								throw;
							}
						});
				});

			// Change status to 'Pending' during long-running load operations.
			if(!loadFrameFuture.isFinished()) {
				if(_numActiveFrameLoaders++ == 0)
					notifyDependents(ReferenceEvent::ObjectStatusChanged);

				// Reset the loading status after the Future is fulfilled.
				loadFrameFuture.finally(executor(), [this]() {
					OVITO_ASSERT(_numActiveFrameLoaders > 0);
					if(--_numActiveFrameLoaders == 0)
						notifyDependents(ReferenceEvent::ObjectStatusChanged);
				});
			}

			return loadFrameFuture;
		})
		// Post-process the results of the load operation before returning them to the caller.
		//
		//  - Turn any exception that was thrown during loading into a 
		//    valid pipeline state with an error code.
		//
		.then_future(executor(), [this,frame](Future<PipelineFlowState> future) mutable {
//					qDebug() << "FileSource::requestFrameInternal: post-processing results of future" << future.sharedState().get();
				OVITO_ASSERT(future.isFinished());
				OVITO_ASSERT(!future.isCanceled());
				try {
					PipelineFlowState state = future.result();
					setStatus(state.status());
					return state;
				}
				catch(Exception& ex) {
					ex.setContext(dataset());
					setStatus(PipelineStatus(PipelineStatus::Error, ex.messages().join(QChar('\n'))));
					ex.reportError();
					ex.prependGeneralMessage(tr("File source reported:"));
					return PipelineFlowState(PipelineStatus(PipelineStatus::Error, ex.messages().join(QChar(' '))), sourceFrameToAnimationTime(frame));
				}
			});
}

/******************************************************************************
* This will trigger a reload of an animation frame upon next request.
******************************************************************************/
void FileSource::reloadFrame(int frameIndex)
{
	if(!importer())
		return;

	// Remove source file from file cache so that it will be downloaded again 
	// if it came from a remote location.
	if(frameIndex >= 0 && frameIndex < frames().size())
		Application::instance()->fileManager()->removeFromCache(frames()[frameIndex].sourceFile);

	invalidateFrameCache(frameIndex);
	notifyDependents(ReferenceEvent::TargetChanged);
}

/******************************************************************************
* Clears the cache entry for the given input frame.
******************************************************************************/
void FileSource::invalidateFrameCache(int frameIndex)
{
	if(frameIndex == -1 || frameIndex == storedFrameIndex()) {
		setStoredFrameIndex(-1);
	}
	invalidatePipelineCache();
}

/******************************************************************************
* Sets which frame is currently stored in this object.
******************************************************************************/
void FileSource::setStoredFrameIndex(int frameIndex)
{
	if(_storedFrameIndex != frameIndex) {
		_storedFrameIndex = frameIndex;
		notifyDependents(ReferenceEvent::ObjectStatusChanged);
	}
}

/******************************************************************************
* Adjusts the animation interval of the current data set to the number of
* frames reported by the file parser.
******************************************************************************/
void FileSource::adjustAnimationInterval(int gotoFrameIndex)
{
	// Automatic adjustment of animation interval may be disabled for this file source.
	if(!adjustAnimationIntervalEnabled())
		return;

	AnimationSettings* animSettings = dataset()->animationSettings();
	UndoSuspender noUndo(this);

	// Adjust the length of the animation interval to match the number of frames in the loaded sequence.
	int numFrames = std::max(1, numberOfFrames());
	TimeInterval interval(sourceFrameToAnimationTime(0), sourceFrameToAnimationTime(std::max(numberOfFrames()-1,0)));
	animSettings->setAnimationInterval(interval);

	// Jump to the frame corresponding to the file picked by the user in the file selection dialog.	
	if(gotoFrameIndex >= 0 && gotoFrameIndex < numberOfFrames()) {
		animSettings->setTime(sourceFrameToAnimationTime(gotoFrameIndex));
	}
	else if(animSettings->time() > interval.end())
		animSettings->setTime(interval.end());
	else if(animSettings->time() < interval.start())
		animSettings->setTime(interval.start());

	// The file importer might assign names to different input frames, e.g. the file name when
	// a file sequence was loaded, or the simulation time when it was parsed from the file headers.
	// We pass the frame names to the animation system so that they can be displayed in the
	// time line.  
	animSettings->clearNamedFrames();
	for(int animFrame = animSettings->timeToFrame(interval.start()); animFrame <= animSettings->timeToFrame(interval.end()); animFrame++) {
		int inputFrame = animationTimeToSourceFrame(animSettings->frameToTime(animFrame));
		if(inputFrame >= 0 && inputFrame < _frames.size() && !_frames[inputFrame].label.isEmpty())
			animSettings->assignFrameName(animFrame, _frames[inputFrame].label);
	}
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void FileSource::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	CachingPipelineObject::saveToStream(stream, excludeRecomputableData);
	stream.beginChunk(0x02);
	stream << _frames;

	// Store the relative path to the external file (in addition to the absolute path, which is automatically saved).
	QUrl relativePath = sourceUrl();
	if(relativePath.isLocalFile() && !relativePath.isRelative()) {
		// Extract relative portion of path (only if both the scene file path and the external file path are absolute).
		QFileDevice* fileDevice = qobject_cast<QFileDevice*>(stream.dataStream().device());
		if(fileDevice) {
			QFileInfo sceneFile(fileDevice->fileName());
			if(sceneFile.isAbsolute()) {
				QFileInfo externalFile(relativePath.toLocalFile());
				// Currently this only works for files in the same directory.
				if(externalFile.path() == sceneFile.path()) {
					relativePath = QUrl::fromLocalFile(externalFile.fileName());
				}
			}
		}
	}
	stream << relativePath;

	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void FileSource::loadFromStream(ObjectLoadStream& stream)
{
	CachingPipelineObject::loadFromStream(stream);
	stream.expectChunk(0x02);
	stream >> _frames;

	QUrl relativePath;
	stream >> relativePath;

	// If the absolute path no longer exists, replace it with the relative one resolved against
	// the scene file's path.
	if(sourceUrl().isLocalFile() && relativePath.isLocalFile()) {
		QFileInfo relativeFileInfo(relativePath.toLocalFile());
		if(relativeFileInfo.isAbsolute() == false) {
			QFileDevice* fileDevice = qobject_cast<QFileDevice*>(stream.dataStream().device());
			if(fileDevice) {
				QFileInfo sceneFile(fileDevice->fileName());
				if(sceneFile.isAbsolute()) {
					_sourceUrl.set(this, PROPERTY_FIELD(sourceUrl), QUrl::fromLocalFile(QFileInfo(sceneFile.dir(), relativeFileInfo.filePath()).absoluteFilePath()));

					// Also update the paths stored in the frame records.
					for(FileSourceImporter::Frame& frame : _frames) {
						if(frame.sourceFile.isLocalFile()) {
							QFileInfo frameFile(frame.sourceFile.toLocalFile());
							frame.sourceFile = QUrl::fromLocalFile(QFileInfo(sceneFile.dir(), frameFile.fileName()).absoluteFilePath());
						}
					}
				}
			}
		}
	}
	stream.closeChunk();
}

/******************************************************************************
* Returns the title of this object.
******************************************************************************/
QString FileSource::objectTitle()
{
	QString filename;
	int frameIndex = storedFrameIndex();
	if(frameIndex >= 0) {
		filename = QFileInfo(frames()[frameIndex].sourceFile.path()).fileName();
	}
	else if(!sourceUrl().isEmpty()) {
		filename = QFileInfo(sourceUrl().path()).fileName();
	}
	if(importer())
		return QString("%2 [%1]").arg(importer()->objectTitle()).arg(filename);
	return CachingPipelineObject::objectTitle();
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void FileSource::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(adjustAnimationIntervalEnabled) ||
			field == PROPERTY_FIELD(playbackSpeedNumerator) ||
			field == PROPERTY_FIELD(playbackSpeedDenominator) ||
			field == PROPERTY_FIELD(playbackStartTime)) {
		adjustAnimationInterval();
	}
	CachingPipelineObject::propertyChanged(field);
}

/******************************************************************************
* Returns the number of sub-objects that should be displayed in the modifier stack.
******************************************************************************/
int FileSource::editableSubObjectCount()
{
	return dataObjects().size();
}

/******************************************************************************
* Returns a sub-object that should be listed in the modifier stack.
******************************************************************************/
RefTarget* FileSource::editableSubObject(int index)
{
	return dataObjects()[index];
}

/******************************************************************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool FileSource::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(event->type() == ReferenceEvent::TargetChanged && dataObjects().contains(static_cast<DataObject*>(source))) {
		if(_handOverInProgress) {
			// Block target changed messages from subobjects while data hand-over is in progress.
			return false;
		}
		else if(!event->sender()->isBeingLoaded()) {
			// Whenever the user changes the subobjects, update the pipeline state stored in the cache.
			PipelineFlowState state = getPipelineCache();
			state.clearObjects();
			for(DataObject* o : dataObjects())
				state.addObject(o);
			state.cloneObjectsIfNeeded(false);
			setPipelineCache(std::move(state));
		}
	}

	return CachingPipelineObject::referenceEvent(source, event);
}

/******************************************************************************
* Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
******************************************************************************/
void FileSource::referenceInserted(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(dataObjects))
		notifyDependents(ReferenceEvent::SubobjectListChanged);

	CachingPipelineObject::referenceInserted(field, newTarget, listIndex);
}

/******************************************************************************
* Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
******************************************************************************/
void FileSource::referenceRemoved(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(dataObjects))
		notifyDependents(ReferenceEvent::SubobjectListChanged);

	CachingPipelineObject::referenceRemoved(field, newTarget, listIndex);
}


OVITO_END_INLINE_NAMESPACE
}	// End of namespace
