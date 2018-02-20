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

#pragma once


#include <core/Core.h>
#include <core/oo/RefTarget.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/scene/SceneNode.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

/**
 * \brief A meta-class for file exporters (i.e. classes derived from FileExporter).
 */
class OVITO_CORE_EXPORT FileExporterClass : public RefTarget::OOMetaClass 
{
public:

	/// Inherit standard constructor from base meta class.
	using RefTarget::OOMetaClass::OOMetaClass;

	/// \brief Returns the filename filter that specifies the file extension that can be exported by this service.
	/// \return A wild-card pattern for the file types that can be produced by the FileExporter class (e.g. \c "*.xyz" or \c "*").
	virtual QString fileFilter() const { 
		OVITO_ASSERT_MSG(false, "FileExporterClass::fileFilter()", "This method should be overridden by a meta-subclass of FileExporterClass.");
		return {}; 
	}
	
	/// \brief Returns the file type description that is displayed in the drop-down box of the export file dialog.
	/// \return A human-readable string describing the file format written by the FileExporter class.
	virtual QString fileFilterDescription() const {
		OVITO_ASSERT_MSG(false, "FileExporterClass::fileFilterDescription()", "This method should be overridden by a meta-subclass of FileExporterClass.");
		return {};
	}
};	

/**
 * \brief Abstract base class for file exporters that export data from OVITO to an external file.
 */
class OVITO_CORE_EXPORT FileExporter : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS_META(FileExporter, FileExporterClass)

public:

	/// \brief Selects the nodes from the scene to be exported by this exporter if no specific set of nodes was provided.
	virtual void selectStandardOutputData() = 0; 

	/// \brief Sets the scene objects to be exported.
	void setOutputData(const QVector<SceneNode*>& nodes);

	/// \brief Returns the list of the scene objects to be exported.
	const QVector<OORef<SceneNode>>& outputData() const { return _nodesToExport; }

	/// \brief Sets the name of the output file that should be written by this exporter.
	virtual void setOutputFilename(const QString& filename);
	
	/// \brief Exports the scene objects to the output file(s).
	/// \return \c true if the output file has been successfully written;
	///         \c false if the export operation has been canceled by the user.
	/// \throws Util::Exception if the export operation has failed due to an error.
	virtual bool exportNodes(TaskManager& taskManager);

	/// Helper function that is called by sub-classes prior to file output in order to
	/// activate the default "C" locale.
	static void activateCLocale();

protected:

	/// Initializes the object.
	FileExporter(DataSet* dataset);

	/// \brief This is called once for every output file to be written and before exportFrame() is called.
	virtual bool openOutputFile(const QString& filePath, int numberOfFrames) = 0;

	/// \brief This is called once for every output file written after exportFrame() has been called.
	virtual void closeOutputFile(bool exportCompleted) = 0;

	/// \brief Exports a single animation frame to the current output file.
	virtual bool exportFrame(int frameNumber, TimePoint time, const QString& filePath, TaskManager& taskManager);

private:

	/// The output file path.
	DECLARE_PROPERTY_FIELD(QString, outputFilename);

	/// Controls whether only the current animation frame or an entire animation interval should be exported.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, exportAnimation, setExportAnimation);

	/// Indicates that the exporter should produce a separate file for each timestep.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useWildcardFilename, setUseWildcardFilename);

	/// The wildcard name that is used to generate the output filenames.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, wildcardFilename, setWildcardFilename);

	/// The first animation frame that should be exported.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, startFrame, setStartFrame);

	/// The last animation frame that should be exported.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, endFrame, setEndFrame);

	/// Controls the interval between exported frames.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, everyNthFrame, setEveryNthFrame);

	/// Controls the desired precision with which floating-point numbers are written if the format is text-based.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, floatOutputPrecision, setFloatOutputPrecision);

	/// Holds the scene objects to be exported.
	QVector<OORef<SceneNode>> _nodesToExport;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


