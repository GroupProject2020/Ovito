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
#include <core/oo/CloneHelper.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/scene/SceneNode.h>
#include <core/dataset/scene/PipelineSceneNode.h>
#include <core/dataset/scene/RootSceneNode.h>
#include <core/dataset/scene/SelectionSet.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/rendering/RenderSettings.h>
#include <core/utilities/concurrent/TaskManager.h>
#include "PythonBinding.h"

namespace PyScript {

using namespace Ovito;

void defineAppSubmodule(py::module m)
{
	py::class_<OvitoObject, OORef<OvitoObject>>(m, "OvitoObject")
		.def("__str__", [](py::object& pyobj) {
			return py::str("<{} at 0x{:x}>").format(pyobj.attr("__class__").attr("__name__"), (std::intptr_t)pyobj.cast<OvitoObject*>());
		})
		.def("__repr__", [](py::object& pyobj) {
			return py::str("{}()").format(pyobj.attr("__class__").attr("__name__"));
		})
		.def("__eq__", [](OvitoObject* o, py::object& other) {
			try { return o == other.cast<OvitoObject*>(); }
			catch(const py::cast_error&) { return false; }
		})
		.def("__ne__", [](OvitoObject* o, py::object& other) {
			try { return o != other.cast<OvitoObject*>(); }
			catch(const py::cast_error&) { return true; }
		})
	;

	ovito_abstract_class<RefMaker, OvitoObject>{m}
		.def_property_readonly("dataset", 
			[](RefMaker& obj) {
				return obj.dataset().data();
			}, py::return_value_policy::reference)
	;

	ovito_abstract_class<RefTarget, RefMaker>{m}
		// This is used by DataCollection.__getitem__():
		.def_property_readonly("object_title", &RefTarget::objectTitle)
		// This internal method is used in various places:
		.def("notify_object_changed", [](RefTarget& target) { target.notifyTargetChanged(); })
	;

	// Note that, for DataSet, we are not using OORef<> as holder type like we normally do for other OvitoObject-derived classes, because
	// we don't want a ScriptEngine to hold a counted reference to a DataSet that it belongs to.
	// This would create a cyclic reference and potentially lead to a memory leak.
	py::class_<DataSet>(m, "Scene",
			"This class encompasses all data of an OVITO program session (basically everything that gets saved in a ``.ovito`` state file). "
			"It provides access to the objects that are part of the three-dimensional scene. "
			"\n\n"
			"From a script's point of view, there exists exactly one universal instance of this class at any time, which can be accessed through "
			"the :py:data:`ovito.scene` module-level variable. A script cannot create another :py:class:`!Scene` instance. ")
		.def_property_readonly("scene_root", &DataSet::sceneRoot)
		// For backward compatibility with OVITO 2.9.0:
		.def_property_readonly("anim", &DataSet::animationSettings)
		.def_property_readonly("viewports", &DataSet::viewportConfig)
		.def_property_readonly("render_settings", &DataSet::renderSettings)
		.def("save", &DataSet::saveToFile,
			"save(filename)"
			"\n\n"
		    "Saves the scene including all data pipelines that are currently in it to an OVITO state file. "
    		"This function works like the *Save State As* menu function of OVITO. Note that pipelines that have not been added to the scene "
			"will not be saved to the state file. "
			"\n\n"
			":param str filename: The output file path\n"
			"\n\n"
			"The saved program state can be loaded again using the :command:`-o` :ref:`command line option <preloading_program_state>` of :program:`ovitos` "
			"or in the `graphical version of OVITO <../../usage.import.html#usage.import.command_line>`__. "
			"After loading the state file, the :py:attr:`.pipelines` list will contain again all :py:class:`~ovito.pipeline.Pipeline` objects "
			"that were part of the scene when it was saved. See also :py:ref:`here <saving_loading_pipelines>`."
			,
			py::arg("filename"))
		// This is needed for the Scene.selected_pipeline attribute:
		.def_property_readonly("selection", &DataSet::selection)
		// This is needed by Viewport.render_image() and Viewport.render_anim():
		.def("render_scene", [](DataSet& dataset, RenderSettings& settings, Viewport& viewport, FrameBuffer& frameBuffer) {
				if(!dataset.renderScene(&settings, &viewport, &frameBuffer, ScriptEngine::getCurrentDataset()->taskManager())) {
					PyErr_SetString(PyExc_KeyboardInterrupt, "Operation has been canceled by the user.");
					throw py::error_already_set();
				}
			})
		.def_property_readonly("container", &DataSet::container, py::return_value_policy::reference)

		// This is called by various Python functions that perform long-running operations.
		.def("request_long_operation", [](DataSet& ds) {
				if(ds.viewportConfig() && ds.viewportConfig()->isRendering()) {
					throw Exception(DataSet::tr("This operation is not permitted while viewport rendering is in progress. "
							"Your script called an OVITO function that triggers a potentially long-running operation. "
							"In order to not block the user interface, such operations are not allowed during interactice viewport rendering."), &ds);
				}
			})
	;

	py::class_<DataSetContainer>{m, "DataSetContainer"}
	;

	py::class_<CloneHelper>(m, "CloneHelper")
		.def(py::init<>())
		.def("clone", py::overload_cast<const RefTarget*, bool>(&CloneHelper::cloneObject<RefTarget>))
	;
}

}
