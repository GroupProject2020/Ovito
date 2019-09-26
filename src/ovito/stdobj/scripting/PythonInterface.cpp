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

#include <ovito/stdobj/StdObj.h>
#include <ovito/stdobj/scripting/PythonBinding.h>
#include <ovito/stdobj/properties/PropertyReference.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/stdobj/properties/GenericPropertyModifier.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/simcell/PeriodicDomainDataObject.h>
#include <ovito/stdobj/simcell/SimulationCellVis.h>
#include <ovito/stdobj/series/DataSeriesObject.h>
#include <ovito/stdobj/io/DataSeriesExporter.h>
#include <ovito/pyscript/binding/PythonBinding.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/dataset/DataSet.h>

namespace Ovito { namespace StdObj {

using namespace PyScript;

PYBIND11_MODULE(StdObjPython, m)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();

	py::options options;
	options.disable_function_signatures();

	ovito_abstract_class<GenericPropertyModifier, Modifier>{m}
	;

	auto SimulationCell_py = ovito_class<SimulationCellObject, DataObject>(m,
			":Base class: :py:class:`ovito.data.DataObject`"
			"\n\n"
			"Stores the geometric shape and the boundary conditions of the simulation cell. "
			"A :py:class:`!SimulationCell` data object is typically part of a :py:class:`DataCollection` and can be retrieved through its :py:attr:`~DataCollection.cell` property: "
			"\n\n"
			".. literalinclude:: ../example_snippets/simulation_cell.py\n"
			"   :lines: 1-8\n"
			"\n\n"
			"The simulation cell geometry is stored as a 3x4 matrix (with column-major ordering). The first three columns of the matrix represent the three cell vectors "
			"and the last column is the position of the cell's origin. For two-dimensional datasets, the :py:attr:`is2D` flag ist set. "
			"In this case the third cell vector and the z-coordinate of the cell origin are ignored by OVITO in many computations. "
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
			"A :py:class:`!SimulationCell` instance are always associated with a corresponding :py:class:`~ovito.vis.SimulationCellVis` "
			"controlling the visual appearance of the simulation box. It can be accessed through "
			"the :py:attr:`~DataObject.vis` attribute inherited from the :py:class:`~ovito.data.DataObject` base class. "
			"\n\n"
			".. literalinclude:: ../example_snippets/simulation_cell.py\n"
			"   :lines: 23-\n"
			"\n\n",
			// Python class name:
			"SimulationCell")
		.def_property_readonly("volume", &SimulationCellObject::volume3D,
				"Computes the volume of the three-dimensional simulation cell.\n"
				"The volume is the absolute value of the determinant of the 3x3 submatrix formed by the three cell vectors.")
		.def_property_readonly("volume2D", &SimulationCellObject::volume2D,
				"Computes the area of the two-dimensional simulation cell (see :py:attr:`.is2D`).\n")

		// Used by context manager interface:
		.def("make_writable", &SimulationCellObject::makeWritableFromPython)
		.def("make_readonly", &SimulationCellObject::makeReadOnlyFromPython)

		// For backward compatibility with OVITO 2.9.0:
		.def_property("matrix", MatrixGetter<SimulationCellObject, AffineTransformation, &SimulationCellObject::cellMatrix>(),
								MatrixSetter<SimulationCellObject, AffineTransformation, &SimulationCellObject::setCellMatrix>())

		// Used for Numpy array interface:
		.def_property_readonly("__array_interface__", [](SimulationCellObject& cell) {
			py::dict ai;
			ai["shape"] = py::make_tuple(3, 4);
			ai["strides"] = py::make_tuple(sizeof(AffineTransformation::element_type), sizeof(AffineTransformation::column_type));
#ifdef FLOATTYPE_FLOAT
			OVITO_STATIC_ASSERT(sizeof(AffineTransformation::element_type) == 4);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
			ai["typestr"] = py::bytes("<f4");
#else
			ai["typestr"] = py::bytes(">f4");
#endif
#else
			OVITO_STATIC_ASSERT(sizeof(AffineTransformation::element_type) == 8);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
			ai["typestr"] = py::bytes("<f8");
#else
			ai["typestr"] = py::bytes(">f8");
#endif
#endif
			if(cell.isWritableFromPython()) {
				ai["data"] = py::make_tuple(reinterpret_cast<std::intptr_t>(cell.cellMatrix().elements()), false);
			}
			else {
				ai["data"] = py::make_tuple(reinterpret_cast<std::intptr_t>(cell.cellMatrix().elements()), true);
			}
			ai["version"] = py::cast(3);
			return ai;
		})
	;
	// Property fields:
	createDataPropertyAccessors(SimulationCell_py, "is2D", &SimulationCellObject::is2D, &SimulationCellObject::setIs2D,
			"Specifies whether the system is two-dimensional (instead of three-dimensional). "
			"For two-dimensional systems, the PBC flag in the third direction (Z) and the third cell vector will typically be ignored. "
			"\n\n"
			":Default: ``False``\n");
	// Used by implementation of SimulationCell.pbc:
	createDataPropertyAccessors(SimulationCell_py, "pbc_x", &SimulationCellObject::pbcX, &SimulationCellObject::setPbcX);
	createDataPropertyAccessors(SimulationCell_py, "pbc_y", &SimulationCellObject::pbcY, &SimulationCellObject::setPbcY);
	createDataPropertyAccessors(SimulationCell_py, "pbc_z", &SimulationCellObject::pbcZ, &SimulationCellObject::setPbcZ);

	ovito_class<SimulationCellVis, DataVis>(m,
			":Base class: :py:class:`ovito.vis.DataVis`"
			"\n\n"
			"Controls the visual appearance of the simulation cell. "
			"An instance of this class is attached to the :py:class:`~ovito.data.SimulationCell` object "
			"and can be accessed through its :py:attr:`~ovito.data.DataObject.vis` field. "
			"See also the corresponding :ovitoman:`user manual page <../../display_objects.simulation_cell>` for this visual element. "
			"\n\n"
			"The following example script demonstrates how to change the display line width and rendering color of the simulation cell "
			"loaded from an input simulation file:"
			"\n\n"
			".. literalinclude:: ../example_snippets/simulation_cell_vis.py\n")
		.def_property("line_width", &SimulationCellVis::cellLineWidth, &SimulationCellVis::setCellLineWidth,
				"The width of the simulation cell line (in simulation units of length)."
				"\n\n"
				":Default: 0.14% of the simulation box diameter\n")
		.def_property("render_cell", &SimulationCellVis::renderCellEnabled, &SimulationCellVis::setRenderCellEnabled,
				"Boolean flag controlling the cell's visibility in rendered images. "
				"If ``False``, the cell will only be visible in the interactive viewports. "
				"\n\n"
				":Default: ``True``\n")
		.def_property("rendering_color", &SimulationCellVis::cellColor, &SimulationCellVis::setCellColor,
				"The line color used when rendering the cell."
				"\n\n"
				":Default: ``(0, 0, 0)``\n")
	;

	auto PeriodicDomainDataObject_py = ovito_abstract_class<PeriodicDomainDataObject, DataObject>(m,
		":Base class: :py:class:`ovito.data.DataObject`\n\n",
		// Python class name:
		"PeriodicDomainObject")
	;
	createDataSubobjectAccessors(PeriodicDomainDataObject_py, "domain", &PeriodicDomainDataObject::domain, &PeriodicDomainDataObject::setDomain,
		"The :py:class:`~ovito.data.SimulationCell` describing the (possibly periodic) domain which this "
		"object is embedded in.");

	auto PropertyContainer_py = ovito_abstract_class<PropertyContainer, DataObject>(m,
		":Base class: :py:class:`ovito.data.DataObject`"
		"\n\n"
		"A dictionary-like object storing a set of :py:class:`Property` objects."
		"\n\n"
		"It implements the ``collections.abc.Mapping`` interface. That means it can be used "
		"like a standard read-only Python ``dict`` object to access the properties by name, e.g.: "
		"\n\n"
		".. literalinclude:: ../example_snippets/property_container.py\n"
		"	:lines: 7-11\n"
		"\n\n"
		"New properties can be added with the :py:meth:`.create_property` method. "
		"\n\n"
		"OVITO provides several concrete implementations of the abstract :py:class:`!PropertyContainer` base class: "
		"\n\n"
		"    * :py:class:`Particles`\n"
		"    * :py:class:`Bonds`\n"
		"    * :py:class:`VoxelGrid`\n"
		"    * :py:class:`DataSeries`\n")

		.def_property_readonly("count", &PropertyContainer::elementCount,
			"The number of data elements in this container, e.g. the number of particles. This value is always equal to the lengths of the :py:class:`Property` arrays managed by this container. ")

		// Required by implementation of create_property() method:
		.def("standard_property_type_id", [](const PropertyContainer& container, const QString& name) {
			return container.getOOMetaClass().standardPropertyTypeId(name);
		})
		.def("create_standard_property", [](PropertyContainer& container, int type, bool initializeMemory, size_t elementCountHint) {
			// Make sure it is safe to modify the property container.
			ensureDataObjectIsMutable(container);
			// Build a data object path from the property container up to the data collection.
			ConstDataObjectPath containerPath;
			const DataObject* obj = &container;
			do {
				containerPath.push_back(obj);
				obj = (obj->dependents().size() == 1) ? dynamic_object_cast<DataObject>(obj->dependents().front()) : nullptr;
			}
			while(obj);
			std::reverse(containerPath.begin(), containerPath.end());
			// Create the new property.
			return container.createProperty(type, initializeMemory, containerPath, elementCountHint);
		})
		.def("create_user_property", [](PropertyContainer& container, const QString& name, int dataType, size_t componentCount, size_t stride, bool initializeMemory, size_t elementCountHint) {
			// Make sure it is safe to modify the property container.
			ensureDataObjectIsMutable(container);
			// Create the new property.
			return container.createProperty(name, dataType, componentCount, stride, initializeMemory, elementCountHint);
		})
	;
	// Needed for implementation of Python dictionary interface of PropertyContainer class:
	expose_subobject_list(PropertyContainer_py, std::mem_fn(&PropertyContainer::properties), "properties", "PropertyList");

	auto ElementType_py = ovito_class<ElementType, DataObject>(m,
		":Base class: :py:class:`ovito.data.DataObject`"
		"\n\n"
		"Describes a single type of elements, for example a particular atom or bond type. "
		"A :py:class:`Property` object can store a set of element types in its :py:attr:`~Property.types` list. "
		"\n\n"
		":py:class:`!ElementType` is the base class for some specialized element types in OVITO: "
		"\n\n"
		"   * :py:class:`ParticleType` (used with typed properties in a :py:class:`Particles` container)\n"
		"   * :py:class:`BondType` (used with typed properties in a :py:class:`Bonds` container)\n")
	;
	createDataPropertyAccessors(ElementType_py, "id", &ElementType::numericId, &ElementType::setNumericId,
			"The unique numeric identifier of the type (typically a positive ``int``). ");
	createDataPropertyAccessors(ElementType_py, "color", &ElementType::color, &ElementType::setColor,
			"The display color used to render elements of this type. This is a tuple with RGB values in the range 0 to 1.");
	createDataPropertyAccessors(ElementType_py, "name", &ElementType::name, &ElementType::setName,
			"The display name of this type. If this string is empty, the numeric :py:attr:`.id` will be used when referring to this type. ");
	createDataPropertyAccessors(ElementType_py, "enabled", &ElementType::enabled, &ElementType::setEnabled,
			"This flag only has a meaning in the context of structure analysis and identification. "
			"Modifiers such as the :py:class:`~ovito.modifiers.PolyhedralTemplateMatchingModifier` or the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` "
			"manage a list of structural types that they can identify (e.g. FCC, BCC, etc.). The identification of individual structure types "
			"can be turned on or off by setting their :py:attr:`!enabled` flag.");

	auto Property_py = ovito_abstract_class<PropertyObject, DataObject>(m,
			":Base class: :py:class:`ovito.data.DataObject`"
			"\n\n"
			"Stores the property values for an array of data elements (e.g. particles, bonds or voxels). "
			"\n\n"
			"Each particle property, for example, is represented by one :py:class:`!Property` object storing the property values for all particles. "
			"Thus, a :py:class:`!Property` object is basically an array of values whose length matches the number of data elements. "
			"\n\n"
			"All :py:class:`!Property` objects belonging to the same class of data elements, for example all particle properties, are managed by "
			"a :py:class:`PropertyContainer`. In the case of particle properties, the corresponding container class is the "
			":py:class:`Particles` class, which is a specialization of the generic :py:class:`PropertyContainer` base class. "
			"\n\n"
			"**Data access**"
			"\n\n"
			"A :py:class:`!Property` object behaves almost like a Numpy array. For example, you can access the property value for the *i*-th data element using indexing:: "
			"\n\n"
			"     positions = data.particles['Position']\n"
			"     print('Position of first particle:', positions[0])\n"
			"     print('Z-coordinate of second particle:', positions[1,2])\n"
			"     for xyz in positions: \n"
			"         print(xyz)\n"
			"\n\n"
			"Element indices start at zero. Properties can be either vectorial (e.g. velocity vectors are stored as an *N* x 3 array) "
			"or scalar (1-d array of length *N*). The length of the first array dimension is in both cases equal to "
			"the number of data elements (number of particles in the example above). Array elements can either be of data type ``float`` or ``int``. "
			"\n\n"
			"If necessary, you can cast a :py:class:`!Property` to a standard Numpy array:: "
			"\n\n"
			"     numpy_array = numpy.asarray(positions)\n"
			"\n\n"
			"No data is copied during this conversion; the Numpy array will reference the same memory as the :py:class:`!Property`. "
			"The internal memory array of a :py:class:`!Property` is write-protected by default to prevent unattended data modifications. "
			"Thus, trying to modify property values will raise an error:: "
			"\n\n"
			"    positions[0] = (0,2,4) # Raises \"ValueError: assignment destination is read-only\"\n"
			"\n\n"
			"A direct modification is prevented by the system, because OVITO's data pipeline uses shallow data copies and needs to know when data objects are being modified. "
			"We need to explicitly announce a modification by using Python's ``with`` statement:: "
			"\n\n"
			"    with positions:\n"
			"        positions[0] = (0,2,4)\n"
			"\n\n"
			"Within the ``with`` compound statement, the array is temporarily made writable, allowing you to alter "
			"the per-element data stored in the :py:class:`!Property` object. "
			"\n\n"
			"**Typed properties**"
			"\n\n"
			"The standard particle property ``'Particle Type'`` stores the types of particles encoded as integer values, e.g.: "
			"\n\n"
			"    >>> data = node.compute()\n"
			"    >>> tprop = data.particles['Particle Type']\n"
			"    >>> print(tprop[...])\n"
			"    [2 1 3 ..., 2 1 2]\n"
			"\n\n"
			"Here, each number in the property array refers to one of the particle types (e.g. 1=Cu, 2=Ni, 3=Fe, etc.). The defined particle types, each one represented by "
			"an instance of the :py:class:`ParticleType` auxiliary class, are stored in the :py:attr:`.types` array "
			"of the :py:class:`!Property`. Each type has a unique :py:attr:`~ElementType.id`, a human-readable :py:attr:`~ElementType.name` "
			"and other attributes like :py:attr:`~ElementType.color` and :py:attr:`~ParticleType.radius` that control the "
			"visual appearance of particles belonging to the type:"
			"\n\n"
			"    >>> for type in tprop.types:\n"
			"    ...     print(type.id, type.name, type.color, type.radius)\n"
			"    ... \n"
			"    1 Cu (0.188 0.313 0.972) 0.74\n"
			"    2 Ni (0.564 0.564 0.564) 0.77\n"
			"    3 Fe (1 0.050 0.050) 0.74\n"
			"\n\n"
			"IDs of types typically start at 1 and form a consecutive sequence as in the example above. "
			"Note, however, that the :py:attr:`.types` list may store the :py:class:`ParticleType` objects in an arbitrary order. "
			"Thus, in general, it is not valid to directly use a type ID as an index into the :py:attr:`.types` array. "
			"Instead, the :py:meth:`.type_by_id` method should be used to look up the :py:class:`ParticleType`:: "
			"\n\n"
			"    >>> for i,t in enumerate(tprop): # (loop over the type ID of each particle)\n"
			"    ...     print('Atom', i, 'is of type', tprop.type_by_id(t).name)\n"
			"    ...\n"
			"    Atom 0 is of type Ni\n"
			"    Atom 1 is of type Cu\n"
			"    Atom 2 is of type Fe\n"
			"    Atom 3 is of type Cu\n"
			"\n\n"
			"Similarly, a :py:meth:`.type_by_name` method exists that looks up a :py:attr:`ParticleType` by name. "
			"For example, to count the number of Fe atoms in a system:"
			"\n\n"
			"    >>> Fe_type_id = tprop.type_by_name('Fe').id   # Determine ID of the 'Fe' type\n"
			"    >>> numpy.count_nonzero(tprop == Fe_type_id)   # Count particles having that type ID\n"
			"    957\n"
			"\n\n"
			"Note that OVITO supports multiple type classifications. For example, in addition to the ``'Particle Type'`` standard particle property, "
			"which stores the chemical types of atoms (e.g. C, H, Fe, ...), the ``'Structure Type'`` property may hold the structural types computed for atoms "
			"(e.g. FCC, BCC, ...) maintaining its own list of known structure types in the :py:attr:`.types` array. ",
			// Python class name:
			"Property")
		// To mimic the numpy ndarray class:
		.def("__len__", &PropertyObject::size)
		.def_property_readonly("size", &PropertyObject::size)
		.def_property_readonly("data_type", &PropertyObject::dataType)
		.def_property_readonly("type", &PropertyObject::type)

		// Used by context manager interface:
		.def("make_writable", &PropertyObject::makeWritableFromPython)
		.def("make_readonly", &PropertyObject::makeReadOnlyFromPython)

		.def_property_readonly("name", &PropertyObject::name, "The name of the property.")
		.def_property_readonly("component_count", &PropertyObject::componentCount,
				"The number of vector components if this is a vector property; or 1 if this is a scalar property.")
		.def_property_readonly("component_names", &PropertyObject::componentNames,
				"The list of names of the vector components if this is a vector property. For example, for the ``Position`` particle property this field contains ``['X', 'Y', 'Z']``.")

		// Used by the type_by_name() and type_by_id() methods:
		.def("_get_type_by_id", static_cast<ElementType* (PropertyObject::*)(int) const>(&PropertyObject::elementType))
		.def("_get_type_by_name", static_cast<ElementType* (PropertyObject::*)(const QString&) const>(&PropertyObject::elementType))

		// Implementation of the Numpy array protocol:
		.def("__array__", [](PropertyObject& p, py::object requested_dtype) {
			// Construct a NumPy array referring to the internal memory of this Property object.
			py::dtype dtype;
			if(p.dataType() == PropertyStorage::Int) {
				dtype = py::dtype::of<int>();
			}
			else if(p.dataType() == PropertyStorage::Int64) {
				dtype = py::dtype::of<qlonglong>();
			}
			else if(p.dataType() == PropertyStorage::Float) {
				dtype = py::dtype::of<FloatType>();
			}
			else throw Exception("Cannot access property with this data type from Python.");
			if(!requested_dtype.is_none() && !dtype.is(requested_dtype)) {
				if(py::cast<bool>(dtype.attr("__eq__")(requested_dtype)) == false)
					throw Exception("Property: Cannot create NumPy array view with dtype other than the native data type of the property.");
			}
			py::array arr;
			if(p.componentCount() == 1) {
				arr = py::array(dtype, { p.size() }, { p.stride() },
								p.isWritableFromPython() ? p.data() : p.constData(), py::cast(p));
			}
			else if(p.componentCount() > 1) {
				arr = py::array(dtype, { p.size(), p.componentCount() }, { p.stride(), p.dataTypeSize() },
								p.isWritableFromPython() ? p.data() : p.constData(), py::cast(p));
			}
			else throw Exception("Cannot access empty property from Python.");

			// Mark NumPy array as read-only if Property is not marked as writable.
			if(!p.isWritableFromPython())
				reinterpret_cast<py::detail::PyArray_Proxy*>(arr.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;

			return arr;
		}, py::arg("dtype") = py::none())
	;
	expose_mutable_subobject_list(Property_py,
		std::mem_fn(&PropertyObject::elementTypes),
		[](PropertyObject& prop, int index, const ElementType* type) {
			ensureDataObjectIsMutable(prop);
			prop.insertElementType(index, type);
		},
		[](PropertyObject& prop, int index) {
			ensureDataObjectIsMutable(prop);
			prop.removeElementType(index);
		},
		"types", "ElementTypeList",
			  "The list of :py:class:`ElementType` instances attached to this property. "
			  "\n\n"
			  "Note that the element types may be stored in arbitrary order in this list. Thus, it is not valid to use a numeric type ID as an index into this list. ");

	py::enum_<PropertyStorage::StandardDataType>(Property_py, "DataType")
		.value("Int", PropertyStorage::Int)
		.value("Int64", PropertyStorage::Int64)
		.value("Float", PropertyStorage::Float)
	;

	auto DataSeries_py = ovito_abstract_class<DataSeriesObject, PropertyContainer>(m,
			":Base class: :py:class:`ovito.data.PropertyContainer`\n\n"
			"This object represents a series of 2d data points and is used for generating function and histogram plots. "
			"A data series mainly consists of an array of y-values and, optionally, an array of corresponding x-values, one for each data point. "
			"\n\n"
			"If the :py:attr:`.x` data array is not present, the x-coordinates of the data points are implicitly determined by the "
			":py:attr:`.interval` property, which specifies a range along the x-axis over which the data points are evenly distributed. "
			"This is used, for example, for histograms with equally sized bins that span a certain value range. "
			"Implicit x-coordinates of data points are obtained by evenly dividing the specified :py:attr:`.interval` into *N* equally sized bins, "
			"with *N* being the number of values in the :py:attr:`.y`-array. The x-coordinates of data points are then placed in the centers "
			"of the bins. "
			"\n\n"
			"Data series are typically generated by certain modifiers in a data pipeline which compute histograms and other 2d charts, e.g. "
			":py:class:`~ovito.modifiers.CoordinationAnalysisModifier` and :py:class:`~ovito.modifiers.HistogramModifier`. "
			"You can access the :py:class:`!DataSeries` objects via the :py:attr:`DataCollection.series <ovito.data.DataCollection.series>` "
			"field. "
			"\n\n",
			// Python class name:
			"DataSeries")

		.def_property_readonly("x", [](const DataSeriesObject& series) { return series.getProperty(DataSeriesObject::XProperty); },
			"Returns the :py:class:`~ovito.data.Property` storing the x-coordinates of this data series. "
			"Not every data series has explicit x-coordinates, so this may be ``None``. In this case, the x-coordinates of the "
			"data points are implicitly given by the :py:attr:`.interval` property of the data series and the number of "
			"data points distributed evenly along that x-interval. ")
		.def_property_readonly("y", [](const DataSeriesObject& series) { return series.getProperty(DataSeriesObject::YProperty); },
			"Returns the :py:class:`~ovito.data.Property` storing the x-coordinates of this data series. "
			"This may be a property with more than one component per data points, in which case this data series "
			"consists of a family of curves. ")
	;
	createDataPropertyAccessors(DataSeries_py, "title", &DataSeriesObject::title, &DataSeriesObject::setTitle,
		"The title of the data series, as it appears in the user interface.");
	// Used internally by the DataSeries.interval property implementation:
	createDataPropertyAccessors(DataSeries_py, "interval_start", &DataSeriesObject::intervalStart, &DataSeriesObject::setIntervalStart);
	createDataPropertyAccessors(DataSeries_py, "interval_end", &DataSeriesObject::intervalEnd, &DataSeriesObject::setIntervalEnd);

	py::enum_<DataSeriesObject::Type>(DataSeries_py, "Type")
		.value("User", DataSeriesObject::UserProperty)
		.value("X", DataSeriesObject::XProperty)
		.value("Y", DataSeriesObject::YProperty)
	;

	ovito_class<DataSeriesExporter, FileExporter>{m}
	;
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(StdObjPython);

/// Helper function that converts a Python string to a C++ PropertyReference instance.
/// The function requires a property class to look up the property name string.
PropertyReference convertPythonPropertyReference(py::object src, PropertyContainerClassPtr propertyClass)
{
	if(src.is_none())
		return {};
	if(!propertyClass)
		throw Exception(QStringLiteral("Cannot set property field without an active property container class."));

	try {
		int ptype = src.cast<int>();
		if(ptype == 0)
			throw Exception(QStringLiteral("User-defined property without a name is not acceptable."));
		if(propertyClass->standardProperties().contains(ptype) == false)
			throw Exception(QStringLiteral("%1 is not a valid standard property type ID.").arg(ptype));
		return PropertyReference(propertyClass, ptype);
	}
	catch(const py::cast_error&) {}

	QString str;
	try {
		str = castToQString(src);
	}
	catch(const py::cast_error&) {
		throw Exception(QStringLiteral("Invalid property name. Expected a string."));
	}

	QStringList parts = str.split(QChar('.'));
	if(parts.length() > 2)
		throw Exception(QStringLiteral("Too many dots in property name string."));
	else if(parts.length() == 0 || parts[0].isEmpty())
		throw Exception(QStringLiteral("Invalid property name. String is empty."));

	// Determine property type.
	QString name = parts[0];
	int type = propertyClass->standardPropertyIds().value(name, 0);

	// Determine vector component.
	int component = -1;
	if(parts.length() == 2) {
		// First try to convert component to integer.
		bool ok;
		component = parts[1].toInt(&ok) - 1;
		if(!ok) {
			if(type != 0) {
				// Perhaps the standard property's component name was used instead of an integer.
				const QString componentName = parts[1].toUpper();
				QStringList standardNames = propertyClass->standardPropertyComponentNames(type);
				component = standardNames.indexOf(componentName);
				if(component < 0)
					throw Exception(QStringLiteral("Component name '%1' is not defined for property '%2'. Possible components are: %3").arg(parts[1]).arg(parts[0]).arg(standardNames.join(',')));
			}
			else {
				// Assume user-defined properties cannot be vectors.
				component = -1;
				name = parts.join(QChar('.'));
			}
		}
	}
	if(type == 0)
		return PropertyReference(propertyClass, name, component);
	else
		return PropertyReference(propertyClass, type, component);
}

/// Helper function that generates a getter function for the 'operate_on' field of a modifier.
py::cpp_function modifierPropertyContainerGetter(const PropertyFieldDescriptor& propertyField)
{
	OVITO_ASSERT(!propertyField.isReferenceField());
	OVITO_ASSERT(propertyField.definingClass()->isDerivedFrom(Modifier::OOClass()));
	return [&](const Modifier& mod) {
		QString refStr;
		QVariant val = mod.getPropertyFieldValue(propertyField);
		OVITO_ASSERT_MSG(val.isValid() && val.canConvert<PropertyContainerReference>(), "modifierPropertyContainerGetter()", QString("The property field of object class %1 is not of type <PropertyContainerReference>.").arg(mod.metaObject()->className()).toLocal8Bit().constData());
		if(PropertyContainerReference propertyContainerRef = val.value<PropertyContainerReference>()) {
			refStr = propertyContainerRef.dataClass()->pythonName();
			if(propertyContainerRef.dataPath().isEmpty() == false)
				refStr += QChar(':') + propertyContainerRef.dataPath();
		}
		return refStr;
	};
}

/// Helper function that generates a setter function for the 'operate_on' field of a modifier.
py::cpp_function modifierPropertyContainerSetter(const PropertyFieldDescriptor& propertyField)
{
	OVITO_ASSERT(!propertyField.isReferenceField());
	OVITO_ASSERT(propertyField.definingClass()->isDerivedFrom(Modifier::OOClass()));
	return [&](Modifier& mod, const QString& refStr) {
		// First, parse the input string into a property container class
		// and a data object path.
		QStringRef dataClassStr, dataPathStr;
		int separatorPos = refStr.indexOf(QChar(':'));
		if(separatorPos == -1) {
			dataClassStr = &refStr;
		}
		else {
			dataClassStr = QStringRef(&refStr, 0, separatorPos);
			dataPathStr = QStringRef(&refStr, separatorPos+1, refStr.length() - separatorPos - 1);
		}
		// Get the currently selected property container from the modifier.
		QVariant val = mod.getPropertyFieldValue(propertyField);
		OVITO_ASSERT_MSG(val.isValid() && val.canConvert<PropertyContainerReference>(), "modifierPropertyContainerSetter()", QString("The property field of object class %1 is not of type <PropertyContainerReference>.").arg(mod.metaObject()->className()).toLocal8Bit().constData());
		PropertyContainerReference propertyContainerRef = val.value<PropertyContainerReference>();

		// Check if values are the same as the current subject.
		if(propertyContainerRef && propertyContainerRef.dataClass()->pythonName() == dataClassStr && propertyContainerRef.dataPath() == dataPathStr)
			return;

		// Look up the property container class.
		for(PropertyContainerClassPtr containerClass : PluginManager::instance().metaclassMembers<PropertyContainer>()) {
			if(containerClass->pythonName() == dataClassStr) {
				mod.setPropertyFieldValue(propertyField, QVariant::fromValue(PropertyContainerReference(containerClass, dataPathStr.toString())));
				return;
			}
		}
		// Error: User did not specify a valid string.
		// Now build the list of valid names to generate a helpful error message.
		QStringList containerClassNames;
		for(PropertyContainerClassPtr containerClass : PluginManager::instance().metaclassMembers<PropertyContainer>()) {
			containerClassNames.push_back(QString("'%1'").arg(containerClass->pythonName()));
		}
		mod.throwException(Modifier::tr("'%1' is not a valid element type this modifier can operate on. Supported types are: (%2)")
				.arg(dataClassStr.toString()).arg(containerClassNames.join(QStringLiteral(", "))));
	};
}

}	// End of namespace
}	// End of namespace
