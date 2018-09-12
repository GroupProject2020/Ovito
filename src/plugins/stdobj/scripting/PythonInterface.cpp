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

#include <plugins/stdobj/StdObj.h>
#include <plugins/stdobj/scripting/PythonBinding.h>
#include <plugins/stdobj/properties/PropertyReference.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include <plugins/stdobj/properties/PropertyContainer.h>
#include <plugins/stdobj/properties/GenericPropertyModifier.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/stdobj/simcell/PeriodicDomainDataObject.h>
#include <plugins/stdobj/simcell/SimulationCellVis.h>
#include <plugins/stdobj/series/DataSeriesObject.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <core/app/PluginManager.h>
#include <core/dataset/DataSet.h>

namespace Ovito { namespace StdObj {

using namespace PyScript;

// Exposes a PropertyStorage object as a Numpy array.
static py::object buildNumpyArray(const PropertyPtr& p, bool makeWritable, py::handle base)
{
	if(!p) 
		return py::none();

	std::vector<ssize_t> shape;
	std::vector<ssize_t> strides;

	shape.push_back(p->size());
	strides.push_back(p->stride());
	if(p->componentCount() > 1) {
		shape.push_back(p->componentCount());		
		strides.push_back(p->dataTypeSize());
	}
	else if(p->componentCount() == 0) {
		throw Exception("Cannot access empty property array from Python.");
	}
	py::dtype t;
	if(p->dataType() == PropertyStorage::Int) {
		t = py::dtype::of<int>();
	}
	else if(p->dataType() == PropertyStorage::Int64) {
		t = py::dtype::of<qlonglong>();
	}
	else if(p->dataType() == PropertyStorage::Float) {
		t = py::dtype::of<FloatType>();
	}
	else throw Exception("Cannot access property array of this data type from Python.");
	py::array arr(t, std::move(shape), std::move(strides), p->data(), base);
	// Mark array as read-only.
	if(!makeWritable)
		reinterpret_cast<py::detail::PyArray_Proxy*>(arr.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
	return std::move(arr);
}

PYBIND11_MODULE(StdObj, m)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();
	
	py::options options;
	options.disable_function_signatures();

	ovito_abstract_class<GenericPropertyModifier, Modifier>{m}
	;

	auto SimulationCell_py = ovito_class<SimulationCellObject, DataObject>(m,
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
			"A :py:class:`!SimulationCell` instance are always associated with a corresponding :py:class:`~ovito.vis.SimulationCellVis`, "
			"which controls the visual appearance of the simulation box. It can be accessed through "
			"the :py:attr:`~DataObject.vis` attribute inherited from :py:class:`~ovito.data.DataObject`. "
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
			"See also the corresponding `user manual page <../../display_objects.simulation_cell.html>`__ for this visual element. "
			"\n\n"
			"The following example script demonstrates how to change the line width of the simulation cell:"
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
		".. literalinclude:: ../example_snippets/particles_view.py\n"
		"	:lines: 7-11\n"
		"\n\n"
		"New properties can be added with the :py:meth:`.create_property` method. ")

		.def_property_readonly("count", &PropertyContainer::elementCount,
			"The number of data elements in this container, for example the number of particles, which is equal to the length of the :py:class:`Property` arrays in this container. ")

		// Required by implementation of create_property() method:
		.def("standard_property_type_id", [](const PropertyContainer& container, const QString& name) { 
			return container.getOOMetaClass().standardPropertyTypeId(name);
		})
		.def("create_standard_property", [](PropertyContainer& container, int type, bool initializeMemory) {
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
			return container.createProperty(type, initializeMemory, containerPath);
		})
		.def("create_user_property", [](PropertyContainer& container, const QString& name, int dataType, size_t componentCount, size_t stride, bool initializeMemory) {
			// Make sure it is safe to modify the property container.
			ensureDataObjectIsMutable(container);
			// Create the new property.
			return container.createProperty(name, dataType, componentCount, stride, initializeMemory);
		})		
	;
	// Needed for implementation of Python dictionary interface of PropertyContainer class:
	expose_subobject_list(PropertyContainer_py, std::mem_fn(&PropertyContainer::properties), "properties", "PropertyList");

	auto ElementType_py = ovito_class<ElementType, DataObject>{m}
	;
	createDataPropertyAccessors(ElementType_py, "id", &ElementType::numericId, &ElementType::setNumericId,
			"The unique numeric identifier of the type. ");
	createDataPropertyAccessors(ElementType_py, "color", &ElementType::color, &ElementType::setColor,
			"The display color used to render elements of this type. ");
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
			"     property = data.particles['Velocity']\n"
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
			"    property[0] = (0, 0, -4) # \"ValueError: assignment destination is read-only\"\n"
			"\n\n"
			"A direct modification is prevented by the system, because OVITO's data pipeline uses shallow data copies and needs to know when data objects are being modified. "
			"Only then results that depend on the changing data can be automatically recalculated. "
			"We need to explicitly announce a modification by using Python's ``with`` statement:: "
			"\n\n"
			"    with property:\n"
			"        property[0] = (0, 0, -4)\n"
			"\n\n"
			"Within the ``with`` compound statement, the array is temporarily made writable, allowing us to alter "
			"the per-particle data stored in the :py:class:`!Property` object. "
			"\n\n",
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
		.def_property_readonly("components", &PropertyObject::componentCount,
				"The number of vector components if this is a vector property; or 1 if this is a scalar property.")
		
		// Used by the type_by_name() and type_by_id() methods:
		.def("_get_type_by_id", static_cast<ElementType* (PropertyObject::*)(int) const>(&PropertyObject::elementType))
		.def("_get_type_by_name", static_cast<ElementType* (PropertyObject::*)(const QString&) const>(&PropertyObject::elementType))

		// Used for Numpy array interface:
		.def_property_readonly("__array_interface__", [](PropertyObject& p) {
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
			if(p.dataType() == PropertyStorage::Int) {
				OVITO_STATIC_ASSERT(sizeof(int) == 4);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
				ai["typestr"] = py::bytes("<i4");
#else
				ai["typestr"] = py::bytes(">i4");
#endif
			}
			else if(p.dataType() == PropertyStorage::Int64) {
				OVITO_STATIC_ASSERT(sizeof(qlonglong) == 8);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
				ai["typestr"] = py::bytes("<i8");
#else
				ai["typestr"] = py::bytes(">i8");
#endif
			}
			else if(p.dataType() == PropertyStorage::Float) {
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
			else throw Exception("Cannot access property with this data type from Python.");
			if(p.isWritableFromPython()) {
				ai["data"] = py::make_tuple(reinterpret_cast<std::intptr_t>(p.data()), false);
			}
			else {
				ai["data"] = py::make_tuple(reinterpret_cast<std::intptr_t>(p.constData()), true);
			}
			ai["version"] = py::cast(3);
			return ai;
		})
	;
	expose_mutable_subobject_list(Property_py,
		std::mem_fn(&PropertyObject::elementTypes), 
		std::mem_fn(&PropertyObject::insertElementType), 
		std::mem_fn(&PropertyObject::removeElementType), "types", "ElementTypeList",
			  "A (mutable) list of :py:class:`ElementType` instances. "
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
			"If the :py:attr:`.`x` data array is not present, the x-coordinates of the data points are implicitly determined by the "
			":py:attr:`.interval` property, which specifies a range along the x-axis over which the data points are evenly distributed. "
			"This is used, for example, for histograms with equally sized bins that span a certain value range. "
			"Implicit x-coordinates of data points are obtained by evenly dividing the specified :py:attr:`.interval` into *N* equally sized bins, "
			"with *N* being the number of values in the :py:attr:`.y`-array. The x-coordinates of data points are then placed in the centers "
			"of the bins. "
			"\n\n"
			"Data series are typically generated by certain modifiers in a data pipeline which compute histograms and other 2d charts, e.g. "
			":py:class:`~ovito.modifiers.CoordinationAnalysisModifier` and :py:class:`~ovito.modifiers.HistogramModifier`. "
			"You can access all :py:class:`!DataSeries` objects through the :py:attr:`DataCollection.series <ovito.data.DataCollection.series>` "
			"property, which returns a dictionary containing all data series. "
			"\n\n",
			// Python class name:
			"DataSeries")
	;
	createDataPropertyAccessors(DataSeries_py, "title", &DataSeriesObject::title, &DataSeriesObject::setTitle,
		"The title of the data series, which is used in the user interface");
	createDataPropertyAccessors(DataSeries_py, "interval_start", &DataSeriesObject::intervalStart, &DataSeriesObject::setIntervalStart);
	createDataPropertyAccessors(DataSeries_py, "interval_end", &DataSeriesObject::intervalEnd, &DataSeriesObject::setIntervalEnd);

	py::enum_<DataSeriesObject::Type>(DataSeries_py, "Type")
		.value("User", DataSeriesObject::UserProperty)
		.value("X", DataSeriesObject::XProperty)
		.value("Y", DataSeriesObject::YProperty)
	;
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(StdObj);

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

/// Helper function that generates a getter function for the 'operate_on' field of a GenericPropertyModifier subclass
py::cpp_function modifierPropertyClassGetter()
{
	return [](const GenericPropertyModifier& mod) {
		QString refStr;
		if(mod.subject()) {
			refStr = mod.subject().dataClass()->pythonName();
			if(mod.subject().dataPath().isEmpty() == false)
				refStr += QChar(':') + mod.subject().dataPath();
		}
		return refStr;
	};
}

/// Helper function that generates a setter function for the 'operate_on' field of a GenericPropertyModifier subclass.
py::cpp_function modifierPropertyClassSetter()
{
	return [](GenericPropertyModifier& mod, const QString& refStr) {
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
		// Check if values are the same as the current subject.
		if(mod.subject() && mod.subject().dataClass()->pythonName() == dataClassStr && mod.subject().dataPath() == dataPathStr)
			return;
		// Look up the property container class.
		for(PropertyContainerClassPtr containerClass : PluginManager::instance().metaclassMembers<PropertyContainer>()) {
			if(containerClass->pythonName() == dataClassStr) {
				mod.setSubject(PropertyContainerReference(containerClass, dataPathStr.toString()));
				return;
			}
		}
		// Error: User did not specify a valid string.
		// Now build the list of valid names to generate a helpful error message.
		QStringList containerClassNames;
		for(PropertyContainerClassPtr containerClass : PluginManager::instance().metaclassMembers<PropertyContainer>()) {
			containerClassNames.push_back(QString("'%1'").arg(containerClass->pythonName()));
		}
		mod.throwException(GenericPropertyModifier::tr("'%1' is not a valid element type this modifier can operate on. Supported types are: (%2)")
				.arg(dataClassStr.toString()).arg(containerClassNames.join(QStringLiteral(", "))));
	};
}

}	// End of namespace
}	// End of namespace
