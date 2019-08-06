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

#include <plugins/pyscript/PyScript.h>
#include <plugins/pyscript/engine/ScriptEngine.h>
#include <core/dataset/io/FileImporter.h>
#include <core/dataset/io/FileExporter.h>
#include <core/dataset/io/FileSourceImporter.h>
#include <core/dataset/io/FileSource.h>
#include <core/dataset/io/AttributeFileExporter.h>
#include <core/dataset/DataSetContainer.h>
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
		.def_static("autodetect_format", [](DataSet& dataset, const QUrl& url) {
			auto future = FileImporter::autodetectFileFormat(&dataset, url);
			// Block until detection is complete and result is available.
			if(!ScriptEngine::waitForFuture(future)) {
				PyErr_SetString(PyExc_KeyboardInterrupt, "Operation has been canceled by the user.");
				throw py::error_already_set();
			}
			return future.result();
		})
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
		.def_property("ignore_pipeline_errors", &FileExporter::ignorePipelineErrors, &FileExporter::setIgnorePipelineErrors)

		// These are required by implementation of export_file():
		.def_property("pipeline", &FileExporter::nodeToExport, &FileExporter::setNodeToExport)
		.def_property("key",
			[](const FileExporter& exporter) {
				return exporter.dataObjectToExport().dataPath();
			},
			[](FileExporter& exporter, const QString& path) {
				exporter.setDataObjectToExport(DataObjectReference(&DataObject::OOClass(), path));
			})
		.def("do_export", [](FileExporter& exporter) {
				if(!exporter.doExport(ScriptEngine::currentTask()->createSubTask())) {
					PyErr_SetString(PyExc_KeyboardInterrupt, "Operation has been canceled by the user.");
					throw py::error_already_set();
				}
			})
		.def("select_default_exportable_data", &FileExporter::selectDefaultExportableData)
	;

	ovito_class<AttributeFileExporter, FileExporter>(m)
		.def_property("columns", &AttributeFileExporter::attributesToExport, &AttributeFileExporter::setAttributesToExport)
	;

	ovito_class<FileSource, CachingPipelineObject>(m,
			"This object type serves as a :py:attr:`Pipeline.source` and takes care of reading the input data for a :py:class:`Pipeline` from an external file. "
			"\n\n"
			"You normally do not need to create an instance of this class yourself; the :py:func:`~ovito.io.import_file` function does it for you and wires the fully configured :py:class:`!FileSource` "
			"to the new :py:attr:`~ovito.pipeline.Pipeline`. However, if needed, the :py:meth:`FileSource.load` method allows you to load a different input file later on and replace the "
			"input of the existing pipeline with a new dataset: "
			"\n\n"
			".. literalinclude:: ../example_snippets/file_source_load_method.py\n"
			"\n"
			"Furthermore, you will encounter other :py:class:`!FileSource` objects in conjunction with certain modifiers that need secondary input data from a separate file. "
			"The :py:class:`~ovito.modifiers.CalculateDisplacementsModifier`, for example, manages its own :py:class:`!FileSource` for loading reference particle positions from a separate input file. "
			"Another example is the :py:class:`~ovito.modifiers.LoadTrajectoryModifier`, "
			"which employs its own separate :py:class:`!FileSource` instance to load the particle trajectories from disk and combine them "
			"with the topology data previously loaded by the main :py:class:`!FileSource` of the data pipeline. "
			"\n\n"
			"**Data access**"
			"\n\n"
			"The :py:class:`!FileSource` class provides two ways of accessing the data that is loaded from the external input file(s). "
			"For read-only access to the data, the :py:meth:`FileSource.compute` method should be called. It loads the data of a specific frame "
			"from the input simulation trajectory and returns it as a new :py:class:`~ovito.data.DataCollection` object: "
			"\n\n"
			".. literalinclude:: ../example_snippets/file_source_data_access.py\n"
			"   :lines: 4-9\n"
			"\n\n"
			"Alternatively, you can directly manipulate the data objects that are stored in the internal cache of the "
			":py:class:`!FileSource`, which is accessible through its :py:attr:`.data` field. The objects in this :py:class:`~ovito.data.DataCollection` "
			"may be manipulated, which sometimes is needed to amend the data entering the pipeline with additional information. "
			"A typical use case is setting the radii and names of the particle types that have been loaded from a simulation file that doesn't contain named atom types: "
			"\n\n"
			".. literalinclude:: ../example_snippets/file_source_data_access.py\n"
			"   :lines: 14-22\n"
			"\n\n"
			"Any changes you make to the data objects in the cache data collection will be seen by modifiers in the pipeline that "
			"is supplied by the :py:class:`!FileSource`. However, those changes may be overwritten again if the same information is already present in the "
			"input file(s). That means, for example, modifying the cached particle positions will have no permanent effect, because they will "
			"likely be replaced with the data parsed from the input file. ")
		.def_property_readonly("importer", &FileSource::importer)
		// Required by the implementation of FileSource.source_path:
		.def("get_source_paths", &FileSource::sourceUrls)
		.def("set_source", &FileSource::setSource)
		// Required by the implementation of FileSource.load():
		.def("wait_until_ready", [](FileSource& fs, TimePoint time) {
			SharedFuture<PipelineFlowState> future = fs.evaluate(time);
			return ScriptEngine::waitForFuture(future);
		})
		// Required by the implementations of import_file() and FileSource.load():
		.def("wait_for_frames_list", [](FileSource& fs) {
			auto future = fs.requestFrameList(false, false);
			return ScriptEngine::waitForFuture(future);
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
			"the master copy of the data loaded from the input file (at frame 0). ")

		// For backward compatibility with OVITO 2.9.0:
		// Returns the zero-based frame index that is currently loaded and kept in memory by the FileSource.
		.def_property_readonly("loaded_frame", &FileSource::storedFrameIndex)
		// For backward compatibility with OVITO 2.9.0:
		.def_property_readonly("loaded_file", [](FileSource& fs) -> QUrl {
					// Return the path or URL of the data file that is currently loaded and kept in memory by the FileSource.
					if(fs.storedFrameIndex() < 0 || fs.storedFrameIndex() >= fs.frames().size()) return QUrl();
					const FileSourceImporter::Frame& frameInfo = fs.frames()[fs.storedFrameIndex()];
					return frameInfo.sourceFile;
				})
	;
}

};
