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
			"You normally do not need to create an instance of this class; the :py:func:`~ovito.io.import_file` function does it for you and wires the configured :py:class:`!FileSource` "
			"to the :py:attr:`~ovito.pipeline.Pipeline`. Subsequently, the :py:meth:`FileSource.load` method allows you to load a different input file if needed:"
			"\n\n"
			".. literalinclude:: ../example_snippets/file_source_load_method.py\n"
			"\n"
			"File sources can furthermore be used in conjunction with certain modifiers. "
			"The :py:class:`~ovito.modifiers.CalculateDisplacementsModifier` can make use of a :py:class:`!FileSource` to load a reference configuration from a separate input file. "
			"Another example is the :py:class:`~ovito.modifiers.LoadTrajectoryModifier`, "
			"which employs its own :py:class:`!FileSource` instance to load the particle trajectories from disk. "
			"\n\n"
			"**Data access**"
			"\n\n"
		    "The :py:class:`!FileSource` class derives from the :py:class:`~ovito.data.DataCollection` base class. "
			"This means it also functions as a container for data objects, which were loaded from the external file and then cached in the :py:class:`!FileSource` object. "
			"You can directly access the cached data through the methods and properties inherited from the :py:class:`~ovito.data.DataCollection` interface. "
			"Note that the :py:class:`!FileSource`'s contents reflect only the outcome of the most recent load operation. "
			"\n\n"
			".. literalinclude:: ../example_snippets/file_source_data_access.py\n"
			)
		.def_property_readonly("importer", &FileSource::importer)
		.def_property_readonly("source_path", &FileSource::sourceUrl)
		.def("set_source", &FileSource::setSource)
		// Required by the implementation of FileSource.load():
		.def("wait_until_ready", [](FileSource& fs, TimePoint time) {
			SharedFuture<PipelineFlowState> future = fs.evaluate(time);
			return ScriptEngine::activeTaskManager().waitForTask(future);
		})
		.def_property_readonly("num_frames", &FileSource::numberOfFrames,
				"The number of frames the imported file or file sequence contains (read-only).")
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
				"A Boolean flag that controls whether the animation length in OVITO is automatically adjusted to match the number of frames in the "
				"loaded file or file sequence."
				"\n\n"
				"The current length of the animation in OVITO is managed by the global :py:class:`~ovito.anim.AnimationSettings` object. The number of frames in the external file "
				"or file sequence is indicated by the :py:attr:`.num_frames` attribute of this :py:class:`!FileSource`. If :py:attr:`.adjust_animation_interval` "
				"is ``True``, then the animation length will be automatically adjusted to match the number of frames provided by the :py:class:`!FileSource`. "
				"\n\n"
				"In some situations it makes sense to turn this option off, for example, if you import several data files into "
				"OVITO simultaneously, but their frame counts do not match. "
				"\n\n"
				":Default: ``True``\n")

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
