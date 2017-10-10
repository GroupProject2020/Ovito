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
#include <plugins/stdobj/properties/PropertyClass.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include <plugins/stdobj/properties/GenericPropertyModifier.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/stdobj/simcell/PeriodicDomainDataObject.h>
#include <plugins/stdobj/simcell/SimulationCellDisplay.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <core/app/PluginManager.h>

namespace Ovito { namespace StdObj {

using namespace PyScript;


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
	if(p.dataType() == PropertyStorage::Int) {
		OVITO_STATIC_ASSERT(sizeof(int) == 4);
		OVITO_STATIC_ASSERT(PropertyStorage::Int == qMetaTypeId<int>());
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
		ai["typestr"] = py::bytes("<i4");
#else
		ai["typestr"] = py::bytes(">i4");
#endif
	}
	else if(p.dataType() == PropertyStorage::Int64) {
		OVITO_STATIC_ASSERT(sizeof(qlonglong) == 8);
		OVITO_STATIC_ASSERT(PropertyStorage::Int64 == qMetaTypeId<qlonglong>());
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
		ai["typestr"] = py::bytes("<i8");
#else
		ai["typestr"] = py::bytes(">i8");
#endif
	}
	else if(p.dataType() == PropertyStorage::Float) {
#ifdef FLOATTYPE_FLOAT		
		OVITO_STATIC_ASSERT(sizeof(FloatType) == 4);
		OVITO_STATIC_ASSERT(PropertyStorage::Float == qMetaTypeId<float>());
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
		ai["typestr"] = py::bytes("<f4");
#else
		ai["typestr"] = py::bytes(">f4");
#endif
#else
		OVITO_STATIC_ASSERT(sizeof(FloatType) == 8);
		OVITO_STATIC_ASSERT(PropertyStorage::Float == qMetaTypeId<double>());
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

PYBIND11_PLUGIN(StdObj)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();
	
	py::options options;
	options.disable_function_signatures();

	py::module m("StdObj");

	ovito_abstract_class<GenericPropertyModifier, Modifier>{m}
	;

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

	ovito_class<SimulationCellDisplay, DisplayObject>(m,
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"Controls the visual appearance of :py:class:`~ovito.data.SimulationCell` objects."
			"The following script demonstrates how to change the line width of the simulation cell:"
			"\n\n"
			".. literalinclude:: ../example_snippets/simulation_cell_display.py\n")
		.def_property("line_width", &SimulationCellDisplay::cellLineWidth, &SimulationCellDisplay::setCellLineWidth,
				"The width of the simulation cell line (in simulation units of length)."
				"\n\n"
				":Default: 0.14% of the simulation box diameter\n")
		.def_property("render_cell", &SimulationCellDisplay::renderCellEnabled, &SimulationCellDisplay::setRenderCellEnabled,
				"Boolean flag controlling the cell's visibility in rendered images. "
				"If ``False``, the cell will only be visible in the interactive viewports. "
				"\n\n"
				":Default: ``True``\n")
		.def_property("rendering_color", &SimulationCellDisplay::cellColor, &SimulationCellDisplay::setCellColor,
				"The line color used when rendering the cell."
				"\n\n"
				":Default: ``(0, 0, 0)``\n")
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

	return m.ptr();
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(StdObj);

/// Helper function that converts a Python string to a C++ PropertyReference instance.
/// The function requires a property class to look up the property name string.
PropertyReference convertPythonPropertyReference(py::object src, const PropertyClass* propertyClass)
{
	if(src.is_none())
		return {};
	if(!propertyClass)
		throw Exception(QStringLiteral("Cannot set property field without an active property class."));

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
		str = src.cast<QString>();
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

}	// End of namespace
}	// End of namespace
