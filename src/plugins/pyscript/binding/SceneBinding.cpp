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
#include <core/dataset/scene/PipelineSceneNode.h>
#include <core/dataset/scene/RootSceneNode.h>
#include <core/dataset/scene/SelectionSet.h>
#include <core/dataset/data/DataObject.h>
#include <core/dataset/data/AttributeDataObject.h>
#include <core/dataset/pipeline/Modifier.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/pipeline/AsynchronousModifierApplication.h>
#include <core/dataset/pipeline/PipelineObject.h>
#include <core/dataset/pipeline/StaticSource.h>
#include <core/dataset/pipeline/DelegatingModifier.h>
#include <core/dataset/pipeline/AsynchronousDelegatingModifier.h>
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
			"a data fragment processed and produced by a data pipeline. See the :py:mod:`ovito.data` module "
			"for a list of the different types of data objects in OVITO. Typically, a data object is contained in a "
			":py:class:`~ovito.data.DataCollection` together with other data objects, forming a *data set*. "
			"Furthermore, data objects may be shared by several data collections. "
			"\n\n"
			"Certain data objects are associated with a :py:class:`~ovito.vis.DataVis` object, which is responsible for "
			"generating the visual representation of the data and rendering it in the viewports. "
			"The :py:attr:`.vis` field provides access to the attached visual element, which can be "
			"configured as needed to change the visual appearance of the data. "
			"The different visual element types of OVITO are all documented in the :py:mod:`ovito.vis` module. ")

		.def_property("id", &DataObject::identifier, &DataObject::setIdentifier,
				"The unique identifier string of the data object. May be empty. ")

		.def_property("vis", &DataObject::visElement, &DataObject::setVisElement,
			"The :py:class:`~ovito.vis.DataVis` element associated with this data object, which is responsible for "
        	"rendering the data visually. If this field contains ``None``, the data is non-visual and doesn't appear in "
			"rendered images or the viewports.")

		// Used by DataCollection.copy_if_needed():
		.def_property_readonly("num_strong_references", &DataObject::numberOfStrongReferences)

		// For backward compatibility with OVITO 2.9.0:
		.def_property("display", &DataObject::visElement, &DataObject::setVisElement)

	;
	expose_mutable_subobject_list(DataObject_py,
								  std::mem_fn(&DataObject::visElements), 
								  std::mem_fn(&DataObject::insertVisElement), 
								  std::mem_fn(&DataObject::removeVisElement), "vis_list", "DataVisList");

	ovito_class<AttributeDataObject, DataObject>{m};

	ovito_abstract_class<PipelineObject, RefTarget>{m}
		.def_property_readonly("status", &PipelineObject::status)
		.def("anim_time_to_source_frame", &PipelineObject::animationTimeToSourceFrame)
		.def("source_frame_to_anim_time", &PipelineObject::sourceFrameToAnimationTime)

		// Required by implementation of FileSource.compute():
		.def("_evaluate", [](PipelineObject& obj, TimePoint time) {

			// Full evaluation of the data pipeline is not possible while interactive viewport rendering 
			// is in progress. If rendering is in progress, we return a preliminary pipeline state only.
			if(obj.dataset()->viewportConfig()->isRendering()) {
				PipelineFlowState state = obj.evaluatePreliminary();
				// Silently ignore errors during preliminary pipeline evaluation.
				if(state.status().type() == PipelineStatus::Error)
					state.setStatus(PipelineStatus(PipelineStatus::Warning, state.status().text()));
				return state;
			}
			else {
				// Start an asynchronous pipeline evaluation.
				SharedFuture<PipelineFlowState> future = obj.evaluate(time);
				// Block until evaluation is complete and result is available.
				if(!ScriptEngine::activeTaskManager().waitForTask(future)) {
					PyErr_SetString(PyExc_KeyboardInterrupt, "Operation has been canceled by the user.");
					throw py::error_already_set();
				}
				return future.result();
			}
		})		
	;

	ovito_abstract_class<CachingPipelineObject, PipelineObject>{m}
	;

	auto DataCollection_py = py::class_<PipelineFlowState>(m,
			// Python class name:
			"DataCollection",
			// Doc string:
			"A :py:class:`!DataCollection` is a container that holds together multiple *data objects*, each representing "
			"a different facet of a dataset. Data collections are the main entities that are generated and processed in OVITO's "
			"data pipeline system. :py:class:`!DataCollection` instances are typically returned by the :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` and the "
			":py:meth:`FileSource.compute() <ovito.pipeline.FileSource.compute>` methods and contain the results of a data pipeline. "
			"\n\n"
			"Within a data collection, you will typically find a bunch of data objects,  "
			"which collectively form the dataset, for example: "
			"\n\n"
			" * :py:class:`~ovito.data.ParticleProperty` (array of per-particle values)\n"
			" * :py:class:`~ovito.data.SimulationCell` (cell vectors and boundary conditions)\n"
			" * :py:class:`~ovito.data.BondProperty` (array of per-bond values)\n"
			" * :py:class:`~ovito.data.SurfaceMesh` (triangle mesh representing a two-dimensional manifold)\n"
			" * :py:class:`~ovito.data.DislocationNetwork` (discrete dislocation lines)\n"
			" * ... and more\n"
			"\n"
			"All these types derive from the common :py:class:`~ovito.data.DataObject` base class. A :py:class:`!DataCollection` comprises two main parts: "
			"\n\n"
			" 1. The :py:attr:`.objects` list, which can hold an arbitrary number of data objects of the types listed above.\n"
			" 2. The :py:attr:`.attributes` dictionary, which stores auxialliary data in the form of simple key-value pairs.\n"
			"\n\n"
			"**Data object access**"
			"\n\n"
			"The :py:meth:`find` and :py:meth:`find_all` methods allow you to look up data objects in the :py:attr:`.objects` list of a data collection "
			"by type. For example, to retrieve the :py:class:`~ovito.data.SimulationCell` from a data collection: "
			"\n\n"
			".. literalinclude:: ../example_snippets/data_collection.py\n"
			"  :lines: 9-10"
			"\n\n"
			"The :py:meth:`find` method yields ``None`` if there is no instance of the given type in the collection. "
			"Alternatively, you can use the :py:meth:`.expect` method, which will instead raise an exception in case the requested object type is not present: "
			"\n\n"
			".. literalinclude:: ../example_snippets/data_collection.py\n"
			"  :lines: 15-15"
			"\n\n"
			"It is possible to programmatically add or remove data objects from the data collection by manipulating its :py:attr:`.objects` list. "
			"For instance, to populate a new data collection with a :py:class:`~ovito.data.SimulationCell` object we can write: "
			"\n\n"
			".. literalinclude:: ../example_snippets/data_collection.py\n"
			"  :lines: 20-22"
			"\n\n"
			"There are certain conventions regarding the numbers and types of data objects that may be present in a data collection. "
			"For example, there should never be more than one :py:class:`~ovito.data.SimulationCell` instance in a data collection. "
			"In contrast, there may be an arbitrary number of :py:class:`~ovito.data.ParticleProperty` instances in a data collection, "
			"but they all must have unique names and the same array length. Furthermore, there must always be one :py:class:`~ovito.data.ParticleProperty` named ``Position`` "
			"in a data collection, or no :py:class:`~ovito.data.ParticleProperty` at all. When manipulating the :py:attr:`.objects` list of a data "
			"collection directly, it is your responsibility to make sure that these conventions are followed. "
			"\n\n"
			"**Particle and bond access**"
			"\n\n"
			"To simplify the work with particles and bonds, which are represented by a bunch of :py:class:`~ovito.data.ParticleProperty` or "
			":py:class:`~ovito.data.BondProperty` instances, respectively, the :py:class:`!DataCollection` class provides two special " 
			"accessor fields. The :py:attr:`.particles` field represents a dictionary-like view of all the :py:class:`~ovito.data.ParticleProperty` data objects that are contained in a data collection. "
			"It thus works like a dynamic filter for the :py:attr:`.objects` list and permits name-based access to individual particle properties: "
			"\n\n"
			".. literalinclude:: ../example_snippets/data_collection.py\n"
			"  :lines: 26-27"
			"\n\n"
			"Similarly, the :py:attr:`.bonds` field is a dictionary-like view of all the :py:class:`~ovito.data.BondProperty` instances in "
			"a data collection. If you are adding or removing particle or bond properties in a data collection, you should "
			"always do so through these accessor fields instead of manipulating the :py:attr:`.objects` list directly. "
			"This will ensure that certain invariants are always maintained, e.g. the uniqueness of property names and the consistent size "
			"of all property arrays. "
			"\n\n"
			"**Attribute access**"
			"\n\n"
			"In addition to data objects, which represent complex forms of data, a data collection can store an arbitrary number of *attributes*, which are simple key-value pairs. The :py:attr:`.attributes` field of the data collection behaves like a Python dictionary and allows you to " 
			"read, manipulate or newly insert attributes, which are typically numeric values or string values. "
			"\n\n"
			"**Data ownership**"
			"\n\n"
			"One data object may be part of several :py:class:`!DataCollection` instances at a time, i.e. it may be shared by several data collections. " 
			"OVITO' pipeline system uses shallow data copies for performance reasons and to implement efficient data caching. " 
			"Modifiers typically manipulate only certain data objects in a collection. For example, the :py:class:`~ovito.modifiers.ColorCodingModifier` "
			"will selectively modify the values of the ``Color`` particle property but won't touch any of the other data objects "
			"present in the input data collection. The unmodified data objects will simply be passed through to the output data collection "
			"without creating a new copy of the data values. As a consequence of this design, both the input data collection and the "
			"output collection of the pipeline may refer to the same data objects. In such a situation, no data collection owns the "
			"data objects exclusively anymore. "
			"\n\n"
			"Thus, in general it is not safe to manipulate the contents of a data object in a data collection, because that could lead to "
			"unwanted side effects or corruption of data maintained by the pipeline system. For example, modifying the particle positions in a data collection "
			"that was returned by a system function is forbidden (or rather discouraged): "
			"\n\n"
			".. literalinclude:: ../example_snippets/data_collection.py\n"
			"  :lines: 30-33"
			"\n\n"
			"Before manipulating the contents of a data object in any way, it is crucial to ensure that no second data collection is referring to the same object. "
			"The :py:meth:`.copy_if_needed` method helps you ensure that a data object is exclusive owned by a certain data collection: "
			"\n\n"
			".. literalinclude:: ../example_snippets/data_collection.py\n"
			"  :lines: 37-44"
			"\n\n"
			":py:meth:`.copy_if_needed` first checks whether the given object is currently shared by more than one data collection. If yes, "
			"a deep copy of the object is made and the original object in the data collection is replaced with the copy. "
			"Now we can be confident that the copied data object is exclusively owned by the data collection and it's safe to modify it without risking side effects. ")
		.def(py::init<>())
		.def_property("status", &PipelineFlowState::status, py::overload_cast<const PipelineStatus&>(&PipelineFlowState::setStatus))

#if 0
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
						obj.attributes().insert(attrName, QVariant::fromValue(castToQString(value.cast<py::str>())));
				}
			})
#endif
	;
	expose_mutable_subobject_list(DataCollection_py,
								  std::mem_fn(&PipelineFlowState::objects), 
								  std::mem_fn(&PipelineFlowState::insertObject), 
								  std::mem_fn(&PipelineFlowState::removeObjectByIndex), "objects", "DataCollectionObjectsList",
			"The list of data objects that make up the data collection. Data objects are instances of :py:class:`DataObject`-derived "
			"classes, for example :py:class:`ParticleProperty`, :py:class:`Bonds` or :py:class:`SimulationCell`. "
			"\n\n"
			"You can add or remove objects from the :py:attr:`!objects` list to insert them or remove them from the :py:class:`!DataCollection`.  "
			"However, it is your responsibility to ensure that the data objects are all in a consistent state. For example, "
			"all :py:class:`ParticleProperty` objects in a data collection must have the same lengths at all times, because "
			"the length implicitly specifies the number of particles. The order in which data objects are stored in the data collection "
			"does not matter. "
			"\n\n"
			"Note that the :py:class:`!DataCollection` class also provides convenience views of the data objects contained in the :py:attr:`!objects` "
			"list: For example, the :py:attr:`.particles` dictionary lists all :py:class:`ParticleProperty` instances in the " 
			"data collection by name and the :py:attr:`.bonds` does the same for all :py:class:`BondProperty` instances. "
			"Since these dictionaries are views, they always reflect the current contents of the master :py:attr:`!objects` list. ");

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
		// Internal method required by implementation of Pipeline.modifiers:
		.def("create_modifier_application", &Modifier::createModifierApplication)
		.def("initialize_modifier", &Modifier::initializeModifier)
		.def_property_readonly("some_modifier_application", &Modifier::someModifierApplication)
	;

	ovito_abstract_class<AsynchronousModifier, Modifier>{m}
	;
	
	ovito_class<ModifierApplication, CachingPipelineObject>{m}
		.def_property("modifier", &ModifierApplication::modifier, &ModifierApplication::setModifier)
		.def_property("input", &ModifierApplication::input, &ModifierApplication::setInput)
	;

	ovito_class<AsynchronousModifierApplication, ModifierApplication>{m}
	;

	ovito_abstract_class<ModifierDelegate, RefTarget>{m}
		.def_property("enabled", &ModifierDelegate::isEnabled, &ModifierDelegate::setEnabled)
	;

	ovito_abstract_class<AsynchronousModifierDelegate, RefTarget>{m}
	;

	ovito_abstract_class<DelegatingModifier, Modifier>{m}
		.def_property("delegate", &DelegatingModifier::delegate, &DelegatingModifier::setDelegate)
	;

	ovito_abstract_class<MultiDelegatingModifier, Modifier>{m}
	;

	ovito_abstract_class<AsynchronousDelegatingModifier, AsynchronousModifier>{m}
		.def_property("delegate", &AsynchronousDelegatingModifier::delegate, &AsynchronousDelegatingModifier::setDelegate)
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
		"Serves as a data :py:attr:`~Pipeline.source` for a :py:class:`Pipeline`. "
		"A :py:class:`!StaticSource` stores a :py:class:`~ovito.data.DataCollection`, which will be passed to the :py:class:`Pipeline` as input data. "
		"One typically fills a :py:class:`!StaticSource` with some data objects and wires it to a :py:class:`Pipeline` as follows: "
		"\n\n"
		".. literalinclude:: ../example_snippets/static_source.py\n"
		)

		.def("assign", [](StaticSource& source, const PipelineFlowState& state) {
			source.setDataObjects({});
			for(DataObject* obj : state.objects())
				source.addDataObject(obj);
		},
		"assign(data)"
		"\n\n"
		"Sets the contents of this :py:class:`!StaticSource`. "
		"\n\n"
		":param data: The :py:class:`~ovito.data.DataCollection` to be copied into this static source object.\n",
		py::arg("data"))

		.def("compute", [](StaticSource& source, py::object /*frame*/) {
			return source.evaluatePreliminary();
		},
		"compute(frame=None)"
		"\n\n"
		"Retrieves the data of this data source, which was previously stored by a call to :py:meth:`.assign`. "
		"\n\n"
		":param frame: This parameter is ignored, because the data of a :py:class:`!StaticSource` is not time-dependent.\n"
		":return: A new :py:class:`~ovito.data.DataCollection` containing the data stored in this :py:class:`!StaticSource`.\n",
		py::arg("frame") = py::none())
	;

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
	

	auto Pipeline_py = ovito_class<PipelineSceneNode, SceneNode>(m,
			"This class encapsulates a data pipeline, consisting of a *data source* and a chain of zero or more *modifiers*, "
			"which manipulate the data on the way through the pipeline. "
			"\n\n"
			"**Pipeline creation**\n"
			"\n\n"
			"A pipeline always has a *data source*, which loads or dynamically generates the input data entering the "
			"pipeline. This source object is accessible through the :py:attr:`Pipeline.source` field and may be replaced if needed. "
			"For pipelines created by the :py:func:`~ovito.io.import_file` function, the data source is automatically set to be a "
			":py:class:`FileSource` instance, which is responsible for loading the input data "
			"from the external file and feeding it into the pipeline. Another type of data source is the "
			":py:class:`StaticSource`, which allows to explicitly specify the set of data objects entering the pipeline. "
			"\n\n"
			"The modifiers that are part of the pipeline are accessible through the :py:attr:`Pipeline.modifiers` list. "
			"This list is initially empty and you can populate it with modifiers of various kinds (see the :py:mod:`ovito.modifiers` module). "
			"Note that it is possible to employ the same :py:class:`Modifier` instance in more than one pipeline. And it is "
			"valid to share the same data source between several pipelines to let them process the same input data. "
			"\n\n"
			"**Pipeline evaluation**\n"
			"\n\n"
			"Once the pipeline is set up, an evaluation can be requested by calling :py:meth:`.compute()`, which means that the input data will be loaded/generated by the :py:attr:`.source` "
			"and all modifiers of the pipeline are applied to the data one after the other. The :py:meth:`.compute()` method "
			"returns a new :py:class:`~ovito.data.DataCollection` containing all the data objects produced by the pipeline. "
			"Under the hood, an automatic caching system ensure that unnecessary file accesses and computations are avoided. "
			"Repeatedly calling :py:meth:`compute` will not trigger a recalculation of the pipeline's results unless you "
			"alter the pipeline's source, the sequence of modifiers or any of the modifier's parameters. "
			"\n\n"
			"**Usage example**\n"
			"\n\n"
			"The following code example shows how to create a new pipeline by importing an MD simulation file and inserting a :py:class:`~ovito.modifiers.SliceModifier` to "
			"cut away some of the particles. Finally, the total number of remaining particles is printed. "
			"\n\n"
			".. literalinclude:: ../example_snippets/pipeline_example.py\n"
			"   :lines: 1-12\n"
			"\n\n"
			"Note that you can access the input data of the pipeline by calling the :py:meth:`FileSource.compute` method: "
			"\n\n"
			".. literalinclude:: ../example_snippets/pipeline_example.py\n"
			"   :lines: 14-16\n"
			"\n\n"
			"**Data visualization**\n"
			"\n\n"
			"If you intend to produce a graphical rendering of a pipeline's output data, "
			"you need to make the pipeline part of the current three-dimensional scene by calling its :py:meth:`.add_to_scene` method. "
			"The visual appearance of the output data is controlled by so-called visual elements, which are generated within the pipeline. "
			"The :py:meth:`.get_vis` method helps you look up a visual element of a particular type. "
			"\n\n"
			"**Data export**\n"
			"\n\n"
			"To export the generated data of the pipeline to an output file, simply call the :py:func:`ovito.io.export_file` function with the pipeline. ",
			// Python class name:
			"Pipeline")
		.def_property("data_provider", &PipelineSceneNode::dataProvider, &PipelineSceneNode::setDataProvider)
		.def_property("source", &PipelineSceneNode::pipelineSource, &PipelineSceneNode::setPipelineSource,
				"The object that provides the data entering the pipeline. "
				"This typically is a :py:class:`FileSource` instance if the pipeline was created by a call to :py:func:`~ovito.io.import_file`. "
				"You can assign a new source to the pipeline if needed. See the :py:mod:`ovito.pipeline` module for a list of available pipeline source types. "
				"Note that you can even make several pipelines share the same source object. ")
		
		// Required by implementation of Pipeline.compute():
		.def("evaluate_pipeline", [](PipelineSceneNode& node, TimePoint time) {

			// Full evaluation of the data pipeline is not possible while interactive viewport rendering 
			// is in progress. If rendering is in progress, we return a preliminary pipeline state only.
			if(node.dataset()->viewportConfig()->isRendering()) {
				PipelineFlowState state = node.evaluatePipelinePreliminary(false);
				// Silently ignore errors during preliminary pipeline evaluation.
				if(state.status().type() == PipelineStatus::Error)
					state.setStatus(PipelineStatus(PipelineStatus::Warning, state.status().text()));
				return std::move(state);
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
	expose_subobject_list(Pipeline_py, std::mem_fn(&PipelineSceneNode::visElements), "vis_elements", "PipelineVisElementsList");

	ovito_class<RootSceneNode, SceneNode>{m}
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

}
