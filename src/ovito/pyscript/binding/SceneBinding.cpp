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

#include <ovito/pyscript/PyScript.h>
#include <ovito/core/dataset/scene/SceneNode.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/dataset/data/DataObject.h>
#include <ovito/core/dataset/data/TransformedDataObject.h>
#include <ovito/core/dataset/data/AttributeDataObject.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifierApplication.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/StaticSource.h>
#include <ovito/core/dataset/pipeline/DelegatingModifier.h>
#include <ovito/core/dataset/pipeline/AsynchronousDelegatingModifier.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include <ovito/pyscript/extensions/PythonScriptModifier.h>
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

	ovito_enum<PipelineStatus::StatusType>(PipelineStatus_py, "Type")
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

		.def_property("vis", static_cast<DataVis* (DataObject::*)() const>(&DataObject::visElement), [](DataObject& obj, DataVis* vis) {
				ensureDataObjectIsMutable(obj);
				obj.setVisElement(vis);
			},
			"The :py:class:`~ovito.vis.DataVis` element associated with this data object, which is responsible for "
        	"rendering the data visually. If this field contains ``None``, the data is non-visual and doesn't appear in "
			"rendered images or the viewports.")

		// Used internally by the Python layer:
		.def_property_readonly("num_strong_references", &DataObject::numberOfStrongReferences)
		.def_property_readonly("is_safe_to_modify", &DataObject::isSafeToModify)

		.def("make_mutable", [](DataObject& parent, const DataObject* subobj) -> DataObject* {
				if(!subobj) return nullptr;
				if(!parent.hasReferenceTo(subobj)) throw Exception("Object to be made mutable is not a sub-object of this parent.");
				return parent.makeMutable(subobj);
			},
			"make_mutable(subobj)"
			"\n\n"
			"Requests a deep copy of a sub-object of this :py:class:`DataObject` in case it is shared with another :py:class:`DataObject`. "
			"\n\n"
#if 0
			"Makes a copy of a data object from this data collection if the object is not exclusively "
    		"owned by the data collection but shared with other collections. After the method returns, "
    		"the data object is exclusively owned by the collection and it becomes safe to modify the object without "
    		"causing unwanted side effects. "
			"\n\n"
			"Typically, this method is used within user-defined modifier functions (see :py:class:`~ovito.modifiers.PythonScriptModifier`) that "
    		"participate in OVITO's data pipeline system. A modifier function receives an input collection of "
    		"data objects from the system. However, modifying these input "
    		"objects in place is not allowed, because they are owned by the pipeline and modifying them would "
    		"lead do unexpected side effects. "
    		"This is where this method comes into play: It makes a copy of a given data object and replaces "
    		"the original in the data collection with the copy. The caller can now safely modify this copy in place, "
    		"because no other data collection can possibly be referring to it. "
			"\n\n"
   			"The :py:meth:`!make_mutable` method first checks if *obj*, which must be a data object from this data collection, is "
    		"shared with some other data collection. If yes, it creates an exact copy of *obj* and replaces the original "
    		"in this data collection with the copy. Otherwise it leaves the object as is, because it is already exclusively owned "
    		"by this data collection. "
			"\n\n"
#endif
			":param DataObject subobj: The object from this data collection to be copied if needed.\n"
    		":return: A copy of *subobj* if it was shared with somebody else. Otherwise the original object is returned.\n",
			py::arg("subobj"))

		// For backward compatibility with OVITO 2.9.0:
		.def_property("display", static_cast<DataVis* (DataObject::*)() const>(&DataObject::visElement), &DataObject::setVisElement)

	;
	createDataPropertyAccessors(DataObject_py, "identifier", &DataObject::identifier, &DataObject::setIdentifier,
				"The unique identifier string of the data object. May be empty. ");
	expose_mutable_subobject_list(DataObject_py,
								  std::mem_fn(&DataObject::visElements),
								  std::mem_fn(&DataObject::insertVisElement),
								  std::mem_fn(&DataObject::removeVisElement), "vis_list", "DataVisList");

	ovito_class<AttributeDataObject, DataObject>{m}
		.def_property("value", &AttributeDataObject::value, [](AttributeDataObject& obj, py::object value) {
			if(!obj.isSafeToModify())
				throw Exception(QStringLiteral("You tried to set the value of a global attribute that is not exclusively owned."));
			if(PyLong_Check(value.ptr()))
				obj.setValue(QVariant::fromValue(PyLong_AsLong(value.ptr())));
			else if(PyFloat_Check(value.ptr()))
				obj.setValue(QVariant::fromValue(PyFloat_AsDouble(value.ptr())));
			else
				obj.setValue(QVariant::fromValue(castToQString(value.cast<py::str>())));
		})
	;

	ovito_abstract_class<PipelineObject, RefTarget>{m}
		.def_property_readonly("status", &PipelineObject::status)
		.def("anim_time_to_source_frame", &PipelineObject::animationTimeToSourceFrame)
		.def("source_frame_to_anim_time", &PipelineObject::sourceFrameToAnimationTime)

		// Required by implementations of FileSource.compute() and DataCollection.apply() methods:
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
				if(!ScriptEngine::waitForFuture(future)) {
					PyErr_SetString(PyExc_KeyboardInterrupt, "Operation has been canceled by the user.");
					throw py::error_already_set();
				}
				return future.result();
			}
		})
	;

	ovito_abstract_class<TransformedDataObject, DataObject>{m};

	ovito_abstract_class<CachingPipelineObject, PipelineObject>{m}
	;

	auto PipelineFlowState_py = py::class_<PipelineFlowState>(m, "PipelineFlowState")
		.def_property_readonly("status", &PipelineFlowState::status)
		.def_property_readonly("data", &PipelineFlowState::data)
		.def_property_readonly("mutable_data", &PipelineFlowState::mutableData)
	;

	auto DataCollection_py = ovito_class<DataCollection, DataObject>(m,
			":Base class: :py:class:`ovito.data.DataObject`"
			"\n\n"
			"A :py:class:`!DataCollection` is a container class holding together individual *data objects*, each representing "
			"different fragments of a dataset. For example, a dataset loaded from a simulation data file may consist "
			"of particles, the simulation cell information and additional auxiliary data such as the current timestep "
			"number of the snaphots, etc. All this information is contained in one :py:class:`!DataCollection`, which "
			"exposes the individual pieces of information as sub-objects, for example via the :py:attr:`DataCollection.particles`, "
			":py:attr:`DataCollection.cell` and :py:attr:`DataCollection.attributes` fields. "
			"\n\n"
			"Data collections are the elementary entities that get processed within a data :py:class:`~ovito.pipeline.Pipeline`. "
			"Each modifier receives a data collection from the preceding modifier, alters it in some way, and passes it "
			"on to the next modifier. The output data collection of the last modifier in the pipeline is returned by the :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` method. "
			"\n\n"
			"A data collection essentially consists of a bunch of :py:class:`DataObjects <ovito.data.DataObject>`, "
			"which are all stored in the :py:attr:`DataCollection.objects` list. Typically, you don't access the data objects "
			"through this list directly but rather use on of the special accessor fields provided by the :py:class:`!DataCollection` class, "
			"which give more convenient access to data objects of a particular kind. For example, the :py:attr:`.surfaces` "
			"dictionary provides key-based access to all the :py:class:`~ovito.data.SurfaceMesh` instances currently in the data collection. "
			"\n\n"
			"You can programmatically add or remove data objects from a data collection by manipulating its :py:attr:`.objects` list. "
			"For instance, to populate a new data collection instance that is initially empty with a new :py:class:`~ovito.data.SimulationCell` object: "
			"\n\n"
			".. literalinclude:: ../example_snippets/data_collection.py\n"
			"  :lines: 9-12"
#if 0
			"\n\n"
			"While in principle a data collection can host an arbitrary number of data objects of any kind, there exist certain conventions that you should follow: "
			"A data collection should contain at most one :py:class:`~ovito.data.SimulationCell` object and at most one :py:class:`~ovito.data.Particles` object. "
			"In constrast to that, there may be an arbitrary number of :py:class:`~ovito.data.DataSeries` or :py:class:`~ovito.data.VoxelGrid` objects "
			"in the same data collection, for example. They will be discriminated by their unique :py:attr:`~ovito.data.DataObject.identifier` keys. "
			"Certain types of data objects should not be directly stored in a data collection. For example, the :py:class:`~ovito.data.Bonds` object "
			"cannot exist on its own and is always a child object of the :py:class:`~ovito.data.Particles` object. "
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
			"Now we can be confident that the copied data object is exclusively owned by the data collection and it's safe to modify it without risking side effects. "
#endif
			)

		// Needed for the implementation of DataCollection.apply(): Copies the data objects over from another DataCollection.
		.def("_assign_objects", [](DataCollection& self, const DataCollection& other) {
			self.setObjects(other.objects());
		});
	;
	expose_mutable_subobject_list(DataCollection_py,
								  std::mem_fn(&DataCollection::objects),
								  std::mem_fn(&DataCollection::insertObject),
								  std::mem_fn(&DataCollection::removeObjectByIndex), "objects", "DataCollectionObjectsList",
			"The unordered list of all :py:class:`DataObjects <DataObject>` stored in this data collection. You can add or remove data objects in this list as needed. "
			"\n\n"
			"Note that typically you don't have to work with this list directly, because the :py:class:`!DataCollection` class provides several convenience accessor fields for the different flavors of objects in this mixed list. "
			"For example, the :py:attr:`DataCollection.particles` field returns the :py:class:`Particles` object from this data objects list. "
			"Also, dictionary views such as :py:attr:`DataCollection.series` provide key-based access to a particular class of data objects from this list. ");

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
			"A :py:class:`!StaticSource` manages a :py:class:`~ovito.data.DataCollection`, which it will pass to the :py:class:`Pipeline` as input data. "
			"One typically initializes a :py:class:`!StaticSource` with a collection of data objects, then wiring it to a :py:class:`Pipeline` as follows: "
			"\n\n"
			".. literalinclude:: ../example_snippets/static_source.py\n"
		)
		.def_property("data", &StaticSource::dataCollection, &StaticSource::setDataCollection,
			"The :py:class:`~ovito.data.DataCollection` managed by this object, which will be fed to the pipeline. "
			"\n\n"
			":Default: ``None``\n")

		.def("compute", [](StaticSource& source, py::object /*frame*/) {
				return source.evaluatePreliminary().data();
			},
			"compute(frame=None)"
			"\n\n"
			"Returns a copy of the :py:class:`~ovito.data.DataCollection` stored in this source's :py:attr:`.data` field. "
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
			"Every pipeline has a *data source*, which loads or dynamically generates the input data entering the "
			"pipeline. This source is accessible through the :py:attr:`Pipeline.source` field and may be replaced with a different kind of source object if needed. "
			"For pipelines created by the :py:func:`~ovito.io.import_file` function, the data source is automatically set to be a "
			":py:class:`FileSource` object, which loads the input data "
			"from the external file and feeds it into the pipeline. Another kind of data source is the "
			":py:class:`StaticSource`, which can be used if you want to programmatically specify the input data for the pipeline "
			"instead of loading it from a file. "
			"\n\n"
			"The modifiers that are part of the pipeline are accessible through the :py:attr:`Pipeline.modifiers` field. "
			"This list is initially empty and you can populate it with the modifier types found in the :py:mod:`ovito.modifiers` module. "
			"Note that it is possible to employ the same :py:class:`Modifier` instance in more than one pipeline. And it is "
			"okay to use the same data source object for several pipelines, letting them process the same input data. "
			"\n\n"
			"**Pipeline evaluation**\n"
			"\n\n"
			"Once the pipeline is set up, its computation results can be requested by calling :py:meth:`.compute()`, which means that the input data will be loaded/generated by the :py:attr:`.source` "
			"and all modifiers of the pipeline are applied to the data one after the other. The :py:meth:`.compute()` method "
			"returns a new :py:class:`~ovito.data.DataCollection` storing the data objects produced by the pipeline. "
			"Under the hood, an automatic caching system ensures that unnecessary file accesses and computations are avoided. "
			"Repeatedly calling :py:meth:`compute` will not trigger a recalculation of the pipeline's results unless you "
			"alter the pipeline's data source, the chain of modifiers, or a modifier's parameters. "
			"\n\n"
			"**Usage example**\n"
			"\n\n"
			"The following code example shows how to create a new pipeline by importing an MD simulation file and inserting a :py:class:`~ovito.modifiers.SliceModifier` to "
			"cut away some of the particles. Finally, the total number of remaining particles is printed. "
			"\n\n"
			".. literalinclude:: ../example_snippets/pipeline_example.py\n"
			"   :lines: 1-12\n"
			"\n\n"
			"If you would like to access the unmodified input data of the pipeline, i.e. *before* it has been processed by any of the modifiers, "
			"you can call the :py:meth:`FileSource.compute` method instead: "
			"\n\n"
			".. literalinclude:: ../example_snippets/pipeline_example.py\n"
			"   :lines: 14-16\n"
			"\n\n"
			"**Data visualization**\n"
			"\n\n"
			"If you intend to produce graphical renderings of a output data produced by a pipeline, "
			"you must make the pipeline part of the current three-dimensional scene by calling the :py:meth:`Pipeline.add_to_scene` method. "
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
				SharedFuture<PipelineFlowState> future = node.evaluatePipeline(time, true);
				// Block until evaluation is complete and result is available.
				if(!ScriptEngine::waitForFuture(future)) {
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
				"is being evaluated. The :py:class:`~ovito.data.DataCollection` *data* initially holds the "
				"input data objects of the modifier, which were produced by the upstream part of the data "
				"pipeline. The user-defined modifier function is free modify the data collection and the data objects "
				"stored in it. "
				"\n\n"
				":Default: ``None``\n")
	;
	ovito_class<PythonScriptModifierApplication, ModifierApplication>{m};
}

}
