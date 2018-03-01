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
#include <core/app/PluginManager.h>
#include <core/utilities/io/FileManager.h>
#include <core/utilities/concurrent/Promise.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/scene/PipelineSceneNode.h>
#include <core/dataset/animation/AnimationSettings.h>
#include "FileExporter.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

IMPLEMENT_OVITO_CLASS(FileExporter);
DEFINE_PROPERTY_FIELD(FileExporter, outputFilename);
DEFINE_PROPERTY_FIELD(FileExporter, exportAnimation);
DEFINE_PROPERTY_FIELD(FileExporter, useWildcardFilename);
DEFINE_PROPERTY_FIELD(FileExporter, wildcardFilename);
DEFINE_PROPERTY_FIELD(FileExporter, startFrame);
DEFINE_PROPERTY_FIELD(FileExporter, endFrame);
DEFINE_PROPERTY_FIELD(FileExporter, everyNthFrame);
DEFINE_PROPERTY_FIELD(FileExporter, floatOutputPrecision);
SET_PROPERTY_FIELD_LABEL(FileExporter, outputFilename, "Output filename");
SET_PROPERTY_FIELD_LABEL(FileExporter, exportAnimation, "Export animation");
SET_PROPERTY_FIELD_LABEL(FileExporter, useWildcardFilename, "Use wildcard filename");
SET_PROPERTY_FIELD_LABEL(FileExporter, wildcardFilename, "Wildcard filename");
SET_PROPERTY_FIELD_LABEL(FileExporter, startFrame, "Start frame");
SET_PROPERTY_FIELD_LABEL(FileExporter, endFrame, "End frame");
SET_PROPERTY_FIELD_LABEL(FileExporter, everyNthFrame, "Every Nth frame");
SET_PROPERTY_FIELD_LABEL(FileExporter, floatOutputPrecision, "Output precision");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(FileExporter, floatOutputPrecision, IntegerParameterUnit, 1, std::numeric_limits<FloatType>::max_digits10);

/******************************************************************************
* Constructs a new instance of the class.
******************************************************************************/
FileExporter::FileExporter(DataSet* dataset) : RefTarget(dataset),
	_exportAnimation(false),
	_useWildcardFilename(false), 
	_startFrame(0), 
	_endFrame(-1),
	_everyNthFrame(1),
	_floatOutputPrecision(10)
{
	// Use the entire animation interval as default export interval.
	setStartFrame(0);
	int lastFrame = dataset->animationSettings()->timeToFrame(dataset->animationSettings()->animationInterval().end());
	setEndFrame(lastFrame);
}

/******************************************************************************
* Sets the scene objects to be exported.
******************************************************************************/
void FileExporter::setOutputData(const QVector<SceneNode*>& nodes)
{
	_nodesToExport.clear();
	for(SceneNode* node : nodes)
		_nodesToExport.push_back(node);
}

/******************************************************************************
* Sets the name of the output file that should be written by this exporter.
******************************************************************************/
void FileExporter::setOutputFilename(const QString& filename)
{
	_outputFilename.set(this, PROPERTY_FIELD(outputFilename), filename);

	// Generate a default wildcard pattern from the filename.
	if(wildcardFilename().isEmpty()) {
		QString fn = QFileInfo(filename).fileName();
		if(!fn.contains('*')) {
			int dotIndex = fn.lastIndexOf('.');
			if(dotIndex > 0)
				setWildcardFilename(fn.left(dotIndex) + QStringLiteral(".*") + fn.mid(dotIndex));
			else
				setWildcardFilename(fn + QStringLiteral(".*"));
		}
		else
			setWildcardFilename(fn);
	}
}

/******************************************************************************
 * Exports the data of the scene nodes to one or more output files.
 *****************************************************************************/
bool FileExporter::exportNodes(TaskManager& taskManager)
{
	if(outputFilename().isEmpty())
		throwException(tr("The output filename not been set for the file exporter."));

	if(startFrame() > endFrame())
		throwException(tr("The animation interval to be exported is empty or has not been set."));

	if(outputData().empty())
		throwException(tr("There is no data to be exported."));

	// Compute the number of frames that need to be exported.
	TimePoint exportTime;
	int firstFrameNumber, numberOfFrames;
	if(exportAnimation()) {
		firstFrameNumber = startFrame();
		exportTime = dataset()->animationSettings()->frameToTime(firstFrameNumber);
		numberOfFrames = (endFrame() - startFrame() + everyNthFrame()) / everyNthFrame();
		if(numberOfFrames < 1 || everyNthFrame() < 1)
			throwException(tr("Invalid export animation range: Frame %1 to %2").arg(startFrame()).arg(endFrame()));
	}
	else {
		exportTime = dataset()->animationSettings()->time();
		firstFrameNumber = dataset()->animationSettings()->timeToFrame(exportTime);
		numberOfFrames = 1;
	}

	// Validate export settings.
	if(exportAnimation() && useWildcardFilename()) {
		if(wildcardFilename().isEmpty())
			throwException(tr("Cannot write animation frame to separate files. Wildcard pattern has not been specified."));
		if(wildcardFilename().contains(QChar('*')) == false)
			throwException(tr("Cannot write animation frames to separate files. The filename must contain the '*' wildcard character, which gets replaced by the frame number."));
	}

	Promise<> exportTask = Promise<>::createSynchronous(&taskManager, true, true);
	exportTask.setProgressText(tr("Opening output file"));

	QDir dir = QFileInfo(outputFilename()).dir();
	QString filename = outputFilename();

	// Open output file for writing.
	if(!exportAnimation() || !useWildcardFilename()) {
		if(!openOutputFile(filename, numberOfFrames))
			return false;
	}

	try {

		// Export animation frames.
		exportTask.setProgressMaximum(numberOfFrames);			
		for(int frameIndex = 0; frameIndex < numberOfFrames; frameIndex++) {
			exportTask.setProgressValue(frameIndex);

			int frameNumber = firstFrameNumber + frameIndex * everyNthFrame();

			if(exportAnimation() && useWildcardFilename()) {
				// Generate an output filename based on the wildcard pattern.
				filename = dir.absoluteFilePath(wildcardFilename());
				filename.replace(QChar('*'), QString::number(frameNumber));

				if(!openOutputFile(filename, 1))
					return false;
			}

			exportTask.setProgressText(tr("Exporting frame %1 to file '%2'").arg(frameNumber).arg(filename));

			if(!exportFrame(frameNumber, exportTime, filename, taskManager))
				exportTask.cancel();

			if(exportAnimation() && useWildcardFilename())
				closeOutputFile(!exportTask.isCanceled());

			if(exportTask.isCanceled())
				break;

			// Go to next animation frame.
			exportTime += dataset()->animationSettings()->ticksPerFrame() * everyNthFrame();
		}
	}
	catch(...) {
		closeOutputFile(false);
		throw;
	}

	// Close output file.
	if(!exportAnimation() || !useWildcardFilename()) {
		exportTask.setProgressText(tr("Closing output file"));
		closeOutputFile(!exportTask.isCanceled());
	}

	return !exportTask.isCanceled();
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool FileExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, TaskManager& taskManager)
{
	return true;
}

/******************************************************************************
* Helper function that is called by sub-classes prior to file output in order to
* activate the default "C" locale.
******************************************************************************/
void FileExporter::activateCLocale()
{
	std::setlocale(LC_ALL, "C");
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
