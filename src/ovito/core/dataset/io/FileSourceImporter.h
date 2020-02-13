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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/dataset/io/FileImporter.h>
#include <ovito/core/dataset/pipeline/PipelineStatus.h>
#include <ovito/core/utilities/concurrent/Future.h>
#include <ovito/core/utilities/concurrent/AsynchronousTask.h>
#include <ovito/core/utilities/io/FileManager.h>

namespace Ovito {

/**
 * \brief Base class for file parsers that can reload a file that has been imported into the scene.
 */
class OVITO_CORE_EXPORT FileSourceImporter : public FileImporter
{
	Q_OBJECT
	OVITO_CLASS(FileSourceImporter)

public:

	/// Data structure that stores meta information about a source animation frame.
	struct Frame {

		/// Default constructor.
		Frame() = default;

		/// Initialization constructor.
		explicit Frame(const FileHandle& fileHandle, qint64 offset = 0, int linenum = 1, const QString& name = QString(), qint64 parserInfo = 0) :
				sourceFile(fileHandle.sourceUrl()), byteOffset(offset), lineNumber(linenum), label(name.isEmpty() ? fileHandle.sourceUrl().fileName() : name), parserData(parserInfo) {
			if(!fileHandle.localFilePath().isEmpty())
				lastModificationTime = QFileInfo(fileHandle.localFilePath()).lastModified();
		}

		/// Initialization constructor.
		explicit Frame(const QUrl& url, qint64 offset = 0, int linenum = 1, const QDateTime& modTime = QDateTime(), const QString& name = QString(), qint64 parserInfo = 0) :
			sourceFile(url), byteOffset(offset), lineNumber(linenum), lastModificationTime(modTime), label(name), parserData(parserInfo) {}

		/// The source file that contains the data of the animation frame.
		QUrl sourceFile;

		/// The byte offset into the source file where the frame's data is stored.
		qint64 byteOffset = 0;

		/// The line number in the source file where the frame data is stored, if the file has a text-based format.
		int lineNumber = 1;

		/// The last modification time of the source file.
		/// This is used to detect changes of the source file, which let the stored byte offset become invalid.
		QDateTime lastModificationTime;

		/// The name or label of the source frame.
		QString label;

		/// An informational field that can be used by the file parser to store additional info about the frame.
		qint64 parserData = 0;

		/// Compares two data records.
		bool operator!=(const Frame& other) const {
			return (sourceFile != other.sourceFile) ||
					(byteOffset != other.byteOffset) ||
					(lineNumber != other.lineNumber) ||
					(lastModificationTime != other.lastModificationTime) ||
					(parserData != other.parserData);
		}
	};

	/**
	 * Base class for data structures holding a single frame's data.
	 */
	class OVITO_CORE_EXPORT FrameData
	{
	public:

		/// Transfers the loaded data into a data collection.
		/// This function is called by the system from the main thread after the asynchronous loading task
		/// has finished. An implementation of this method should try to re-use any existing data objects from the provided data collection.
		virtual OORef<DataCollection> handOver(const DataCollection* existing, bool isNewFile, FileSource* fileSource) = 0;

		/// Returns the status of the load operation.
		const PipelineStatus& status() const { return _status; }

		/// Sets the status of the load operation.
		void setStatus(const QString& statusText) { _status.setText(statusText); }

	private:

		/// Stores additional status information about the load operation.
		PipelineStatus _status;
	};

	/// A managed pointer to a FrameData instance.
	using FrameDataPtr = std::shared_ptr<FrameData>;

	/**
	 * Base class for frame data loader routines.
	 */
	class OVITO_CORE_EXPORT FrameLoader : public AsynchronousTask<FrameDataPtr>
	{
	public:

		/// Constructor.
		FrameLoader(const Frame& frame, const FileHandle& fileHandle) :
			_frame(frame), _fileHandle(fileHandle) {}

		/// Returns the source file information.
		const Frame& frame() const { return _frame; }

		/// Returns the local handle to the input data file.
		const FileHandle& fileHandle() const { return _fileHandle; }

		/// Calls loadFile() and sets the returned frame data as result of the asynchronous task.
		virtual void perform() override;

	protected:

		/// Reads the frame data from the external file.
		virtual FrameDataPtr loadFile() = 0;

	private:

		/// The source file information.
		Frame _frame;

		/// The local handle to the input file.
		FileHandle _fileHandle;
	};

	/// A managed pointer to a FrameLoader instance.
	using FrameLoaderPtr = std::shared_ptr<FrameLoader>;

	/**
	 * Base class for frame discovery routines.
	 */
	class OVITO_CORE_EXPORT FrameFinder : public AsynchronousTask<QVector<Frame>>
	{
	public:

		/// Constructor.
		FrameFinder(const FileHandle& file) : _file(file) {}

		/// Returns the data file to scan.
		const FileHandle& fileHandle() const { return _file; }

		/// Scans the source URL for input frames.
		virtual void perform() override;

	protected:

		/// Scans the data file and builds a list of source frames.
		virtual void discoverFramesInFile(QVector<Frame>& frames);

	private:

		/// The data file to scan.
		FileHandle _file;
	};

	/// A managed pointer to a FrameFinder instance.
	using FrameFinderPtr = std::shared_ptr<FrameFinder>;

public:

	/// \brief Constructs a new instance of this class.
	FileSourceImporter(DataSet* dataset) : FileImporter(dataset), _isMultiTimestepFile(false) {}

	///////////////////////////// from FileImporter /////////////////////////////

	/// \brief Asks the importer if the option to replace the currently selected object
	///        with the new file is available.
	virtual bool isReplaceExistingPossible(const QUrl& sourceUrl) override;

	/// \brief Imports the given file into the scene.
	virtual OORef<PipelineSceneNode> importFile(std::vector<QUrl> sourceUrls, ImportMode importMode, bool autodetectFileSequences) override;

	//////////////////////////// Specific methods ////////////////////////////////

	/// This method indicates whether a wildcard pattern should be automatically generated
	/// when the user picks a new input filename. The default implementation returns if isMultiTimestepFile is set to false.
	/// Subclasses can override this method to disable generation of wildcard patterns.
	virtual bool autoGenerateWildcardPattern() { return !isMultiTimestepFile(); }

	/// Scans the given external path(s) (which may be a directory and a wild-card pattern,
	/// or a single file containing multiple frames) to find all available animation frames.
	///
	/// \param sourceUrls The list of source files or wild-card patterns to scan for animation frames.
	/// \return A Future that will yield the list of discovered animation frames.
	///
	/// The default implementation of this method checks if the given URLs contain a wild-card pattern.
	/// If yes, it scans the directory to find all matching files.
	virtual Future<QVector<Frame>> discoverFrames(const std::vector<QUrl>& sourceUrls);

	/// Scans the given external path (which may be a directory and a wild-card pattern,
	/// or a single file containing multiple frames) to find all available animation frames.
	///
	/// \param sourceUrl The source file or wild-card patterns to scan for animation frames.
	/// \return A Future that will yield the list of discovered animation frames.
	///
	/// The default implementation of this method checks if the given URLs contain a wild-card pattern.
	/// If yes, it scans the directory to find all matching files.
	virtual Future<QVector<Frame>> discoverFrames(const QUrl& sourceUrl);

	/// \brief Returns the list of files that match the given wildcard pattern.
	static Future<std::vector<QUrl>> findWildcardMatches(const QUrl& sourceUrl, DataSet* dataset);

	/// \brief Sends a request to the FileSource owning this importer to reload the input file.
	void requestReload(bool refetchFiles = false, int frame = -1);

	/// \brief Sends a request to the FileSource owning this importer to refresh the animation frame sequence.
	void requestFramesUpdate();

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual FrameLoaderPtr createFrameLoader(const Frame& frame, const FileHandle& file) = 0;

	/// Creates an asynchronous frame discovery object that scans a file for contained animation frames.
	virtual FrameFinderPtr createFrameFinder(const FileHandle& file) { return {}; }

protected:

	/// \brief Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// This method is called when the pipeline scene node for the FileSource is created.
	/// It can be overwritten by importer subclasses to customize the initial pipeline, add modifiers, etc.
	/// The default implementation does nothing.
	virtual void setupPipeline(PipelineSceneNode* node, FileSource* importObj) {}

	/// Checks if a filename matches to the given wildcard pattern.
	static bool matchesWildcardPattern(const QString& pattern, const QString& filename);

	/// Determines whether the input file should be scanned to discover all contained frames.
	/// The default implementation returns the value of isMultiTimestepFile().
	virtual bool shouldScanFileForFrames(const QUrl& sourceUrl) const { return isMultiTimestepFile(); }

	/// Determines whether the URL contains a wildcard pattern.
	static bool isWildcardPattern(const QUrl& sourceUrl);

private:

	/// Indicates that the input file contains multiple timesteps.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, isMultiTimestepFile, setMultiTimestepFile, PROPERTY_FIELD_NO_CHANGE_MESSAGE);
};

/// \brief Writes an animation frame information record to a binary output stream.
/// \relates FileSourceImporter::Frame
OVITO_CORE_EXPORT SaveStream& operator<<(SaveStream& stream, const FileSourceImporter::Frame& frame);

/// \brief Reads an animation frame information record from a binary input stream.
/// \relates FileSourceImporter::Frame
OVITO_CORE_EXPORT LoadStream& operator>>(LoadStream& stream, FileSourceImporter::Frame& frame);

}	// End of namespace
