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
		.def_static("autodetect_format", (OORef<FileImporter> (*)(DataSet*, const QUrl&))&FileImporter::autodetectFileFormat)
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

	auto FileSource_py = ovito_class<FileSource, CachingPipelineObject>(m, 
			":Base class: :py:class:`ovito.data.DataCollection`\n\n"
			"This object type can serve as a :py:attr:`Pipeline.source` and takes care of reading the input for the :py:class:`Pipeline` from an external data file. "
			"\n\n"
			"You normally do not need to create an instance of this class yourself; the :py:func:`~ovito.io.import_file` function does it for you and wires the configured :py:class:`!FileSource` "
			"to the :py:attr:`~ovito.pipeline.Pipeline`. If desired, the :py:meth:`FileSource.load` method allows you to load a different input file later on and replace the inputs of the pipeline:"
			"\n\n"
			".. literalinclude:: ../example_snippets/file_source_load_method.py\n"
			"\n"
			"Furthermore, additional :py:class:`!FileSource` instances are typically used in conjunction with certain modifiers. "
			"The :py:class:`~ovito.modifiers.CalculateDisplacementsModifier` can make use of a :py:class:`!FileSource` to load reference particle positions from a separate input file. "
			"Another example is the :py:class:`~ovito.modifiers.LoadTrajectoryModifier`, "
			"which employs its own separate :py:class:`!FileSource` instance to load the particle trajectories from disk and combine them "
			"with the topology data previously loaded by the main :py:class:`!FileSource` of the data pipeline. "
			"\n\n"
			"**Data access**"
			"\n\n"
		    "You can call a file source's :py:meth:`.compute` method to retrieve a :py:class:`~ovito.data.PipelineFlowState` containing the data that was read "
			"from the input file. Typically you would use this method to access a :py:class:`~ovito.pipeline.Pipeline`'s "
			"unmodified input data: "
			"\n\n"
			".. literalinclude:: ../example_snippets/file_source_data_access.py\n"
			"   :lines: 3-8\n"
			"\n\n"
		    "Note that the :py:class:`!FileSource` object itself derives from the :py:class:`~ovito.data.DataCollection` base class. "
			"It operates like a cache for the frame data that was read from the external file during the most recent load operation. " 
			"You may directly access this cached data through the methods and properties inherited from the :py:class:`~ovito.data.DataCollection` interface: "
			"\n\n"
			".. literalinclude:: ../example_snippets/file_source_data_access.py\n"
			"   :lines: 10-11\n"
			)
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
		.def_property("adjust_animation_interval", &FileSource::adjustAnimationIntervalEnabled, &FileSource::setAdjustAnimationIntervalEnabled,
				"This Boolean flag controls whether the global animation length in OVITO is automatically adjusted to match the :py:attr:`.num_frames` value "
				"of this :py:class:`!FileSource`."
				"\n\n"
				"The length of the current animation interval is managed by the global :py:class:`~ovito.anim.AnimationSettings` object. "
				"The current animation interval determines the length of movies rendered by OVITO and the length of the timeline that is displayed in the "
				"graphical program version of OVITO. "
				"If :py:attr:`!adjust_animation_interval` is ``True``, then the global animation length will automatically be adjusted to match the "
				":py:attr:`.num_frames` value of this :py:class:`!FileSource`. "
				"\n\n"
				"Keep in mind that is possible to have multiple data pipelines in the same scene, each having its own :py:class:`!FileSource`. "
				"By default, OVITO sets :py:attr:`!adjust_animation_interval` to ``True`` only for the very first :py:class:`!FileSource` that is created ("
				"typically through a call to :py:func:`~ovito.io.import_file`). The animation length will subsequently be slaved to the number of frames "
				"loaded by this master :py:class:`!FileSource`. In some situations it makes sense to turn the :py:attr:`!adjust_animation_interval` option off again "
				"and adjust the animation length manually by modifying the global :py:class:`~ovito.anim.AnimationSettings` object directly. ")

		// The following methods are required for the DataCollection.attributes property.
		.def_property_readonly("attribute_names", [](FileSource& obj) -> QStringList {
				return obj.attributes().keys();
			})
		.def("get_attribute", [](FileSource& obj, const QString& attrName) -> py::object {
				auto item = obj.attributes().find(attrName);
				if(item == obj.attributes().end())
					return py::none();
				else
					return py::cast(item.value());
			})
		.def("set_attribute", [](FileSource& obj, const QString& attrName, py::object value) {
				obj.throwException("Attributes of a FileSource are read-only.");
			})
				
	;
	expose_mutable_subobject_list(FileSource_py,
		std::mem_fn(&FileSource::dataObjects), 
		std::mem_fn(&FileSource::insertDataObject), 
		std::mem_fn(&FileSource::removeDataObject), "objects", "FileSourceDataObjectList");
}

};
