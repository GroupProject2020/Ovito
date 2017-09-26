///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include <core/dataset/scene/SceneNode.h>
#include <core/dataset/scene/ObjectNode.h>
#include <core/dataset/scene/SceneRoot.h>
#include <core/dataset/scene/SelectionSet.h>
#include <core/dataset/data/DataObject.h>
#include <core/dataset/pipeline/Modifier.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/pipeline/AsynchronousModifierApplication.h>
#include <core/dataset/pipeline/PipelineObject.h>
#include <core/dataset/pipeline/StaticSource.h>
#include <core/dataset/pipeline/DelegatingModifier.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/app/PluginManager.h>
#include <core/utilities/concurrent/TaskManager.h>
#include <plugins/pyscript/extensions/PythonScriptModifier.h>
#include "PythonBinding.h"

namespace PyScript {

using namespace Ovito;

void defineSceneSubmodule(py::module m)
{
	auto PipelineStatus_py = py::class_<PipelineStatus>(m, "PipelineStatus")
		.def(py::init<>())
		.def(py::init<PipelineStatus::StatusType, const QString&>())
		.def_property_readonly("type", &PipelineStatus::type)
		.def_property_readonly("text", &PipelineStatus::text)
		.def(py::self == PipelineStatus())
		.def(py::self != PipelineStatus())
	;

	py::enum_<PipelineStatus::StatusType>(PipelineStatus_py, "Type")
		.value("Success", PipelineStatus::Success)
		.value("Warning", PipelineStatus::Warning)
		.value("Error", PipelineStatus::Error)
		.value("Pending", PipelineStatus::Pending)
	;

	auto DataObject_py = ovito_abstract_class<DataObject, RefTarget>(m,
			"Abstract base class for all data objects. A :py:class:`!DataObject` represents "
			"a piece of data processed and produced by a data pipeline. Typically, a data object "
			"is part of a :py:class:`~ovito.data.DataCollection`. The :py:mod:`ovito.data` module "
			"lists the different data object types implemented in OVITO. "
			"\n\n"
			"Certain data objects are associated with a :py:class:`~ovito.vis.Display` object, which is responsible for "
			"generating the visual representation of the data and rendering it in the viewports. "
			"The :py:attr:`.display` property provides access to the attached display object, which can be "
			"configured as needed to change the visual appearance of the data. "
			"The different types of display objects that exist in OVITO are documented in the :py:mod:`ovito.vis` module. ")
		.def_property("display", &DataObject::displayObject, &DataObject::setDisplayObject,
			"The :py:class:`~ovito.vis.Display` object associated with this data object, which is responsible for "
        	"rendering the data. If this field contains ``None``, the data is non-visual and doesn't appear in rendered images or the viewports.")
		// Used by DataCollection.copy_if_needed():
		.def_property_readonly("num_strong_references", &DataObject::numberOfStrongReferences)
	;
	expose_mutable_subobject_list(DataObject_py,
								  std::mem_fn(&DataObject::displayObjects), 
								  std::mem_fn(&DataObject::insertDisplayObject), 
								  std::mem_fn(&DataObject::removeDisplayObject), "display_objects", "DisplayObjectList");

	ovito_abstract_class<PipelineObject, RefTarget>{m}
		.def_property_readonly("status", &PipelineObject::status)
		.def("anim_time_to_source_frame", &PipelineObject::animationTimeToSourceFrame)
		.def("source_frame_to_anim_time", &PipelineObject::sourceFrameToAnimationTime)
	;

	ovito_abstract_class<CachingPipelineObject, PipelineObject>{m}
	;

	auto PipelineFlowState_py = py::class_<PipelineFlowState>(m,
			// Python class name:
			"PipelineFlowState",
			// Doc string:			
			":Base class: :py:class:`ovito.data.DataCollection`"
			"\n\n"
			"A specific form of :py:class:`~ovito.data.DataCollection` flowing down "
			"a data pipeline and being manipulated by modifiers on the way. A :py:class:`!PipelineFlowState` is "
			"returned by the :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` method. "
			"\n\n"
			"Note that OVITO's data pipeline system typically works with shallow copies of data collections. Thus, data objects "
			"may be shared by more than one data collection. This implies that modifying a data object is potentially unsafe as it can "
			"cause unexpected side effects. To avoid accidentally modifying data that is owned by the pipeline system and shared by multiple data collections, "
			"the following rule must be strictly followed: Before modifying a data object in a :py:class:`!PipelineFlowState` "
			"you have to use the :py:meth:`~DataCollection.copy_if_needed` method to ensure the object is really a deep copy "
			"exclusively owned by the :py:class:`!PipelineFlowState`: "
			"\n\n"
			".. literalinclude:: ../example_snippets/pipeline_flow_state.py\n"
			"   :lines: 5-14\n"
			"\n\n"
			"Note: The rule does not apply to :py:class:`~ovito.vis.Display` objects. These objects, which are typically attached "
			"to data objects, are not modified by a data pipeline. It typically is okay to modify them even though they are shallow copies "
			"shared by multiple data objects: "
			"\n\n"
			".. literalinclude:: ../example_snippets/pipeline_flow_state.py\n"
			"   :lines: 16-\n")
		.def(py::init<>())
		.def_property("status", &PipelineFlowState::status, &PipelineFlowState::setStatus)

		// The following methods are required for the DataCollection.attributes property.
		.def_property_readonly("attribute_names", [](PipelineFlowState& obj) -> QStringList {
				return obj.attributes().keys();
			})
		.def("get_attribute", [](PipelineFlowState& obj, const QString& attrName) -> py::object {
				auto item = obj.attributes().find(attrName);
				if(item == obj.attributes().end())
					return py::none();
				else
					return py::cast(item.value());
			})
		.def("set_attribute", [](PipelineFlowState& obj, const QString& attrName, py::object value) {
				if(value.is_none()) {
					obj.attributes().remove(attrName);
				}
				else {
					if(PyLong_Check(value.ptr()))
						obj.attributes().insert(attrName, QVariant::fromValue(PyLong_AsLong(value.ptr())));
					else if(PyFloat_Check(value.ptr()))
						obj.attributes().insert(attrName, QVariant::fromValue(PyFloat_AsDouble(value.ptr())));
					else
						obj.attributes().insert(attrName, QVariant::fromValue(value.cast<py::str>().cast<QString>()));
				}
			})
	;
	expose_mutable_subobject_list(PipelineFlowState_py,
								  std::mem_fn(&PipelineFlowState::objects), 
								  std::mem_fn(&PipelineFlowState::insertObject), 
								  std::mem_fn(&PipelineFlowState::removeObjectByIndex), "objects", "PipelineFlowStateObjectsList");

	ovito_abstract_class<Modifier, RefTarget>(m,
			"This is the base class for all modifier types in OVITO. See the :py:mod:`ovito.modifiers` module "
			"for a list of concrete modifier types that can be inserted into a data :py:class:`Pipeline`. ")
		.def_property("enabled", &Modifier::isEnabled, &Modifier::setEnabled,
				"Controls whether the modifier is applied to the data. Disabled modifiers "
				"are skipped during evaluation of a data pipeline. "
				"\n\n"
				":Default: ``True``\n")
		.def_property_readonly("modifier_applications", [](Modifier& mod) -> py::list {
				py::list l;
				for(ModifierApplication* modApp : mod.modifierApplications())
					l.append(py::cast(modApp));
				return l;
			})
		.def("create_modifier_application", &Modifier::createModifierApplication)
		.def("initialize_modifier", &Modifier::initializeModifier)
	;

	ovito_abstract_class<AsynchronousModifier, Modifier>{m}
	;
	
	ovito_class<ModifierApplication, CachingPipelineObject>{m}
		.def_property_readonly("modifier", &ModifierApplication::modifier)
		.def_property("input", &ModifierApplication::input, &ModifierApplication::setInput)
	;

	ovito_class<AsynchronousModifierApplication, ModifierApplication>{m}
	;

	ovito_abstract_class<DelegatingModifier, Modifier>{m}
	;

	ovito_abstract_class<MultiDelegatingModifier, Modifier>{m}
	;

	// This binding is required for the implementation of the modifier_operate_on_list() function:
	py::class_<std::vector<OORef<ModifierDelegate>>>(m, "ModifierDelegatesList")
		.def("__len__", [](const std::vector<OORef<ModifierDelegate>>& list) {
				size_t count = 0;
				for(ModifierDelegate* delegate : list)
					if(delegate->isEnabled()) count++;
				return count;
			})
		.def("__iter__", [](const std::vector<OORef<ModifierDelegate>>& list) {
				py::list pylist;
				for(ModifierDelegate* delegate : list)
					if(delegate->isEnabled()) pylist.append(delegate->getOOMetaClass().pythonDataName());
				return py::iter(pylist);
			})
		.def("__contains__", [](const std::vector<OORef<ModifierDelegate>>& list, const QString& type) {
				for(ModifierDelegate* delegate : list)
					if(type == delegate->getOOMetaClass().pythonDataName()) return delegate->isEnabled();
				return false;
			})
		.def("__repr__", [](const std::vector<OORef<ModifierDelegate>>& list) {
				py::set s;
				for(ModifierDelegate* delegate : list)
					if(delegate->isEnabled()) s.add(delegate->getOOMetaClass().pythonDataName());
				return py::repr(s);
			})
		.def("clear", [](std::vector<OORef<ModifierDelegate>>& list) {
				for(ModifierDelegate* delegate : list)
					delegate->setEnabled(false);
			})
		.def("remove", [](std::vector<OORef<ModifierDelegate>>& list, const QString& type) {
				for(ModifierDelegate* delegate : list) {
					if(type == delegate->getOOMetaClass().pythonDataName()) {
						delegate->setEnabled(false);
						return;
					}
				}
				throw py::value_error("Element is not present in set");
			})
		.def("discard", [](std::vector<OORef<ModifierDelegate>>& list, const QString& type) {
				for(ModifierDelegate* delegate : list)
					if(type == delegate->getOOMetaClass().pythonDataName())
						delegate->setEnabled(false);
			})
		.def("add", [](std::vector<OORef<ModifierDelegate>>& list, const QString& type) {
				for(ModifierDelegate* delegate : list) {
					if(type == delegate->getOOMetaClass().pythonDataName()) {
						delegate->setEnabled(true);
						return;
					}
				}
				throw py::value_error("This is not a valid data element name supported by this modifier");
			})
		.def("assign", [](std::vector<OORef<ModifierDelegate>>& list, const std::set<QString>& types) {
				for(ModifierDelegate* delegate : list) {
					if(types.find(delegate->getOOMetaClass().pythonDataName()) == types.end())
						delegate->setEnabled(false);
				}
				for(const QString& type : types) {
					bool found = false;
					for(ModifierDelegate* delegate : list) {
						if(type == delegate->getOOMetaClass().pythonDataName()) {
							delegate->setEnabled(true);
							found = true;
						}
					}
					if(!found)
						throw py::value_error("This is not a valid data element name supported by this modifier: '" + type.toStdString() + "'");
				}
			})
	;

	auto StaticSource_py = ovito_class<StaticSource, PipelineObject>(m,
		":Base class: :py:class:`ovito.data.DataCollection`\n\n"
		"Serves as a data :py:attr:`~Pipeline.source` for a :py:class:`Pipeline`. "
		"Being a type of :py:class:`~ovito.data.DataCollection`, a :py:class:`!StaticSource` can store a set of data objects, which will be passed to the :py:class:`Pipeline` as input data. "
		"One typically fills a :py:class:`!StaticSource` with some data objects and wires it to a :py:class:`Pipeline` as follows: "
		"\n\n"
		".. literalinclude:: ../example_snippets/static_source.py\n"
		"\n\n"
		"Another common use of the :py:class:`!StaticSource` class is in conjunction with the :py:func:`ovito.io.ase.ase_to_ovito` function. "
		)
		// The following methods are required for the DataCollection.attributes property.
		.def_property_readonly("attribute_names", [](StaticSource& obj) -> QStringList {
				return obj.attributes().keys();
			})
		.def("get_attribute", [](StaticSource& obj, const QString& attrName) -> py::object {
				auto item = obj.attributes().find(attrName);
				if(item == obj.attributes().end())
					return py::none();
				else
					return py::cast(item.value());
			})
		.def("set_attribute", [](StaticSource& obj, const QString& attrName, py::object value) {
				if(value.is_none()) {
					obj.removeAttribute(attrName);
				}
				else {
					if(PyLong_Check(value.ptr()))
						obj.setAttribute(attrName, QVariant::fromValue(PyLong_AsLong(value.ptr())));
					else if(PyFloat_Check(value.ptr()))
						obj.setAttribute(attrName, QVariant::fromValue(PyFloat_AsDouble(value.ptr())));
					else
						obj.setAttribute(attrName, QVariant::fromValue(value.cast<py::str>().cast<QString>()));
				}
			})
	;
	expose_mutable_subobject_list(StaticSource_py,
		std::mem_fn(&StaticSource::dataObjects), 
		std::mem_fn(&StaticSource::insertDataObject), 
		std::mem_fn(&StaticSource::removeDataObject), "objects", "StaticSourceDataObjectList");		

	auto SceneNode_py = ovito_abstract_class<SceneNode, RefTarget>(m)
		.def_property("name", &SceneNode::nodeName, &SceneNode::setNodeName)
		.def_property("display_color", &SceneNode::displayColor, &SceneNode::setDisplayColor)
		.def_property_readonly("parent_node", &SceneNode::parentNode)
		.def_property_readonly("lookat_node", &SceneNode::lookatTargetNode)
		.def_property("transform_ctrl", &SceneNode::transformationController, &SceneNode::setTransformationController)
		.def_property_readonly("is_selected", &SceneNode::isSelected)
		.def("delete", &SceneNode::deleteNode)
	;
	expose_mutable_subobject_list(SceneNode_py,
								  std::mem_fn(&SceneNode::children), 
								  std::mem_fn(&SceneNode::insertChildNode), 
								  std::mem_fn(&SceneNode::removeChildNode), "children", "SceneNodeChildren");		
	

	ovito_class<ObjectNode, SceneNode>(m,
			"This class represents a data pipeline, i.e. a data source plus a chain of zero or more modifiers "
			"that manipulate the data on the way through the pipeline. "
			"\n\n"
			"**Pipeline creation**\n"
			"\n\n"
			"Pipelines have a *data source*, which loads or dynamically generates the input data entering the "
			"pipeline. This source object is accessible through the :py:attr:`Pipeline.source` field and may be replaced if needed. "
			"For pipelines created by the :py:func:`~ovito.io.import_file` function, the data source is set to be a "
			":py:class:`FileSource` instance, which is responsible for loading the input data "
			"from the external file and feeding it into the pipeline. Another type of data source is the "
			":py:class:`StaticSource`, which allows to explicitly specify the set of data objects that enter the pipeline. "
			"\n\n"
			"The modifiers that are part of the pipeline are accessible through the :py:attr:`Pipeline.modifiers` list. "
			"This list is initially empty and you can populate it with modifiers of various kinds (see the :py:mod:`ovito.modifiers` module). "
			"Note that it is possible to employ the same :py:class:`Modifier` instance in more than one pipeline. It is "
			"also valid to assign the same source object as :py:attr:`.source` of several pipelines to make them share the same input data. "
			"\n\n"
			"**Pipeline evaluation**\n"
			"\n\n"
			"Once the pipeline is set up, an evaluation can be requested by calling :py:meth:`.compute()`, which means that the input data is loaded/generated by the :py:attr:`.source` "
			"and all modifiers of the pipeline are applied to the data one after the other. The :py:meth:`.compute()` method "
			"returns a :py:class:`~ovito.data.PipelineFlowState` structure containing all the data objects produced by the pipeline. "
			"Under the hood, an automatic caching system makes sure that unnecessary file accesses and computations are avoided. "
			"Repeatedly calling :py:meth:`compute` will not trigger a recalculation of the pipeline's results unless you "
			"modify the pipeline source, the sequence of modifiers or any of the modifier's parameters in some way. "
			"\n\n"
			"**Usage example**\n"
			"\n\n"
			"The following code example shows how to create a new pipeline by importing an MD simulation file and inserting a :py:class:`~ovito.modifiers.SliceModifier` to "
			"cut away some of the particles. Finally, the total number of remaining particles is printed. "
			"\n\n"
			".. literalinclude:: ../example_snippets/pipeline_example.py\n"
			"   :lines: 1-12\n"
			"\n\n"
			"It is possible to also access the input data that enters the modification pipeline. It is cached by the :py:class:`FileSource` "
			"constituting the :py:attr:`.source` of the pipeline: "
			"\n\n"
			".. literalinclude:: ../example_snippets/pipeline_example.py\n"
			"   :lines: 14-16\n"
			"\n\n"
			"**Data visualization and export**\n"
			"\n\n"
			"Finally, if you are interested in producing a graphical rendering of the data generated by a :py:class:`Pipeline`, "
			"you need to make the pipeline part of the current three-dimensional scene by calling the :py:meth:`.add_to_scene` method. "
			"Another possibility is to pass the pipeline to the :py:func:`ovito.io.export_file` function to export its output data "
			"to a file. ",
			// Python class name:
			"Pipeline")
		.def_property("data_provider", &ObjectNode::dataProvider, &ObjectNode::setDataProvider)
		.def_property("source", &ObjectNode::sourceObject, &ObjectNode::setSourceObject,
				"The object that provides the data entering the pipeline. "
				"This typically is a :py:class:`FileSource` instance if the node was created by a call to :py:func:`~ovito.io.import_file`. "
				"You can assign a new source to the pipeline if needed. See the :py:mod:`ovito.pipeline` module for a list of available pipeline source types. "
				"Note that you can even make several pipelines share the same source object. ")
		
		// Required by implementation of Pipeline.compute():
		.def("evaluate_pipeline", [](ObjectNode& node, TimePoint time) {

			// Full evaluation of the data pipeline not possible while interactive viewport rendering 
			// is in progress. In this case we return a preliminary pipeline state only.
			if(node.dataset()->viewportConfig()->isRendering()) {
				PipelineFlowState state = node.evaluatePipelinePreliminary(false);
				// Silently ignore errors during preliminary pipeline evaluation.
				if(state.status().type() == PipelineStatus::Error)
					state.setStatus(PipelineStatus(PipelineStatus::Warning, state.status().text()));
				return state;
			}
			else {
				// Start an asynchronous pipeline evaluation.
				SharedFuture<PipelineFlowState> future = node.evaluatePipeline(time);
				// Block until evaluation is complete and result is available.
				if(!ScriptEngine::activeTaskManager().waitForTask(future)) {
					PyErr_SetString(PyExc_KeyboardInterrupt, "Operation has been canceled by the user.");
					throw py::error_already_set();
				}
				return future.result();
			}
		})
	;

	ovito_class<SceneRoot, SceneNode>{m}
	;

	auto SelectionSet_py = ovito_class<SelectionSet, RefTarget>{m}
	;
	expose_mutable_subobject_list(SelectionSet_py,
								  std::mem_fn(&SelectionSet::nodes), 
								  std::mem_fn(&SelectionSet::insert), 
								  std::mem_fn(&SelectionSet::removeByIndex), "nodes", "SelectionSetNodes");

	ovito_class<PythonScriptModifier, Modifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"A modifier that allows to plug a custom Python script function into a data pipeline. "
			"\n\n"
			"This class makes it possible to implement new modifier types in the Python language which can participate in OVITO's "
			"data pipeline system and which may be used like OVITO's built-in modifiers. "
			"You can learn more about the usage of this class in the :ref:`writing_custom_modifiers` section. "
			"\n\n"
			"Example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/python_script_modifier.py")

		.def_property("script", &PythonScriptModifier::script, &PythonScriptModifier::setScript)

		.def_property("function", &PythonScriptModifier::scriptFunction, &PythonScriptModifier::setScriptFunction,
				"The Python function to be called each time the data pipeline is evaluated by the system."
				"\n\n"
				"The function must have a signature as shown in the example above. "
				"The *frame* parameter specifies the current animation frame number at which the data pipeline "
				"is being evaluated. The :py:class:`~ovito.data.DataCollection` *input* holds the "
				"input data objects of the modifier, which were produced by the upstream part of the data "
				"pipeline. *output* is the :py:class:`~ovito.data.DataCollection` where the function "
				"should store its output. "
				"\n\n"
				"By default, the *output* data collection already contains all data objects from the *input* data collection. "
				"Thus, without any further action, all data gets passed through the modifier unmodified. "
				"\n\n"
				":Default: ``None``\n")
	;
	ovito_class<PythonScriptModifierApplication, ModifierApplication>{m};
}

/// Helper function that generates a getter function for the 'operate_on' attribute of a DelegatingModifier subclass
py::cpp_function modifierDelegateGetter()
{
	return [](const DelegatingModifier& mod) { 
		return mod.delegate() ? mod.delegate()->getOOMetaClass().pythonDataName() : QString();
	};
}

/// Helper function that generates a setter function for the 'operate_on' attribute of a DelegatingModifier subclass.
py::cpp_function modifierDelegateSetter(const OvitoClass& delegateType)
{
	return [&delegateType](DelegatingModifier& mod, const QString& typeName) { 
		if(mod.delegate() && mod.delegate()->getOOMetaClass().pythonDataName() == typeName)
			return;
		for(auto clazz : PluginManager::instance().metaclassMembers<ModifierDelegate>(delegateType)) {
			if(clazz->pythonDataName() == typeName) {
				mod.setDelegate(static_object_cast<ModifierDelegate>(clazz->createInstance(mod.dataset())));
				return;
			}
		}
		// Error: User did not specify a valid type name.
		// Now build the list of valid names to generate a helpful error message.
		UndoSuspender noUndo(&mod);
		QStringList delegateTypeNames; 
		for(auto clazz : PluginManager::instance().metaclassMembers<ModifierDelegate>(delegateType)) {
			delegateTypeNames.push_back(QString("'%1'").arg(clazz->pythonDataName()));
		}
		mod.throwException(DelegatingModifier::tr("'%1' is not a valid type of data element this modifier can operate on. Supported types are: (%2)")
			.arg(typeName).arg(delegateTypeNames.join(QStringLiteral(", "))));
	};
}

};
