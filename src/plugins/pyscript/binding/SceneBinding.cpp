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
#include <core/dataset/data/simcell/SimulationCellObject.h>
#include <core/dataset/data/simcell/PeriodicDomainDataObject.h>
#include <core/dataset/data/properties/PropertyObject.h>
#include <core/dataset/pipeline/Modifier.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/pipeline/AsynchronousModifierApplication.h>
#include <core/dataset/pipeline/PipelineObject.h>
#include <core/dataset/pipeline/StaticSource.h>
#include <core/dataset/pipeline/modifiers/DelegatingModifier.h>
#include <core/dataset/pipeline/modifiers/GenericPropertyModifier.h>
#include <core/dataset/pipeline/modifiers/SliceModifier.h>
#include <core/dataset/pipeline/modifiers/AffineTransformationModifier.h>
#include <core/dataset/pipeline/modifiers/ClearSelectionModifier.h>
#include <core/dataset/pipeline/modifiers/InvertSelectionModifier.h>
#include <core/dataset/pipeline/modifiers/ColorCodingModifier.h>
#include <core/dataset/pipeline/modifiers/AssignColorModifier.h>
#include <core/dataset/pipeline/modifiers/DeleteSelectedModifier.h>
#include <core/dataset/pipeline/modifiers/SelectTypeModifier.h>
#include <core/dataset/pipeline/modifiers/HistogramModifier.h>
#include <core/dataset/pipeline/modifiers/ScatterPlotModifier.h>
#include <core/dataset/pipeline/modifiers/ReplicateModifier.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/app/PluginManager.h>
#include <core/utilities/concurrent/TaskManager.h>
#include <plugins/pyscript/extensions/PythonScriptModifier.h>
#include "PythonBinding.h"

namespace PyScript {

using namespace Ovito;

template<bool ReadOnly>
py::dict PropertyObject__array_interface__(PropertyObject& p)
{
	py::dict ai;
	if(p.componentCount() == 1) {
		ai["shape"] = py::make_tuple(p.size());
		if(p.stride() != p.dataTypeSize())
			ai["strides"] = py::make_tuple(p.stride());
	}
	else if(p.componentCount() > 1) {
		ai["shape"] = py::make_tuple(p.size(), p.componentCount());
		ai["strides"] = py::make_tuple(p.stride(), p.dataTypeSize());
	}
	else throw Exception("Cannot access empty property from Python.");
	if(p.dataType() == qMetaTypeId<int>()) {
		OVITO_STATIC_ASSERT(sizeof(int) == 4);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
		ai["typestr"] = py::bytes("<i4");
#else
		ai["typestr"] = py::bytes(">i4");
#endif
	}
	else if(p.dataType() == qMetaTypeId<FloatType>()) {
#ifdef FLOATTYPE_FLOAT		
		OVITO_STATIC_ASSERT(sizeof(FloatType) == 4);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
		ai["typestr"] = py::bytes("<f4");
#else
		ai["typestr"] = py::bytes(">f4");
#endif
#else
		OVITO_STATIC_ASSERT(sizeof(FloatType) == 8);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
		ai["typestr"] = py::bytes("<f8");
#else
		ai["typestr"] = py::bytes(">f8");
#endif
#endif
	}
	else throw Exception("Cannot access property of this data type from Python.");
	if(ReadOnly) {
		ai["data"] = py::make_tuple(reinterpret_cast<std::intptr_t>(p.constData()), true);
	}
	else {
		ai["data"] = py::make_tuple(reinterpret_cast<std::intptr_t>(p.data()), false);
	}
	ai["version"] = py::cast(3);
	return ai;
}

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

	ovito_abstract_class<GenericPropertyModifier, Modifier>{m}
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

	auto SliceModifier_py = ovito_class<SliceModifier, MultiDelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"Deletes or selects data elements in a semi-infinite region bounded by a plane or in a slab bounded by a pair of parallel planes. "
			"The modifier can operate on several kinds of data elements: "
			"\n\n"
			"  * Particles (including bonds)\n"
			"  * Surfaces (:py:class:`~ovito.data.SurfaceMesh`) \n"
			"  * Dislocation lines (:py:class:`~ovito.data.DislocationNetwork`)\n"
			"\n\n"
			"The modifier will act on all of them by default. Restriction to a subset is possible by setting the :py:attr:`.operate_on` field. ")
		.def_property("distance", &SliceModifier::distance, &SliceModifier::setDistance,
				"The distance of the slicing plane from the origin (along its normal vector)."
				"\n\n"
				":Default: 0.0\n")
		.def_property("normal", &SliceModifier::normal, &SliceModifier::setNormal,
				"The normal vector of the slicing plane. Does not have to be a unit vector."
				"\n\n"
				":Default: ``(1,0,0)``\n")
		.def_property("slice_width", &SliceModifier::sliceWidth, &SliceModifier::setSliceWidth,
				"The width of the slab to cut. If zero, the modifier cuts away everything on one "
				"side of the slicing plane."
				"\n\n"
				":Default: 0.0\n")
		.def_property("inverse", &SliceModifier::inverse, &SliceModifier::setInverse,
				"Reverses the sense of the slicing plane."
				"\n\n"
				":Default: ``False``\n")
		.def_property("select", &SliceModifier::createSelection, &SliceModifier::setCreateSelection,
				"If ``True``, the modifier selects data elements instead of deleting them."
				"\n\n"
				":Default: ``False``\n")
		.def_property("only_selected", &SliceModifier::applyToSelection, &SliceModifier::setApplyToSelection,
				"Controls whether the modifier should act only on currently selected data elements (e.g. selected particles)."
				"\n\n"
				":Default: ``False``\n")
	;
	modifier_operate_on_list(SliceModifier_py, std::mem_fn(&SliceModifier::delegates), "operate_on", 
			"A set of strings specifying the kinds of data elements this modifier should operate on. "
			"By default the set contains all data element types supported by the modifier. "
			"\n\n"
			":Default: ``{'particles', 'surfaces', 'dislocations'}``\n");


	auto AffineTransformationModifier_py = ovito_class<AffineTransformationModifier, MultiDelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"Applies an affine transformation to data elements in order to move, rotate, shear or scale them."
			"The modifier can operate on several kinds of data elements: "
			"\n\n"
			"  * Particle positions\n"
			"  * Particle vector properties (``'Velocity'``, ``'Force'``, ``'Displacement'``)\n"
			"  * Simulation cells (:py:class:`~ovito.data.SimulationCell`) \n"
			"  * Surfaces (:py:class:`~ovito.data.SurfaceMesh`) \n"
			"\n\n"
			"The modifier will act on all of them by default. Restriction to a subset is possible by setting the :py:attr:`.operate_on` field. "
			"Example::"
			"\n\n"
			"    xy_shear = 0.05\n"
			"    mod = AffineTransformationModifier(\n"
			"              operate_on = {'particles'},  # Transform particles but not simulation box\n"
			"              transformation = [[1,xy_shear,0,0],\n"
			"                                [0,       1,0,0],\n"
			"                                [0,       0,1,0]])\n"
			"\n")
		.def_property("transformation", MatrixGetter<AffineTransformationModifier, AffineTransformation, &AffineTransformationModifier::transformationTM>(), 
										MatrixSetter<AffineTransformationModifier, AffineTransformation, &AffineTransformationModifier::setTransformationTM>(),
				"The 3x4 transformation matrix being applied to input elements. "
				"The first three matrix columns define the linear part of the transformation, while the fourth "
				"column specifies the translation vector. "
				"\n\n"
				"This matrix describes a relative transformation and is used only if :py:attr:`.relative_mode` == ``True``."
				"\n\n"
				":Default: ``[[ 1.  0.  0.  0.] [ 0.  1.  0.  0.] [ 0.  0.  1.  0.]]``\n")
		.def_property("target_cell", MatrixGetter<AffineTransformationModifier, AffineTransformation, &AffineTransformationModifier::targetCell>(), 
									 MatrixSetter<AffineTransformationModifier, AffineTransformation, &AffineTransformationModifier::setTargetCell>(),
				"This 3x4 matrix specifies the target cell shape. It is used when :py:attr:`.relative_mode` == ``False``. "
				"\n\n"
				"The first three columns of the matrix specify the three edge vectors of the target cell. "
				"The fourth column defines the origin vector of the target cell.")
		.def_property("relative_mode", &AffineTransformationModifier::relativeMode, &AffineTransformationModifier::setRelativeMode,
				"Selects the operation mode of the modifier."
				"\n\n"
				"If ``relative_mode==True``, the modifier transforms elements "
				"by applying the matrix given by the :py:attr:`.transformation` parameter."
				"\n\n"
				"If ``relative_mode==False``, the modifier transforms elements "
				"such that the old simulation cell will have the shape given by the the :py:attr:`.target_cell` parameter after the transformation."
				"\n\n"
				":Default: ``True``\n")
		.def_property("only_selected", &AffineTransformationModifier::selectionOnly, &AffineTransformationModifier::setSelectionOnly,
				"Controls whether the modifier should affect only on currently selected elements (e.g. selected particles)."
				"\n\n"
				":Default: ``False``\n")
	;
	modifier_operate_on_list(AffineTransformationModifier_py, std::mem_fn(&AffineTransformationModifier::delegates), "operate_on", 
			"A set of strings specifying the kinds of data elements this modifier should operate on. "
			"By default the set contains all data element types supported by the modifier. "
			"\n\n"
			":Default: ``{'particles', 'vector_properties', 'cell', 'surfaces'}``\n");


	auto ReplicateModifier_py = ovito_class<ReplicateModifier, MultiDelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier replicates all particles and bonds to generate periodic images. ")
		.def_property("num_x", &ReplicateModifier::numImagesX, &ReplicateModifier::setNumImagesX,
				"A positive integer specifying the number of copies to generate in the *x* direction (including the existing primary image)."
				"\n\n"
				":Default: 1\n")
		.def_property("num_y", &ReplicateModifier::numImagesY, &ReplicateModifier::setNumImagesY,
				"A positive integer specifying the number of copies to generate in the *y* direction (including the existing primary image)."
				"\n\n"
				":Default: 1\n")
		.def_property("num_z", &ReplicateModifier::numImagesZ, &ReplicateModifier::setNumImagesZ,
				"A positive integer specifying the number of copies to generate in the *z* direction (including the existing primary image)."
				"\n\n"
				":Default: 1\n")
		.def_property("adjust_box", &ReplicateModifier::adjustBoxSize, &ReplicateModifier::setAdjustBoxSize,
				"Controls whether the simulation cell is resized. "
				"If ``True``, the simulation cell is accordingly extended to fit the replicated data. "
				"If ``False``, the original simulation cell size (containing only the primary image of the system) is maintained. "
				"\n\n"
				":Default: ``True``\n")
		.def_property("unique_ids", &ReplicateModifier::uniqueIdentifiers, &ReplicateModifier::setUniqueIdentifiers,
				"If ``True``, the modifier automatically generates new unique IDs for each copy of particles. "
				"Otherwise, the replica will keep the same IDs as the original particles, which is typically not what you want. "
				"\n\n"
				"Note: This option has no effect if the input particles do not already have numeric IDs (i.e. the ``'Particle Identifier` property does not exist). "
				"\n\n"
				":Default: ``True``\n")
	;
	modifier_operate_on_list(ReplicateModifier_py, std::mem_fn(&ReplicateModifier::delegates), "operate_on", 
			"A set of strings specifying the kinds of data elements this modifier should operate on. "
			"By default the set contains all data element types supported by the modifier. "
			"\n\n"
			":Default: ``{'particles', 'vector_properties', 'cell', 'surfaces'}``\n");

	ovito_class<ClearSelectionModifier, GenericPropertyModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier clears the current selection. It can operate on several kinds of data elements: "
			"\n\n"
			"  * Particles (removing the ``'Selection'`` :ref:`particle property <particle-types-list>`)\n"
			"  * Bonds (removing the ``'Selection'`` :ref:`bond property <bond-types-list>`)\n"
			"\n\n"
			"The modifier will act on particles only by default. This can be changed by setting the :py:attr:`.operate_on` field. ")
		.def_property("operate_on", modifierPropertyClassGetter(), modifierPropertyClassSetter(),
				"Selects the kind of data elements this modifier operate on. "
				"Supported values are: ``'particles'``, ``'bonds'``, ``'voxels'``. "
				"\n\n"
				":Default: ``'particles'``\n")		
	;

	ovito_class<InvertSelectionModifier, GenericPropertyModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier inverts the current selection. It can operate on several kinds of data elements: "
			"\n\n"
			"  * Particles (inverting the ``'Selection'`` :ref:`particle property <particle-types-list>`)\n"
			"  * Bonds (inverting the ``'Selection'`` :ref:`bond property <bond-types-list>`)\n"
			"\n\n"
			"The modifier will act on particles only by default. This can be changed by setting the :py:attr:`.operate_on` field. ")
		.def_property("operate_on", modifierPropertyClassGetter(), modifierPropertyClassSetter(),
				"Selects the kind of data elements this modifier operate on. "
				"Supported values are: ``'particles'``, ``'bonds'``, ``'voxels'``. "
				"\n\n"
				":Default: ``'particles'``\n")		
	;
			
	auto ColorCodingModifier_py = ovito_class<ColorCodingModifier, DelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"Assigns colors to elements based on some scalar input property to visualize the property values. "
			"The modifier can operate on several kinds of data elements: "
			"\n\n"
			"  * Particles (setting the ``'Color'`` :ref:`particle property <particle-types-list>`)\n"
			"  * Particle vectors (setting the ``'Vector Color'`` :ref:`particle property <particle-types-list>`)\n"
			"  * Bonds (setting the ``'Color'`` :ref:`bond property <bond-types-list>`)\n"
			"\n\n"
			"The modifier will act on particles by default. You can change this by setting the modifier's :py:attr:`.operate_on` field. "
			"\n\n"
			"Usage example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/color_coding.py\n"
			"   :lines: 6-\n"
			"\n"
			"If the :py:attr:`.start_value` and :py:attr:`.end_value` parameters are not explicitly specified during modifier construction, "
			"then the modifier will automatically adjust them to the minimum and maximum values of the input property at the time it "
			"is inserted into a data pipeline. "
			"\n\n"
			"The :py:class:`~ovito.vis.ColorLegendOverlay` may be used in conjunction with a :py:class:`ColorCodingModifier` "
			"to insert a color legend into rendered images. "
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Color`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The computed particle colors if :py:attr:`.operate_on` is set to ``'particles'``.\n"
			" * ``Vector Color`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The computed arrow colors if :py:attr:`.operate_on` is set to ``'vectors'``.\n"
			" * ``Color`` (:py:class:`~ovito.data.BondProperty`):\n"
			"   The compute bond colors if :py:attr:`.operate_on` is set to ``'bonds'``.\n"
			"\n")
		
		.def_property("property", &ColorCodingModifier::sourceProperty, [](ColorCodingModifier& mod, py::object val) {
					auto* delegate = static_object_cast<ColorCodingModifierDelegate>(mod.delegate());
					mod.setSourceProperty(convertPythonPropertyReference(val, delegate ? &delegate->propertyClass() : nullptr));
				},
				"The name of the input property that should be used to color elements. "
				"\n\n"
				"If :py:attr:`.operate_on` is set to ``'particles'`` or ``'vectors'``, this can be one of the :ref:`standard particle properties <particle-types-list>` "
				"or a name of a user-defined :py:class:`~ovito.data.ParticleProperty`. "
				"If :py:attr:`.operate_on` is set to ``'bonds'``, this can be one of the :ref:`standard bond properties <bond-types-list>` "
				"or a name of a user-defined :py:class:`~ovito.data.BondProperty`. "
				"\n\n"
				"When the input property has multiple components, then a component name must be appended to the property base name, e.g. ``\"Velocity.X\"``. "
				"\n\n"
				"Note: Make sure that :py:attr:`.operate_on` is set to the desired value *before* setting this attribute, "
				"because changing :py:attr:`.operate_on` will implicitly reset the :py:attr:`!property` attribute. ")
		.def_property("start_value", &ColorCodingModifier::startValue, &ColorCodingModifier::setStartValue,
				"This parameter defines, together with the :py:attr:`.end_value` parameter, the normalization range for mapping the input property values to colors.")
		.def_property("end_value", &ColorCodingModifier::endValue, &ColorCodingModifier::setEndValue,
				"This parameter defines, together with the :py:attr:`.start_value` parameter, the normalization range for mapping the input property values to colors.")
		.def_property("gradient", &ColorCodingModifier::colorGradient, &ColorCodingModifier::setColorGradient,
				"The color gradient object, which is responsible for mapping normalized property values to colors. "
				"Available gradient types are:\n"
				" * ``ColorCodingModifier.BlueWhiteRed()``\n"
				" * ``ColorCodingModifier.Grayscale()``\n"
				" * ``ColorCodingModifier.Hot()``\n"
				" * ``ColorCodingModifier.Jet()``\n"
				" * ``ColorCodingModifier.Magma()``\n"
				" * ``ColorCodingModifier.Rainbow()`` [default]\n"
				" * ``ColorCodingModifier.Viridis()``\n"
				" * ``ColorCodingModifier.Custom(\"<image file>\")``\n"
				"\n"
				"The last color map constructor expects the path to an image file on disk, "
				"which will be used to create a custom color gradient from a row of pixels in the image.")
		.def_property("only_selected", &ColorCodingModifier::colorOnlySelected, &ColorCodingModifier::setColorOnlySelected,
				"If ``True``, only selected elements will be affected by the modifier and the existing colors "
				"of unselected elements will be preserved; if ``False``, all elements will be colored."
				"\n\n"
				":Default: ``False``\n")
		.def_property("operate_on", modifierDelegateGetter(), modifierDelegateSetter(ColorCodingModifierDelegate::OOClass()),
				"Selects the kind of data elements this modifier should operate on. "
				"Supported values are: ``'particles'``, ``'bonds'``, ``'vectors'``. "
				"\n\n"
				"Note: Assigning a new value to this attribute resets the :py:attr:`.property` field. "
				"\n\n"
				":Default: ``'particles'``\n")
	;

	ovito_abstract_class<ColorCodingGradient, RefTarget>(ColorCodingModifier_py)
		.def("valueToColor", &ColorCodingGradient::valueToColor)
	;

	ovito_class<ColorCodingHSVGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Rainbow")
	;
	ovito_class<ColorCodingGrayscaleGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Grayscale")
	;
	ovito_class<ColorCodingHotGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Hot")
	;
	ovito_class<ColorCodingJetGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Jet")
	;
	ovito_class<ColorCodingBlueWhiteRedGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "BlueWhiteRed")
	;
	ovito_class<ColorCodingViridisGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Viridis")
	;
	ovito_class<ColorCodingMagmaGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Magma")
	;
	ovito_class<ColorCodingImageGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Image")
		.def("load_image", &ColorCodingImageGradient::loadImage)
	;
	ColorCodingModifier_py.def_static("Custom", [](const QString& filename) {
	    OORef<ColorCodingImageGradient> gradient(new ColorCodingImageGradient(ScriptEngine::activeDataset()));
    	gradient->loadImage(filename);
    	return gradient;
	});

	ovito_class<SelectTypeModifier, GenericPropertyModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"Selects all elements of a certain type (e.g. atoms of a chemical type). "
			"The modifier can operate on different kinds of data elements: "
			"\n\n"
			"  * Particles\n"
			"  * Bonds\n"
			"\n\n"
			"The modifier will act on particles by default. You can change this by setting the modifier's :py:attr:`.operate_on` field. "
			"\n\n"
			"Usage example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/select_type_modifier.py\n"
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Selection`` (:py:class:`~ovito.data.ParticleProperty` or :py:class:`~ovito.data.BondProperty`):\n"
			"   The output property will be set to 1 for particles/bonds whose type is contained in :py:attr:`.types` and 0 for others.\n"
			" * ``SelectType.num_selected`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of elements that were selected by the modifier.\n"
			"\n")
		.def_property("property", &SelectTypeModifier::sourceProperty, [](SelectTypeModifier& mod, py::object val) {
					mod.setSourceProperty(convertPythonPropertyReference(val, mod.propertyClass()));
				},
				"The name of the property to use as input; must be an integer property. "
				"\n\n"
				"When selecting particles, possible input properties are ``\'Particle Type\'`` and ``\'Structure Type\'``, for example. "
				"When selecting bonds, ``'Bond Type'`` is a typical input property for this modifier. "
				"\n\n"
				"Note: Make sure that :py:attr:`.operate_on` is set to the desired value *before* setting this attribute, "
				"because changing :py:attr:`.operate_on` will implicitly reset the :py:attr:`!property` attribute. "
				"\n\n"
				":Default: ``''``\n")
		.def_property("operate_on", modifierPropertyClassGetter(), modifierPropertyClassSetter(),
				"Selects the kind of data elements this modifier should select. "
				"Supported values are: ``'particles'``, ``'bonds'``. "
				"\n\n"
				"Note: Assigning a new value to this attribute resets the :py:attr:`.property` field. "
				"\n\n"
				":Default: ``'particles'``\n")
		// Required by implementation of SelectTypeModifier.types attribute:
		.def_property("_selected_type_ids", &SelectTypeModifier::selectedTypeIDs, &SelectTypeModifier::setSelectedTypeIDs)
		.def_property("_selected_type_names", &SelectTypeModifier::selectedTypeNames, &SelectTypeModifier::setSelectedTypeNames)
	;

	auto HistogramModifier_py = ovito_class<HistogramModifier, GenericPropertyModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"Generates a histogram from the values of a property. The modifier can operate on properties of different kinds of elements: "
			"\n\n"
			"  * Particles (:py:class:`~ovito.data.ParticleProperty`)\n"
			"  * Bonds (:py:class:`~ovito.data.BondProperty`)\n"
			"  * Voxel grids (:py:class:`~ovito.data.VoxelProperty`)\n"
			"\n\n"
			"The modifier will act on particles by default. You can change this by setting the modifier's :py:attr:`.operate_on` field. "
			"\n\n"
			"The value range of the histogram is determined automatically from the minimum and maximum values of the selected property "
			"unless :py:attr:`.fix_xrange` is set to ``True``. In this case the range of the histogram is controlled by the "
			":py:attr:`.xrange_start` and :py:attr:`.xrange_end` parameters."
			"\n\n"
			"Usage example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/histogram_modifier.py\n"
			"\n\n")
		.def_property("operate_on", modifierPropertyClassGetter(), modifierPropertyClassSetter(),
				"Selects the kind of data elements this modifier operate on. "
				"Supported values are: ``'particles'``, ``'bonds'``, ``'oxels'``. "
				"\n\n"
				"Note: Assigning a new value to this attribute resets the :py:attr:`.property` field. "
				"\n\n"
				":Default: ``'particles'``\n")		
		.def_property("property", &HistogramModifier::sourceProperty, [](HistogramModifier& mod, py::object val) {					
					mod.setSourceProperty(convertPythonPropertyReference(val, mod.propertyClass()));
				},
				"The name of the input property for which to compute the histogram. "
				"For vector properties a component name must be appended in the string, e.g. ``\"Velocity.X\"``. "
				"\n\n"
				"Note: Make sure that :py:attr:`.operate_on` is set to the desired value *before* setting this attribute, "
				"because changing :py:attr:`.operate_on` will implicitly reset the :py:attr:`!property` attribute. "
				"\n\n"
				":Default: ``''``\n")
		.def_property("bin_count", &HistogramModifier::numberOfBins, &HistogramModifier::setNumberOfBins,
				"The number of histogram bins."
				"\n\n"
				":Default: 200\n")
		.def_property("fix_xrange", &HistogramModifier::fixXAxisRange, &HistogramModifier::setFixXAxisRange,
				"Controls how the value range of the histogram is determined. If false, the range is chosen automatically by the modifier to include "
				"all input values. If true, the range is specified manually using the :py:attr:`.xrange_start` and :py:attr:`.xrange_end` attributes."
				"\n\n"
				":Default: ``False``\n")
		.def_property("xrange_start", &HistogramModifier::xAxisRangeStart, &HistogramModifier::setXAxisRangeStart,
				"If :py:attr:`.fix_xrange` is true, then this specifies the lower end of the value range covered by the histogram."
				"\n\n"
				":Default: 0.0\n")
		.def_property("xrange_end", &HistogramModifier::xAxisRangeEnd, &HistogramModifier::setXAxisRangeEnd,
				"If :py:attr:`.fix_xrange` is true, then this specifies the upper end of the value range covered by the histogram."
				"\n\n"
				":Default: 0.0\n")
		.def_property("only_selected", &HistogramModifier::onlySelected, &HistogramModifier::setOnlySelected,
				"If ``True``, the histogram is computed only on the basis of currently selected particles or bonds. "
				"You can use this to restrict histogram calculation to a subset of particles/bonds. "
				"\n\n"
				":Default: ``False``\n")
		.def_property_readonly("_histogram_data", py::cpp_function([](HistogramModifier& mod) {
				HistogramModifierApplication* modApp = dynamic_object_cast<HistogramModifierApplication>(mod.someModifierApplication());
				if(!modApp) mod.throwException(HistogramModifier::tr("Modifier has not been evaluated yet. Histogram data is not yet available."));
				py::array_t<size_t> array(modApp->histogramData().size(), modApp->histogramData().data(), py::cast(&mod));
				// Mark array as read-only.
				reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
				return array;
			}))
	;
	ovito_class<HistogramModifierApplication, ModifierApplication>{m};
	
	ovito_class<ScatterPlotModifier, GenericPropertyModifier>{m};
	ovito_class<ScatterPlotModifierApplication, ModifierApplication>{m};
	
	ovito_class<AssignColorModifier, DelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"Assigns a uniform color to all selected elements. The modifier can operate on several kinds of data elements: "
			"\n\n"
			"  * Particles (setting the ``'Color'`` :ref:`particle property <particle-types-list>`)\n"
			"  * Particle vectors (setting the ``'Vector Color'`` :ref:`particle property <particle-types-list>`)\n"
			"  * Bonds (setting the ``'Color'`` :ref:`bond property <bond-types-list>`)\n"
			"\n\n"
			"The modifier will act on particles by default. You can change this by setting the modifier's :py:attr:`.operate_on` field. "
			"\n\n"
			"The modifier uses the ``'Selection'`` property as input to decide which elements "
			"are being assigned the color. If the  ``'Selection'`` property does not exist in the modifier's input, "
			"the color will be assigned to all elements. ")
		.def_property("color", VectorGetter<AssignColorModifier, Color, &AssignColorModifier::color>(), 
							   VectorSetter<AssignColorModifier, Color, &AssignColorModifier::setColor>(),
				"The uniform RGB color that will be assigned to elements by the modifier."
				"\n\n"
				":Default: ``(0.3, 0.3, 1.0)``\n")
		.def_property("keep_selection", &AssignColorModifier::keepSelection, &AssignColorModifier::setKeepSelection)
		.def_property("operate_on", modifierDelegateGetter(), modifierDelegateSetter(AssignColorModifierDelegate::OOClass()),
				"Selects the kind of data elements this modifier should operate on. "
				"Supported values are: ``'particles'``, ``'bonds'``, ``'vectors'``. "
				"\n\n"
				":Default: ``'particles'``\n")
	;

	auto DeleteSelectedModifier_py = ovito_class<DeleteSelectedModifier, MultiDelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier deletes the currently selected elements of the following kinds: "
			"\n\n"
			"  * Particles (deleting particles whose ``'Selection'`` :ref:`property <particle-types-list>` is non-zero)\n"
			"  * Bonds (deleting bonds whose ``'Selection'`` :ref:`property <bond-types-list>` is non-zero)\n"
			"\n\n"
			"The modifier will act on all of them by default. Restriction to a subset is possible by setting the :py:attr:`.operate_on` field. ")
	;	
	modifier_operate_on_list(DeleteSelectedModifier_py, std::mem_fn(&DeleteSelectedModifier::delegates), "operate_on", 
			"A set of strings specifying the kinds of data elements this modifier should operate on. "
			"By default the set contains all data element types supported by the modifier. "
			"\n\n"
			":Default: ``{'particles', 'bonds'}``\n");	

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

	ovito_class<SimulationCellObject, DataObject>(m,
			":Base class: :py:class:`ovito.data.DataObject`\n\n"
			"Stores the geometric shape and the boundary conditions of the simulation cell. "
			"A :py:class:`!SimulationCell` data object is typically part of a :py:class:`DataCollection` and can be retrieved through its :py:meth:`~DataCollection.expect` method: "
			"\n\n"
			".. literalinclude:: ../example_snippets/simulation_cell.py\n"
			"   :lines: 1-8\n"
			"\n\n"
			"The simulation cell geometry is stored as a 3x4 matrix (with column-major ordering). The first three columns of the matrix represent the three cell vectors "
			"and the last column is the position of the cell's origin. For two-dimensional datasets, the :py:attr:`is2D` flag ist set. "
			"In this case the third cell vector and the z-coordinate of the cell origin are ignored by OVITO. "
			"\n\n"
			".. literalinclude:: ../example_snippets/simulation_cell.py\n"
			"   :lines: 10-17\n"
			"\n\n"
			"The :py:class:`!SimulationCell` object behaves like a standard Numpy array of shape (3,4). Data access is read-only, however. "
			"If you want to manipulate the cell vectors, you have to use a ``with`` compound statement as follows: "
			"\n\n"
			".. literalinclude:: ../example_snippets/simulation_cell.py\n"
			"   :lines: 19-21\n"
			"\n\n"
			"A :py:class:`!SimulationCell` instance are always associated with a corresponding :py:class:`~ovito.vis.SimulationCellDisplay`, "
			"which controls the visual appearance of the simulation box. It can be accessed through "
			"the :py:attr:`~DataObject.display` attribute inherited from :py:class:`~ovito.data.DataObject`. "
			"\n\n"
			".. literalinclude:: ../example_snippets/simulation_cell.py\n"
			"   :lines: 23-\n"
			"\n\n",
			// Python class name:
			"SimulationCell")
		// Used by implementation of SimulationCell.pbc:
		.def_property("pbc_x", &SimulationCellObject::pbcX, &SimulationCellObject::setPbcX)
		.def_property("pbc_y", &SimulationCellObject::pbcY, &SimulationCellObject::setPbcY)
		.def_property("pbc_z", &SimulationCellObject::pbcZ, &SimulationCellObject::setPbcZ)
		.def_property("matrix", MatrixGetterCopy<SimulationCellObject, AffineTransformation, &SimulationCellObject::cellMatrix>(),
								MatrixSetter<SimulationCellObject, AffineTransformation, &SimulationCellObject::setCellMatrix>())
		.def_property("is2D", &SimulationCellObject::is2D, &SimulationCellObject::setIs2D,
				"Specifies whether the system is two-dimensional (true) or three-dimensional (false). "
				"For two-dimensional systems the PBC flag in the third direction (z) and the third cell vector are ignored. "
				"\n\n"
				":Default: ``false``\n")
		.def_property_readonly("volume", &SimulationCellObject::volume3D,
				"Computes the volume of the three-dimensional simulation cell.\n"
				"It is the absolute value of the determinant of the 3x3 cell submatrix.")
		.def_property_readonly("volume2D", &SimulationCellObject::volume2D,
				"Computes the area of the two-dimensional simulation cell (see :py:attr:`.is2D`).\n")
	;

	ovito_abstract_class<PeriodicDomainDataObject, DataObject>{m};		
	
	ovito_class<ElementType, RefTarget>{m}
	;

	ovito_abstract_class<PropertyObject, DataObject>(m,
			":Base class: :py:class:`ovito.data.DataObject`\n\n"
			"Stores the values for an array of elements (e.g. particle or bonds). "
			"\n\n"
			"In OVITO's data model, an arbitrary number of properties can be associated with data elements such as particle or bonds, "
			"each property being represented by a :py:class:`!Property` object. A :py:class:`!Property` "
			"is basically an array of values whose length matches the number of data elements. "
			"\n\n"
			":py:class:`!Property` is the common base class for the :py:class:`ParticleProperty` and :py:class:`BondProperty` "
			"specializations. "
			"\n\n"
			"**Data access**"
			"\n\n"
			"A :py:class:`!Property` object behaves almost like a Numpy array. For example, you can access the property value for the *i*-th data element using indexing:: "
			"\n\n"
			"     property = data.particle_properties['Velocity']\n"
			"     print('Velocity vector of first particle:', property[0])\n"
			"     print('Z-velocity of second particle:', property[1,2])\n"
			"     for v in property: print(v)\n"
			"\n\n"
			"Element indices start at zero. Properties can be either vectorial (e.g. velocity vectors are stored as an *N* x 3 array) "
			"or scalar (1-d array of length *N*). Length of the first array dimension is in both cases equal to "
			"the number of data elements (number of particles in the example above). Array elements can either be of data type ``float`` or ``int``. "
			"\n\n"
			"If necessary, you can cast a :py:class:`!Property` to a standard Numpy array:: "
			"\n\n"
			"     velocities = numpy.asarray(property)\n"
			"\n\n"
			"No data is copied during the conversion; the Numpy array will refer to the same memory as the :py:class:`!Property`. "
			"By default, the memory of a :py:class:`!Property` is write-protected. Thus, trying to modify property values will raise an error:: "
			"\n\n"
			"    property[0] = (0, 0, -4) # \"TypeError: 'ParticleProperty' object does not\n"
			"                             # support item assignment\"\n"
			"\n\n"
			"A direct modification is prevented by the system, because OVITO's data pipeline uses shallow data copies and needs to know when data objects are being modified. "
			"Only then results that depend on the changing data can be automatically recalculated. "
			"We need to explicitly announce a modification by using the :py:meth:`Property.modify` method and a Python ``with`` statement:: "
			"\n\n"
			"    with property.modify() as arr:\n"
			"        arr[0] = (0, 0, -4)\n"
			"\n\n"
			"Within the ``with`` compound statement, the variable ``arr`` refers to a *modifiable* Numpy array, allowing us to alter "
			"the per-particle data stored in the :py:class:`!Property` object. "
			"\n\n",
			// Python class name:
			"Property")
		// To mimic the numpy ndarray class:
		.def("__len__", &PropertyObject::size)
		.def_property_readonly("size", &PropertyObject::size)
		.def_property_readonly("data_type", &PropertyObject::dataType)
		// Used for Numpy array interface:
		.def_property_readonly("__array_interface__", &PropertyObject__array_interface__<true>)
		// Required for implementation of the Property.modify() method:
		.def_property_readonly("__mutable_array_interface__", &PropertyObject__array_interface__<false>)

		.def_property("name", &PropertyObject::name, &PropertyObject::setName,
				"The name of the property.")
		.def_property_readonly("components", &PropertyObject::componentCount,
				"The number of vector components if this is a vector property; or 1 if this is a scalar property.")
		
		// Used by the type_by_name() and type_by_id() methods:
		.def("_get_type_by_id", static_cast<ElementType* (PropertyObject::*)(int) const>(&PropertyObject::elementType))
		.def("_get_type_by_name", static_cast<ElementType* (PropertyObject::*)(const QString&) const>(&PropertyObject::elementType))
	;
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

/// Helper function that generates a getter function for the 'operate_on' attribute of a GenericPropertyModifier subclass
py::cpp_function modifierPropertyClassGetter()
{
	return [](const GenericPropertyModifier& mod) { 
			return mod.propertyClass() ? mod.propertyClass()->pythonName() : QString();
	};
}

/// Helper function that generates a setter function for the 'operate_on' attribute of a GenericPropertyModifier subclass.
py::cpp_function modifierPropertyClassSetter()
{
	return [](GenericPropertyModifier& mod, const QString& typeName) { 
		if(mod.propertyClass() && mod.propertyClass()->pythonName() == typeName)
			return;
		for(PropertyClassPtr propertyClass : PluginManager::instance().metaclassMembers<PropertyObject>()) {
			if(propertyClass->pythonName() == typeName) {
				mod.setPropertyClass(propertyClass);
				return;
			}
		}
		// Error: User did not specify a valid type name.
		// Now build the list of valid names to generate a helpful error message.
		QStringList propertyClassNames; 
		for(PropertyClassPtr propertyClass : PluginManager::instance().metaclassMembers<PropertyObject>()) {
			propertyClassNames.push_back(QString("'%1'").arg(propertyClass->pythonName()));
		}
		mod.throwException(GenericPropertyModifier::tr("'%1' is not a valid type of data element this modifier can operate on. Supported types are: (%2)")
				.arg(typeName).arg(propertyClassNames.join(QStringLiteral(", "))));
	};
}

};
