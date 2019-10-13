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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/dataset/pipeline/CachingPipelineObject.h>
#include <ovito/core/dataset/data/DataCollection.h>
#include <ovito/core/utilities/concurrent/Future.h>
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

	/// \brief Asks the object for the result of the data pipeline.
	/// \param time Specifies at which animation time the pipeline should be evaluated.
	/// \param breakOnError Tells the pipeline system to stop the evaluation as soon as a first error occurs.
	virtual SharedFuture<PipelineFlowState> evaluate(TimePoint time, bool breakOnError = false) override;

	/// \brief Returns the results of an immediate and preliminary evaluation of the data pipeline.
	virtual PipelineFlowState evaluatePreliminary() override;

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

	/// \brief Returns the index of the input frame currently stored by this source object.
	int storedFrameIndex() const { return _storedFrameIndex; }

	/// \brief Returns the list of animation frames in the input file(s).
	const QVector<FileSourceImporter::Frame>& frames() const { return _frames; }

	/// \brief Returns the number of animation frames this pipeline object can provide.
	virtual int numberOfSourceFrames() const override { return _frames.size(); }

	/// \brief Given an animation time, computes the source frame to show.
	virtual int animationTimeToSourceFrame(TimePoint time) const override;

	/// \brief Given a source frame index, returns the animation time at which it is shown.
	virtual TimePoint sourceFrameToAnimationTime(int frame) const override;

	/// \brief Returns the human-readable labels associated with the animation frames (e.g. the simulation timestep numbers).
	virtual QMap<int, QString> animationFrameLabels() const override;

	/// \brief Requests a source frame from the input sequence.
	SharedFuture<PipelineFlowState> requestFrame(int frame);

	/// Returns the title of this object.
	virtual QString objectTitle() const override;

	/// Returns the current status of the pipeline object.
	virtual PipelineStatus status() const override;

	/// Returns the list of data objects that are managed by this data source.
	/// The returned data objects will be displayed as sub-objects of the data source in the pipeline editor.
	virtual DataCollection* getSourceDataCollection() const override { return dataCollection(); }

protected:

	/// Asks the object for the results of the data pipeline.
	virtual Future<PipelineFlowState> evaluateInternal(TimePoint time, bool breakOnError) override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

private:

	/// Requests a source frame from the input sequence.
	Future<PipelineFlowState> requestFrameInternal(int frame);

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

	/// The list of source files (may include wild-card patterns).
	DECLARE_PROPERTY_FIELD_FLAGS(std::vector<QUrl>, sourceUrls, PROPERTY_FIELD_NO_UNDO);

	/// Controls the mapping of input file frames to animation frames (i.e. the numerator of the playback rate for the file sequence).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, playbackSpeedNumerator, setPlaybackSpeedNumerator);

	/// Controls the mapping of input file frames to animation frames (i.e. the denominator of the playback rate for the file sequence).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, playbackSpeedDenominator, setPlaybackSpeedDenominator);

	/// Specifies the starting animation frame to which the first frame of the file sequence is mapped.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, playbackStartTime, setPlaybackStartTime);

	/// Stores the prototypes of the loaded data objects.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(DataCollection, dataCollection, setDataCollection, PROPERTY_FIELD_ALWAYS_DEEP_COPY | PROPERTY_FIELD_NO_CHANGE_MESSAGE | PROPERTY_FIELD_DONT_SAVE_RECOMPUTABLE_DATA);

	/// The list of frames of the data source.
	QVector<FileSourceImporter::Frame> _frames;

	/// The human-readable labels associated with animation frames (e.g. the simulation timestep numbers).
	mutable QMap<int, QString> _frameLabels;

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

	/// Indicates that the cached pipeline state should be updated with the current contents of the data collection
	/// of this FileSource.
	bool _updateCacheWithDataCollection = false;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
