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

#include <plugins/pyscript/PyScript.h>
#include <plugins/pyscript/engine/ScriptEngine.h>
#include <core/dataset/io/FileImporter.h>
#include <core/dataset/io/FileExporter.h>
#include <core/dataset/io/FileSourceImporter.h>
#include <core/dataset/io/FileSource.h>
#include <core/dataset/io/AttributeFileExporter.h>
#include <core/utilities/io/FileManager.h>
#include <core/utilities/concurrent/TaskManager.h>
#include "PythonBinding.h"

namespace PyScript {

using namespace Ovito;

void defineIOSubmodule(py::module m)
{
	ovito_abstract_class<FileImporter, RefTarget>{m}
		// These are needed by implementation of import_file():
		.def("import_file", &FileImporter::importFile)
		.def_static("autodetect_format", static_cast<OORef<FileImporter> (*)(DataSet*, const QUrl&)>(&FileImporter::autodetectFileFormat))
	;

	// This is needed by implementation of import_file():
	py::enum_<FileImporter::ImportMode>(m, "ImportMode")
		.value("AddToScene", FileImporter::AddToScene)
		.value("ReplaceSelected", FileImporter::ReplaceSelected)
		.value("ResetScene", FileImporter::ResetScene)
	;

	ovito_abstract_class<FileSourceImporter, FileImporter>{m}
	;

	ovito_abstract_class<FileExporter, RefTarget>(m)
		.def_property("output_filename", &FileExporter::outputFilename, &FileExporter::setOutputFilename)
		.def_property("multiple_frames", &FileExporter::exportAnimation, &FileExporter::setExportAnimation)
		.def_property("use_wildcard_filename", &FileExporter::useWildcardFilename, &FileExporter::setUseWildcardFilename)
		.def_property("wildcard_filename", &FileExporter::wildcardFilename, &FileExporter::setWildcardFilename)
		.def_property("start_frame", &FileExporter::startFrame, &FileExporter::setStartFrame)
		.def_property("end_frame", &FileExporter::endFrame, &FileExporter::setEndFrame)
		.def_property("every_nth_frame", &FileExporter::everyNthFrame, &FileExporter::setEveryNthFrame)
		.def_property("precision", &FileExporter::floatOutputPrecision, &FileExporter::setFloatOutputPrecision)
		
		// These are required by implementation of export_file():
		.def("set_pipeline", [](FileExporter& exporter, SceneNode* node) { exporter.setOutputData({ node }); })
		.def("export_nodes", &FileExporter::exportNodes)
		.def("select_standard_output_data", &FileExporter::selectStandardOutputData)		
	;

	ovito_class<AttributeFileExporter, FileExporter>(m)
		.def_property("columns", &AttributeFileExporter::attributesToExport, &AttributeFileExporter::setAttributesToExport)
	;

	ovito_class<FileSource, CachingPipelineObject>(m, 
			"This object type can serve as a :py:attr:`Pipeline.source` and takes care of reading the input for the :py:class:`Pipeline` from an external data file. "
			"\n\n"
			"You normally do not need to create an instance of this class yourself; the :py:func:`~ovito.io.import_file` function does it for you and wires the configured :py:class:`!FileSource` "
			"to a :py:attr:`~ovito.pipeline.Pipeline`. If desired, the :py:meth:`FileSource.load` method allows you to load a different input file later on and replace the inputs of the pipeline:"
			"\n\n"
			".. literalinclude:: ../example_snippets/file_source_load_method.py\n"
			"\n"
			"Furthermore, additional :py:class:`!FileSource` instances are typically employed in conjunction with certain modifiers. "
			"The :py:class:`~ovito.modifiers.CalculateDisplacementsModifier` can make use of a :py:class:`!FileSource` to load reference particle positions from a separate input file. "
			"Another example is the :py:class:`~ovito.modifiers.LoadTrajectoryModifier`, "
			"which employs its own separate :py:class:`!FileSource` instance to load the particle trajectories from disk and combine them "
			"with the topology data previously loaded by the main :py:class:`!FileSource` of the data pipeline. "
			"\n\n"
			"**Data access**"
			"\n\n"
		    "Calling a file source's :py:meth:`.compute` method returns a new :py:class:`~ovito.data.DataCollection` containing the data that was read "
			"from the input file. Thus, this method provides access to a :py:class:`~ovito.pipeline.Pipeline`'s "
			"unmodified input data: "
			"\n\n"
			".. literalinclude:: ../example_snippets/file_source_data_access.py\n"
			"   :lines: 3-8\n"
			"\n\n")
		.def_property_readonly("importer", &FileSource::importer)
		// Required by the implementation of FileSource.source_path:
		.def("get_source_paths", &FileSource::sourceUrls)
		.def("set_source", &FileSource::setSource)
		// Required by the implementation of FileSource.load():
		.def("wait_until_ready", [](FileSource& fs, TimePoint time) {
			SharedFuture<PipelineFlowState> future = fs.evaluate(time);
			return ScriptEngine::activeTaskManager().waitForTask(future);
		})
		// Required by the implementations of import_file() and FileSource.load():
		.def("wait_for_frames_list", [](FileSource& fs) {
			auto future = fs.requestFrameList(false, false);
			return ScriptEngine::activeTaskManager().waitForTask(future);
		})
		.def_property_readonly("num_frames", &FileSource::numberOfFrames,
				"This read-only attribute reports the number of frames found in the input file or sequence of input files. "
				"The data for the individual frames can be obtained using the :py:meth:`.compute` method.")
		.def_property("adjust_animation_interval", &FileSource::adjustAnimationIntervalEnabled, &FileSource::setAdjustAnimationIntervalEnabled)
		.def_property("playback_speed_numerator", &FileSource::playbackSpeedNumerator, &FileSource::setPlaybackSpeedNumerator)
		.def_property("playback_speed_denominator", &FileSource::playbackSpeedDenominator, &FileSource::setPlaybackSpeedDenominator)
		.def_property("playback_start_time", &FileSource::playbackStartTime, &FileSource::setPlaybackStartTime)

		.def_property_readonly("data", &FileSource::dataCollection,
			"This field exposes the internal :py:class:`~ovito.data.DataCollection` of the source object holding "
			"the master data copy currently loaded from the input file. ")
		
		// For backward compatibility with OVITO 2.9:
		// Returns the zero-based frame index that is currently loaded and kept in memory by the FileSource.
		.def_property_readonly("loaded_frame", &FileSource::storedFrameIndex)
		// For backward compatibility with OVITO 2.9:
		.def_property_readonly("loaded_file", [](FileSource& fs) -> QUrl {
					// Return the path or URL of the data file that is currently loaded and kept in memory by the FileSource. 
					if(fs.storedFrameIndex() < 0 || fs.storedFrameIndex() >= fs.frames().size()) return QUrl();
					const FileSourceImporter::Frame& frameInfo = fs.frames()[fs.storedFrameIndex()];
					return frameInfo.sourceFile;
				})
	;
}

};
