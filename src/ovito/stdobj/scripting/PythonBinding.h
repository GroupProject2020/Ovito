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

#pragma once


#include <ovito/stdobj/StdObj.h>
#include <ovito/stdobj/properties/PropertyReference.h>
#include <ovito/pyscript/binding/PythonBinding.h>

namespace pybind11 { namespace detail {

	/// Automatic PropertyReference -> Python string conversion
	/// Note that conversion in the other direction is not possible without additional information,
	/// because the property class is unknown.
    template<> struct type_caster<Ovito::StdObj::PropertyReference> {
    public:
        PYBIND11_TYPE_CASTER(Ovito::StdObj::PropertyReference, _("PropertyReference"));

        bool load(handle src, bool) {
			return false;
		}

        static handle cast(const Ovito::StdObj::PropertyReference& src, return_value_policy /* policy */, handle /* parent */) {
        	object s = pybind11::cast(src.nameWithComponent());
			return s.release();
        }
    };

	/// Automatic Python string <--> TypedPropertyReference conversion
    template<class PropertyObjectType> struct type_caster<Ovito::StdObj::TypedPropertyReference<PropertyObjectType>> {
    public:
        PYBIND11_TYPE_CASTER(Ovito::StdObj::TypedPropertyReference<PropertyObjectType>, _("PropertyReference<") + make_caster<PropertyObjectType>::name() + _(">"));

        bool load(handle src, bool) {
			using namespace Ovito;
			using namespace Ovito::StdObj;

			if(!src) return false;
			if(src.is_none())
				return true;

			try {
				int ptype = src.cast<int>();
				if(ptype == 0)
					throw Exception(QStringLiteral("User-defined property without a name is not acceptable."));
				if(PropertyObjectType::OOClass().standardProperties().contains(ptype) == false)
					throw Exception(QStringLiteral("%1 is not a valid standard property type ID.").arg(ptype));
				value = TypedPropertyReference<PropertyObjectType>(ptype);
				return true;
			}
			catch(const cast_error&) {}

			QString str;
			try {
				str = castToQString(src);
			}
			catch(const cast_error&) {
				return false;
			}

			QStringList parts = str.split(QChar('.'));
			if(parts.length() > 2)
				throw Exception(QStringLiteral("Too many dots in property name string."));
			else if(parts.length() == 0 || parts[0].isEmpty())
				throw Exception(QStringLiteral("Property name string is empty."));

			// Determine property type.
			QString name = parts[0];
			int type = PropertyObjectType::OOClass().standardPropertyIds().value(name, 0);

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
						QStringList standardNames = PropertyObjectType::OOClass().standardPropertyComponentNames(type);
						component = standardNames.indexOf(componentName);
						if(component < 0)
							throw Exception(QStringLiteral("Component name '%1' is not defined for particle property '%2'. Possible components are: %3").arg(parts[1]).arg(parts[0]).arg(standardNames.join(',')));
					}
					else {
						// Assume user-defined properties cannot be vectors.
						component = -1;
						name = parts.join(QChar('.'));
					}
				}
			}
			if(type == 0)
				value = TypedPropertyReference<PropertyObjectType>(name, component);
			else
				value = TypedPropertyReference<PropertyObjectType>(type, component);
			return true;
		}

        static handle cast(const Ovito::StdObj::TypedPropertyReference<PropertyObjectType>& src, return_value_policy /* policy */, handle /* parent */) {
        	object s = pybind11::cast(src.nameWithComponent());
			return s.release();
        }
    };

}} // namespace pybind11::detail

namespace Ovito { namespace StdObj {

namespace py = pybind11;

/// Helper function that generates a getter function for the 'operate_on' attribute of a modifier.
OVITO_STDOBJ_EXPORT py::cpp_function modifierPropertyContainerGetter(const PropertyFieldDescriptor& propertyField);

/// Helper function that generates a setter function for the 'operate_on' attribute of a modifier.
OVITO_STDOBJ_EXPORT py::cpp_function modifierPropertyContainerSetter(const PropertyFieldDescriptor& propertyField);

/// Helper function that converts a Python string to a C++ PropertyReference instance.
/// The function requires a property class to look up the property name string.
OVITO_STDOBJ_EXPORT PropertyReference convertPythonPropertyReference(py::object src, PropertyContainerClassPtr propertyClass);

}	// End of namespace
}	// End of namespace
