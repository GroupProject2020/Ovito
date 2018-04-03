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

#pragma once


#include <core/Core.h>
#include <core/dataset/pipeline/CachingPipelineObject.h>
#include <core/utilities/concurrent/Future.h>
#include "FileSourceImporter.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

/**
 * \brief An object in the data pipeline that reads data from an external file. 
 *
 * This class works in concert with the FileSourceImporter class.
 */
class OVITO_CORE_EXPORT FileSource : public CachingPipelineObject
{
	Q_OBJECT
	OVITO_CLASS(FileSource)
	
public:

	/// Constructor.
	Q_INVOKABLE FileSource(DataSet* dataset);
	
	/// \brief Sets the source location(s) for importing data.
	/// \param sourceUrls The new source location(s).
	/// \param importer The importer object that will parse the input file.
	/// \param autodetectFileSequences Enables the automatic detection of file sequences.
	/// \return false if the operation has been canceled by the user.
	bool setSource(std::vector<QUrl> sourceUrls, FileSourceImporter* importer, bool autodetectFileSequences);

	/// \brief This triggers a reload of input data from the external file for the given frame.
	/// \param frameIndex The index of the input frame to refresh; or -1 to refresh all frames.
	void reloadFrame(int frameIndex = -1);

	/// \brief Scans the external file source and updates the internal frame list.
	/// Note: This method operates asynchronously.
	void updateListOfFrames();

	/// \brief Implementation method for scanning the external data file to find all contained frames.
	/// This is an implementation detail. Please use the high-level method updateListOfFrames() instead. 
	SharedFuture<QVector<FileSourceImporter::Frame>> requestFrameList(bool forceRescan, bool forceReloadOfCurrentFrame);

	/// \brief Returns the number of frames that are provided by the data source.
	int numberOfFrames() const { return _frames.size(); }

	/// \brief Returns the index of the input frame currently stored by this source object.
	int storedFrameIndex() const { return _storedFrameIndex; }

	/// \brief Returns the list of animation frames in the input file(s).
	const QVector<FileSourceImporter::Frame>& frames() const { return _frames; }

	/// \brief Given an animation time, computes the source frame to show.
	virtual int animationTimeToSourceFrame(TimePoint time) const override;

	/// \brief Given a source frame index, returns the animation time at which it is shown.
	virtual TimePoint sourceFrameToAnimationTime(int frame) const override;

	/// \brief Requests a source frame from the input sequence.
	SharedFuture<PipelineFlowState> requestFrame(int frame);

	/// Returns the title of this object.
	virtual QString objectTitle() override;

	/// Returns the number of sub-objects that should be displayed in the modifier stack.
	virtual int editableSubObjectCount() override;

	/// Returns a sub-object that should be listed in the modifier stack.
	virtual RefTarget* editableSubObject(int index) override;

	/// Returns the current status of the pipeline object.
	virtual PipelineStatus status() const override;

	/// Inserts an object into the current list of active data objects of this file source.
	void insertDataObject(int index, DataObject* obj) { 
		OVITO_ASSERT(!dataObjects().contains(obj)); 
		_dataObjects.insert(this, PROPERTY_FIELD(dataObjects), index, obj); 
	}

	/// Appends an object to the current list of active data objects of this file source.
	void addDataObject(DataObject* obj) { 
		OVITO_ASSERT(!dataObjects().contains(obj)); 
		_dataObjects.push_back(this, PROPERTY_FIELD(dataObjects), obj); 
	}
	
	/// Removes an object from the current list of active data objects of this file source.
	void removeDataObject(int index) { _dataObjects.remove(this, PROPERTY_FIELD(dataObjects), index); }

	/// Returns the global attributes loaded from the current file, which are passed along with the data objects into the pipeline.
	const QVariantMap& attributes() const { return _attributes; }
		
protected:

	/// Asks the object for the results of the data pipeline.
	virtual Future<PipelineFlowState> evaluateInternal(TimePoint time) override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
	virtual void referenceInserted(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex) override;

	/// Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
	virtual void referenceRemoved(const PropertyFieldDescriptor& field, RefTarget* oldTarget, int listIndex) override;

	/// Creates a copy of this object.
	virtual OORef<RefTarget> clone(bool deepCopy, CloneHelper& cloneHelper) override;

private:

	/// Requests a source frame from the input sequence.
	Future<PipelineFlowState> requestFrameInternal(int frame);

	/// \brief Adjusts the animation interval of the current data set to the number of frames in the data source.
	void adjustAnimationInterval(int gotoFrameIndex = -1);

	/// Sets which frame is currently stored by this source object.
	void setStoredFrameIndex(int frameIndex);

	/// Updates the internal list of input frames. 
	/// Invalidates cached frames in case they did change.
	void setListOfFrames(QVector<FileSourceImporter::Frame> frames);

	/// Clears the cache entry for the given input frame.
	void invalidateFrameCache(int frameIndex = -1);

private:

	/// The associated importer object that is responsible for parsing the input file.
	DECLARE_REFERENCE_FIELD_FLAGS(FileSourceImporter, importer, PROPERTY_FIELD_ALWAYS_DEEP_COPY | PROPERTY_FIELD_NO_UNDO);

	/// Controls whether the scene's animation interval is adjusted to the number of frames found in the input file.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, adjustAnimationIntervalEnabled, setAdjustAnimationIntervalEnabled);

	/// The list of source files (may include wild-card patterns).
	DECLARE_PROPERTY_FIELD_FLAGS(std::vector<QUrl>, sourceUrls, PROPERTY_FIELD_NO_UNDO);

	/// Controls the mapping of input file frames to animation frames (i.e. the numerator of the playback rate for the file sequence).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, playbackSpeedNumerator, setPlaybackSpeedNumerator);

	/// Controls the mapping of input file frames to animation frames (i.e. the denominator of the playback rate for the file sequence).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, playbackSpeedDenominator, setPlaybackSpeedDenominator);

	/// Specifies the starting animation frame to which the first frame of the file sequence is mapped.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, playbackStartTime, setPlaybackStartTime);

	/// Stores the prototypes of the loaded data objects.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD_FLAGS(DataObject, dataObjects, setDataObjects, PROPERTY_FIELD_ALWAYS_DEEP_COPY | PROPERTY_FIELD_NO_CHANGE_MESSAGE | PROPERTY_FIELD_DONT_SAVE_RECOMPUTABLE_DATA);

	/// The list of frames of the data source.
	QVector<FileSourceImporter::Frame> _frames;

	/// The active future if loading the list of frames is in progress.
	SharedFuture<QVector<FileSourceImporter::Frame>> _framesListFuture;

	/// The number of frame loading operatiosn currently in progress.
	int _numActiveFrameLoaders = 0;

	/// The index of the loaded source frame that is currently stored.
	int _storedFrameIndex = -1;

	/// Flag indicating that the file being loaded has been newly selected by the user.
	/// If not, then the file being loaded is just another frame from the existing sequence.
	bool _isNewFile = false;

	/// The file that was originally selected by the user when importing the input file.
	QString _originallySelectedFilename;

	/// Indicates whether the data from a frame loader is currently being handed over to the FileSource.
	bool _handOverInProgress = false;

	/// The global attributes loaded for the current animation frame.
	QVariantMap _attributes;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
