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
DEFINE_REFERENCE_FIELD(FileExporter, nodeToExport);
DEFINE_PROPERTY_FIELD(FileExporter, dataObjectToExport);
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
	int lastFrame = dataset->animationSettings()->timeToFrame(dataset->animationSettings()->animationInterval().end());
	setEndFrame(lastFrame);
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
* Selects the default scene node to be exported by this exporter.
******************************************************************************/
void FileExporter::selectDefaultExportableData()
{
	// By default, export the data of the selected pipeline.
	if(!nodeToExport()) {
		if(SceneNode* selectedNode = dataset()->selection()->firstNode()) {
			if(isSuitableNode(selectedNode)) {
				setNodeToExport(selectedNode);
			}
		}
	}

	// If no scene node is currently selected, pick the first suitable node from the scene. 
	if(!nodeToExport()) {
		if(isSuitableNode(dataset()->sceneRoot())) {
			setNodeToExport(dataset()->sceneRoot());
		}
		else {
			dataset()->sceneRoot()->visitChildren([this](SceneNode* node) {
				if(isSuitableNode(node)) {
					setNodeToExport(node);
					return false;
				}
				return true;
			});
		}
	}
}

/******************************************************************************
* Determines whether the given scene node is suitable for exporting with this 
* exporter service. By default, all pipeline scene nodes are considered suitable 
* that produce suitable data objects of the type specified by the 
* FileExporter::exportableDataObjectClass() method. 
******************************************************************************/
bool FileExporter::isSuitableNode(SceneNode* node) const 
{
	if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(node)) {
		return isSuitablePipelineOutput(pipeline->evaluatePipelinePreliminary(true));
	}
	return false;
}

/******************************************************************************
* Determines whether the given pipeline output is suitable for exporting with 
* this exporter service. By default, all data collections are considered suitable
* that contain suitable data objects of the type specified by the 
* FileExporter::exportableDataObjectClass() method.
******************************************************************************/
bool FileExporter::isSuitablePipelineOutput(const PipelineFlowState& state) const 
{ 
	if(state.isEmpty()) return false;
	std::vector<const DataObject::OOMetaClass*> objClasses = exportableDataObjectClass();
	if(objClasses.empty()) 
		return true;
	for(const DataObject::OOMetaClass* objClass : objClasses) {
		if(state.containsObjectRecursive(*objClass))
			return true;
	}
	return false;
}

/******************************************************************************
* Evaluates the pipeline whose data is to be exported.
******************************************************************************/
PipelineFlowState FileExporter::getPipelineDataToBeExported(TimePoint time, AsyncOperation& operation, bool requestRenderState) const
{
	PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(nodeToExport());
	if(!pipeline)
		throwException(tr("The scene object to be exported is not a data pipeline."));

	// Evaluate pipeline.
	auto evalFuture = requestRenderState ? pipeline->evaluateRenderingPipeline(time) : pipeline->evaluatePipeline(time);
	if(!operation.waitForFuture(evalFuture))
		return {};
	PipelineFlowState state = evalFuture.result();
	if(state.isEmpty())
		throwException(tr("The data collection to be exported is empty."));

	return state;
}

/******************************************************************************
 * Exports the scene data to the output file(s).
 *****************************************************************************/
bool FileExporter::doExport(AsyncOperation&& operation)
{
	if(outputFilename().isEmpty())
		throwException(tr("The output filename not been set for the file exporter."));

	if(startFrame() > endFrame())
		throwException(tr("The animation interval to be exported is empty or has not been set."));

	if(!nodeToExport())
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
		if(!wildcardFilename().contains(QChar('*')))
			throwException(tr("Cannot write animation frames to separate files. The filename must contain the '*' wildcard character, which gets replaced by the frame number."));
	}

	operation.setProgressText(tr("Opening output file"));

	QDir dir = QFileInfo(outputFilename()).dir();
	QString filename = outputFilename();

	// Open output file for writing.
	if(!exportAnimation() || !useWildcardFilename()) {
		if(!openOutputFile(filename, numberOfFrames, operation))
			return false;
	}

	try {

		// Export animation frames.
		operation.setProgressMaximum(numberOfFrames);			
		for(int frameIndex = 0; frameIndex < numberOfFrames; frameIndex++) {
			operation.setProgressValue(frameIndex);

			int frameNumber = firstFrameNumber + frameIndex * everyNthFrame();

			if(exportAnimation() && useWildcardFilename()) {
				// Generate an output filename based on the wildcard pattern.
				filename = dir.absoluteFilePath(wildcardFilename());
				filename.replace(QChar('*'), QString::number(frameNumber));

				if(!openOutputFile(filename, 1, operation))
					return false;
			}

			operation.setProgressText(tr("Exporting frame %1 to file '%2'").arg(frameNumber).arg(filename));

			exportFrame(frameNumber, exportTime, filename, operation.createSubOperation());

			if(exportAnimation() && useWildcardFilename())
				closeOutputFile(!operation.isCanceled());

			if(operation.isCanceled())
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
		operation.setProgressText(tr("Closing output file"));
		closeOutputFile(!operation.isCanceled());
	}

	return !operation.isCanceled();
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool FileExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation)
{
	return !operation.isCanceled();
}

/******************************************************************************
* Helper function that is called by sub-classes prior to file output in order to
* activate the default "C" locale.
******************************************************************************/
void FileExporter::activateCLocale()
{
	std::setlocale(LC_ALL, "C");
}

/******************************************************************************
* Returns a string with the list of available data objects of the given type.
******************************************************************************/
QString FileExporter::getAvailableDataObjectList(const PipelineFlowState& state, const DataObject::OOMetaClass& objectType) const
{
	QString str;
	if(!state.isEmpty()) {
		for(const ConstDataObjectPath& dataPath : state.data()->getObjectsRecursive(objectType)) {
			QString pathString = dataPath.toString();
			if(!pathString.isEmpty()) {
				if(!str.isEmpty()) str += QStringLiteral(", ");
				str += pathString;
			}
		}
	}
	if(str.isEmpty())
		str = tr("<none>");
	return str;
}


OVITO_END_INLINE_NAMESPACE
}	// End of namespace
