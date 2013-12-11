///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <core/scene/ObjectNode.h>
#include <core/scene/SceneRoot.h>
#include <core/scene/objects/SceneObject.h>
#include <core/animation/AnimationSettings.h>
#include <core/gui/mainwin/MainWindow.h>

#include <plugins/particles/data/ParticlePropertyObject.h>
#include "ParticleExporter.h"

namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, ParticleExporter, FileExporter)
DEFINE_PROPERTY_FIELD(ParticleExporter, _outputFilename, "OutputFile")
DEFINE_PROPERTY_FIELD(ParticleExporter, _exportAnimation, "ExportAnimation")
DEFINE_PROPERTY_FIELD(ParticleExporter, _useWildcardFilename, "UseWildcardFilename")
DEFINE_PROPERTY_FIELD(ParticleExporter, _wildcardFilename, "WildcardFilename")
DEFINE_PROPERTY_FIELD(ParticleExporter, _startFrame, "StartFrame")
DEFINE_PROPERTY_FIELD(ParticleExporter, _endFrame, "EndFrame")
DEFINE_PROPERTY_FIELD(ParticleExporter, _everyNthFrame, "EveryNthFrame")
SET_PROPERTY_FIELD_LABEL(ParticleExporter, _outputFilename, "Output filename")
SET_PROPERTY_FIELD_LABEL(ParticleExporter, _exportAnimation, "Export animation")
SET_PROPERTY_FIELD_LABEL(ParticleExporter, _useWildcardFilename, "Use wildcard filename")
SET_PROPERTY_FIELD_LABEL(ParticleExporter, _wildcardFilename, "Wildcard filename")
SET_PROPERTY_FIELD_LABEL(ParticleExporter, _startFrame, "Start frame")
SET_PROPERTY_FIELD_LABEL(ParticleExporter, _endFrame, "End frame")
SET_PROPERTY_FIELD_LABEL(ParticleExporter, _everyNthFrame, "Every Nth frame")

/******************************************************************************
* Constructs a new instance of the class.
******************************************************************************/
ParticleExporter::ParticleExporter(DataSet* dataset) : FileExporter(dataset),
	_exportAnimation(false),
	_useWildcardFilename(false), _startFrame(0), _endFrame(-1),
	_everyNthFrame(1), _compressor(&_outputFile)
{
	INIT_PROPERTY_FIELD(ParticleExporter::_outputFilename);
	INIT_PROPERTY_FIELD(ParticleExporter::_exportAnimation);
	INIT_PROPERTY_FIELD(ParticleExporter::_useWildcardFilename);
	INIT_PROPERTY_FIELD(ParticleExporter::_wildcardFilename);
	INIT_PROPERTY_FIELD(ParticleExporter::_startFrame);
	INIT_PROPERTY_FIELD(ParticleExporter::_endFrame);
	INIT_PROPERTY_FIELD(ParticleExporter::_everyNthFrame);
}

/******************************************************************************
* Sets the name of the output file that should be written by this exporter.
******************************************************************************/
void ParticleExporter::setOutputFilename(const QString& filename)
{
	_outputFilename = filename;

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
* Exports the scene to the given file.
******************************************************************************/
bool ParticleExporter::exportToFile(const QString& filePath)
{
	// Save the output path.
	setOutputFilename(filePath);

	// Get the data to be exported.
	PipelineFlowState flowState = getParticles(dataset()->animationSettings()->time());
	if(flowState.isEmpty())
		throw Exception(tr("The scene does not contain any particles that can be exported."));

	// Use the entire animation as default export interval if no interval has been set before.
	if(startFrame() > endFrame()) {
		setStartFrame(0);
		int lastFrame = dataset()->animationSettings()->timeToFrame(dataset()->animationSettings()->animationInterval().end());
		setEndFrame(lastFrame);
	}

	// Show optional export settings dialog.
	if(!showSettingsDialog(flowState, dataset()->mainWindow()))
		return false;

	// Perform the actual export operation.
	return writeOutputFiles();
}

/******************************************************************************
* Retrieves the particles to be exported by evaluating the modification pipeline.
******************************************************************************/
PipelineFlowState ParticleExporter::getParticles(TimePoint time)
{
	// Iterate over all scene nodes.
	for(SceneNodesIterator iter(dataset()->sceneRoot()); !iter.finished(); iter.next()) {

		ObjectNode* node = dynamic_object_cast<ObjectNode>(iter.current());
		if(!node) continue;

		// Check if the node's pipeline evaluates to something that contains particles.
		const PipelineFlowState& state = node->evalPipeline(time);
		for(const auto& o : state.objects()) {
			ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o.get());
			if(property && property->type() == ParticleProperty::PositionProperty) {
				return state;
			}
		}
	}

	// Nothing to export.
	return PipelineFlowState();
}

/******************************************************************************
 * Exports the particles contained in the given scene to the output file(s).
 *****************************************************************************/
bool ParticleExporter::writeOutputFiles()
{
	OVITO_ASSERT_MSG(!outputFilename().isEmpty(), "ParticleExporter::exportParticles()", "Output filename has not been set. ParticleExporter::setOutputFilename() must be called first.");
	OVITO_ASSERT_MSG(startFrame() <= endFrame(), "ParticleExporter::exportParticles()", "Export interval has not been set. ParticleExporter::setStartFrame() and ParticleExporter::setEndFrame() must be called first.");

	if(startFrame() > endFrame())
		throw Exception(tr("The animation interval to be exported is empty or has not been set."));

	// Show progress dialog.
	QProgressDialog progressDialog(dataset()->mainWindow());
	progressDialog.setWindowModality(Qt::WindowModal);
	progressDialog.setAutoClose(false);
	progressDialog.setAutoReset(false);
	progressDialog.setMinimumDuration(0);

	// Compute the number of frames that need to be exported.
	TimePoint exportTime;
	int firstFrameNumber, numberOfFrames;
	if(_exportAnimation) {
		firstFrameNumber = startFrame();
		exportTime = dataset()->animationSettings()->frameToTime(firstFrameNumber);
		numberOfFrames = (endFrame() - startFrame() + everyNthFrame()) / everyNthFrame();
		if(numberOfFrames < 1 || everyNthFrame() < 1)
			throw Exception(tr("Invalid export animation range: Frame %1 to %2").arg(startFrame()).arg(endFrame()));
	}
	else {
		exportTime = dataset()->animationSettings()->time();
		firstFrameNumber = dataset()->animationSettings()->timeToFrame(exportTime);
		numberOfFrames = 1;
	}

	// Validate export settings.
	if(_exportAnimation && useWildcardFilename()) {
		if(wildcardFilename().isEmpty())
			throw Exception(tr("Cannot write animation frame to separate files. No wildcard pattern has been specified."));
		if(wildcardFilename().contains(QChar('*')) == false)
			throw Exception(tr("Cannot write animation frames to separate files. The filename must contain the '*' wildcard character, which gets replaced by the frame number."));
	}

	progressDialog.setMaximum(numberOfFrames * 100);
	QDir dir = QFileInfo(outputFilename()).dir();
	QString filename = outputFilename();

	// Open output file for writing.
	if(!_exportAnimation || !useWildcardFilename()) {
		if(!openOutputFile(filename, numberOfFrames))
			return false;
	}

	try {

		// Export animation frames.
		for(int frameIndex = 0; frameIndex < numberOfFrames; frameIndex++) {
			progressDialog.setValue(frameIndex * 100);

			int frameNumber = firstFrameNumber + frameIndex * everyNthFrame();

			if(_exportAnimation && useWildcardFilename()) {
				// Generate an output filename based on the wildcard pattern.
				filename = dir.absoluteFilePath(wildcardFilename());
				filename.replace(QChar('*'), QString::number(frameNumber));

				if(!openOutputFile(filename, 1))
					return false;
			}

			if(!exportFrame(frameNumber, exportTime, filename, progressDialog))
				progressDialog.cancel();

			if(_exportAnimation && useWildcardFilename())
				closeOutputFile(!progressDialog.wasCanceled());

			if(progressDialog.wasCanceled())
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
	if(!_exportAnimation || !useWildcardFilename()) {
		closeOutputFile(!progressDialog.wasCanceled());
	}

	return true;
}

/******************************************************************************
 * This is called once for every output file to be written and before exportParticles() is called.
 *****************************************************************************/
bool ParticleExporter::openOutputFile(const QString& filePath, int numberOfFrames)
{
	OVITO_ASSERT(!_outputFile.isOpen());

	_outputFile.setFileName(filePath);

	// Automatically write a gzipped file if filename ends with .gz suffix.
	if(filePath.endsWith(".gz", Qt::CaseInsensitive)) {

		// Open compressed file for writing.
		_compressor.setStreamFormat(QtIOCompressor::GzipFormat);
		if(!_compressor.open(QIODevice::WriteOnly))
			throw Exception(tr("Failed to open file '%1' for writing: %2").arg(filePath).arg(_compressor.errorString()));

		_textStream.setDevice(&_compressor);
	}
	else {
		if(!_outputFile.open(QIODevice::WriteOnly | QIODevice::Text))
			throw Exception(tr("Failed to open file '%1' for writing: %2").arg(filePath).arg(_outputFile.errorString()));

		_textStream.setDevice(&_outputFile);
	}
	_textStream.setRealNumberPrecision(10);

	return true;
}

/******************************************************************************
 * This is called once for every output file written after exportParticles() has been called.
 *****************************************************************************/
void ParticleExporter::closeOutputFile(bool exportCompleted)
{
	if(_compressor.isOpen())
		_compressor.close();
	if(_outputFile.isOpen())
		_outputFile.close();

	if(!exportCompleted)
		_outputFile.remove();
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool ParticleExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, QProgressDialog& progressDialog)
{
	// Jump to animation time.
	dataset()->animationSettings()->setTime(time);

	// Wait until the scene is ready.
	volatile bool sceneIsReady = false;
	dataset()->runWhenSceneIsReady( [&sceneIsReady]() { sceneIsReady = true; } );
	if(!sceneIsReady) {
		progressDialog.setLabelText(tr("Preparing frame %1 for export...").arg(frameNumber));
		while(!sceneIsReady) {
			if(progressDialog.wasCanceled())
				return false;
			QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 200);
		}
	}
	progressDialog.setLabelText(tr("Exporting frame %1 to file '%2'.").arg(frameNumber).arg(filePath));

	// Evaluate modification pipeline to get the particles to be exported.
	PipelineFlowState state = getParticles(time);
	if(state.isEmpty())
		throw Exception(tr("The scene does not contain any particles that can be exported."));

	ProgressInterface progressInterface(progressDialog);
	return exportParticles(state, frameNumber, time, filePath, progressInterface);
}

/******************************************************************************
* Retrieves the given standard particle property from the pipeline flow state.
******************************************************************************/
ParticlePropertyObject* ParticleExporter::findStandardProperty(ParticleProperty::Type type, const PipelineFlowState& flowState)
{
	for(const auto& sceneObj : flowState.objects()) {
		ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(sceneObj.get());
		if(property && property->type() == type) return property;
	}
	return nullptr;
}

};