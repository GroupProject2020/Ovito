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


#include <core/Core.h>
#include <core/oo/ReferenceEvent.h>
#include "PropertyFieldDescriptor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* This structure describes one member field of a RefMaker object that stores
* a property of that object.
******************************************************************************/
class OVITO_CORE_EXPORT NativePropertyFieldDescriptor : public PropertyFieldDescriptor
{
public:

	/// Inherit all constructors from base class.
	using PropertyFieldDescriptor::PropertyFieldDescriptor;

public:

	// Internal helper class that is used to specify the units for a controller
	// property field. Do not use this class directly but use the
	// SET_PROPERTY_FIELD_UNITS macro instead.
	struct PropertyFieldUnitsSetter : public NumericalParameterDescriptor {
		PropertyFieldUnitsSetter(NativePropertyFieldDescriptor& propfield, const QMetaObject* parameterUnitType, FloatType minValue = FLOATTYPE_MIN, FloatType maxValue = FLOATTYPE_MAX) {
			OVITO_ASSERT(propfield._parameterInfo == nullptr);
			propfield._parameterInfo = this;
			this->unitType = parameterUnitType;
			this->minValue = minValue;
			this->maxValue = maxValue;
		}
	};

	// Internal helper class that is used to specify the label text for a
	// property field. Do not use this class directly but use the
	// SET_PROPERTY_FIELD_LABEL macro instead.
	struct PropertyFieldDisplayNameSetter {
		PropertyFieldDisplayNameSetter(NativePropertyFieldDescriptor& propfield, const QString& label) {
			OVITO_ASSERT(propfield._displayName.isEmpty());
			propfield._displayName = label;
		}
	};

	// Internal helper class that is used to set the reference event type to generate
	// for a property field every time its value changes. Do not use this class directly but use the
	// SET_PROPERTY_FIELD_CHANGE_EVENT macro instead.
	struct PropertyFieldChangeEventSetter {
		PropertyFieldChangeEventSetter(NativePropertyFieldDescriptor& propfield, ReferenceEvent::Type eventType) {
			OVITO_ASSERT(propfield._extraChangeEventType == 0);
			propfield._extraChangeEventType = eventType;
		}
	};
};

/*** Macros to define reference and property fields in RefMaker-derived classes: ***/

/// This macros returns a reference to the PropertyFieldDescriptor of a 
/// named reference or property field.
#define PROPERTY_FIELD(RefMakerClassPlusStorageFieldName) \
		RefMakerClassPlusStorageFieldName##__propdescr()

#define DEFINE_REFERENCE_FIELD(classname, fieldname) \
	Ovito::NativePropertyFieldDescriptor classname::fieldname##__propdescr_instance( \
			const_cast<classname::OOMetaClass*>(&classname::OOClass()), \
			&decltype(classname::_##fieldname)::target_type::OOClass(), \
			#fieldname, \
			static_cast<Ovito::PropertyFieldFlags>(classname::__##fieldname##_flags), \
			&classname::fieldname##__access_field \
		);
	
/// Adds a reference field to a class definition.
/// The first parameter specifies the RefTarget derived class of the referenced object.
/// The second parameter determines the name of the reference field. It must be unique within the current class.
#define DECLARE_REFERENCE_FIELD_FLAGS(type, name, flags) \
	private: \
		static Ovito::SingleReferenceFieldBase& name##__access_field(RefMaker* obj) { \
			return static_cast<ovito_class*>(obj)->_##name; \
		} \
		enum { __##name##_flags = flags }; \
		static Ovito::NativePropertyFieldDescriptor name##__propdescr_instance; \
	public: \
		static Ovito::NativePropertyFieldDescriptor& PROPERTY_FIELD(name) { \
			return name##__propdescr_instance; \
		} \
		Ovito::ReferenceField<type> _##name; \
		type* name() const { return _##name; } \
	private:

/// Adds a reference field to a class definition.
/// The first parameter specifies the RefTarget derived class of the referenced object.
/// The second parameter determines the name of the reference field. It must be unique within the current class.
#define DECLARE_REFERENCE_FIELD(type, name) \
	DECLARE_REFERENCE_FIELD_FLAGS(type, name, PROPERTY_FIELD_NO_FLAGS)

/// Adds a settable reference field to a class definition.
/// The first parameter specifies the RefTarget derived class of the referenced object.
/// The second parameter determines the name of the reference field. It must be unique within the current class.
/// The third parameter is the name of the setter method to be created for this reference field.
#define DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(type, name, setterName, flags) \
	public: \
		void setterName(type* obj) { _##name.set(this, PROPERTY_FIELD(name), obj); } \
		DECLARE_REFERENCE_FIELD_FLAGS(type, name, flags)
		
/// Adds a settable reference field to a class definition.
/// The first parameter specifies the RefTarget derived class of the referenced object.
/// The second parameter determines the name of the reference field. It must be unique within the current class.
/// The third parameter is the name of the setter method to be created for this reference field.
#define DECLARE_MODIFIABLE_REFERENCE_FIELD(type, name, setterName) \
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(type, name, setterName, PROPERTY_FIELD_NO_FLAGS)

/// Adds a vector reference field to a class definition.
/// The first parameter specifies the RefTarget-derived class of the objects stored in the vector reference field.
/// The second parameter determines the name of the vector reference field. It must be unique within the current class.
#define DECLARE_VECTOR_REFERENCE_FIELD_FLAGS(type, name, flags) \
	private: \
		static Ovito::VectorReferenceFieldBase& name##__access_field(RefMaker* obj) { \
			return static_cast<ovito_class*>(obj)->_##name; \
		} \
		enum { __##name##_flags = flags | PROPERTY_FIELD_VECTOR }; \
		static Ovito::NativePropertyFieldDescriptor name##__propdescr_instance; \
	public: \
		static Ovito::NativePropertyFieldDescriptor& PROPERTY_FIELD(name) { \
			return name##__propdescr_instance; \
		} \
		Ovito::VectorReferenceField<type> _##name; \
		const QVector<type*>& name() const { return _##name.targets(); } \
	private:

/// Adds a vector reference field to a class definition.
/// The first parameter specifies the RefTarget-derived class of the objects stored in the vector reference field.
/// The second parameter determines the name of the vector reference field. It must be unique within the current class.
#define DECLARE_VECTOR_REFERENCE_FIELD(type, name) \
	DECLARE_VECTOR_REFERENCE_FIELD_FLAGS(type, name, PROPERTY_FIELD_NO_FLAGS)

/// Adds a vector reference field to a class definition that is settable.
/// The first parameter specifies the RefTarget-derived class of the objects stored in the vector reference field.
/// The second parameter determines the name of the vector reference field. It must be unique within the current class.
/// The third parameter is the name of the setter method to be created for this reference field.
#define DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD_FLAGS(type, name, setterName, flags) \
	public: \
		void setterName(const QVector<type*>& lst) { _##name.set(this, PROPERTY_FIELD(name), lst); } \
		DECLARE_VECTOR_REFERENCE_FIELD_FLAGS(type, name, flags)

/// Adds a vector reference field to a class definition that is settable.
/// The first parameter specifies the RefTarget-derived class of the objects stored in the vector reference field.
/// The second parameter determines the name of the vector reference field. It must be unique within the current class.
/// The third parameter is the name of the setter method to be created for this reference field.
#define DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(type, name, setterName) \
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD_FLAGS(type, name, setterName, PROPERTY_FIELD_NO_FLAGS)

/// Assigns a unit class to an animation controller reference or numeric property field.
/// The unit class will automatically be assigned to the numeric input field for this parameter in the user interface. 
#define SET_PROPERTY_FIELD_UNITS(DefiningClass, name, ParameterUnitClass)								\
	static Ovito::NativePropertyFieldDescriptor::PropertyFieldUnitsSetter __unitsSetter##DefiningClass##name(PROPERTY_FIELD(DefiningClass::name), &ParameterUnitClass::staticMetaObject);

/// Assigns a unit class and a minimum value limit to an animation controller reference or numeric property field.
/// The unit class and the value limit will automatically be assigned to the numeric input field for this parameter in the user interface. 
#define SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DefiningClass, name, ParameterUnitClass, minValue)	\
	static Ovito::NativePropertyFieldDescriptor::PropertyFieldUnitsSetter __unitsSetter##DefiningClass##name(PROPERTY_FIELD(DefiningClass::name), &ParameterUnitClass::staticMetaObject, minValue);

/// Assigns a unit class and a minimum and maximum value limit to an animation controller reference or numeric property field.
/// The unit class and the value limits will automatically be assigned to the numeric input field for this parameter in the user interface. 
#define SET_PROPERTY_FIELD_UNITS_AND_RANGE(DefiningClass, name, ParameterUnitClass, minValue, maxValue)	\
	static Ovito::NativePropertyFieldDescriptor::PropertyFieldUnitsSetter __unitsSetter##DefiningClass##name(PROPERTY_FIELD(DefiningClass::name), &ParameterUnitClass::staticMetaObject, minValue, maxValue);

/// Assigns a label string to the given reference or property field.
/// This string will be used in the user interface.
#define SET_PROPERTY_FIELD_LABEL(DefiningClass, name, labelText)										\
	static Ovito::NativePropertyFieldDescriptor::PropertyFieldDisplayNameSetter __displayNameSetter##DefiningClass##name(PROPERTY_FIELD(DefiningClass::name), labelText);

/// Use this macro to let the system automatically generate an event of the
/// given type every time the given property field changes its value. 
#define SET_PROPERTY_FIELD_CHANGE_EVENT(DefiningClass, name, eventType)										\
	static Ovito::NativePropertyFieldDescriptor::PropertyFieldChangeEventSetter __changeEventSetter##DefiningClass##name(PROPERTY_FIELD(DefiningClass::name), eventType);

/// Adds a property field to a class definition.
/// The first parameter specifies the data type of the property field.
/// The second parameter determines the name of the property field. It must be unique within the current class.
#define DECLARE_PROPERTY_FIELD_FLAGS(type, name, flags) \
	private: \
		static QVariant __read_propfield_##name(const RefMaker* obj) { \
			return static_cast<const ovito_class*>(obj)->_##name.getQVariant(); \
		} \
		static void __write_propfield_##name(RefMaker* obj, const QVariant& newValue) { \
			static_cast<ovito_class*>(obj)->_##name.setQVariant(obj, PROPERTY_FIELD(name), newValue); \
		} \
		static void __save_propfield_##name(const RefMaker* obj, SaveStream& stream) { \
			static_cast<const ovito_class*>(obj)->_##name.saveToStream(stream); \
		} \
		static void __load_propfield_##name(RefMaker* obj, LoadStream& stream) { \
			static_cast<ovito_class*>(obj)->_##name.loadFromStream(stream); \
		} \
		static void __copy_propfield_##name(RefMaker* obj, const RefMaker* other) { \
			static_cast<ovito_class*>(obj)->_##name.set(obj, PROPERTY_FIELD(name), static_cast<const ovito_class*>(other)->_##name.get()); \
		} \
		enum { __##name##_flags = flags }; \
		static Ovito::NativePropertyFieldDescriptor name##__propdescr_instance; \
	public: \
		static Ovito::NativePropertyFieldDescriptor& PROPERTY_FIELD(name) { \
			return name##__propdescr_instance; \
		} \
		Ovito::PropertyField<type> _##name; \
		const type& name() const { return _##name; } \
	private:

#define DEFINE_PROPERTY_FIELD(classname, fieldname) \
	Ovito::NativePropertyFieldDescriptor classname::fieldname##__propdescr_instance( \
			const_cast<classname::OOMetaClass*>(&classname::OOClass()), \
			#fieldname, \
			static_cast<Ovito::PropertyFieldFlags>(classname::__##fieldname##_flags), \
			&classname::__copy_propfield_##fieldname, \
			&classname::__read_propfield_##fieldname, \
			&classname::__write_propfield_##fieldname, \
			&classname::__save_propfield_##fieldname, \
			&classname::__load_propfield_##fieldname \
		);
		
/// Adds a property field to a class definition.
/// The first parameter specifies the data type of the property field.
/// The second parameter determines the name of the property field. It must be unique within the current class.
#define DECLARE_PROPERTY_FIELD(type, name) \
	DECLARE_PROPERTY_FIELD_FLAGS(type, name, PROPERTY_FIELD_NO_FLAGS)

/// Adds a settable property field to a class definition.
/// The first parameter specifies the data type of the property field.
/// The second parameter determines the name of the property field. It must be unique within the current class.
/// The third parameter is the name of the setter method to be created for this property field.
#define DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(type, name, setterName, flags) \
	public: \
		void setterName(const type& value) { _##name.set(this, PROPERTY_FIELD(name), value); } \
		DECLARE_PROPERTY_FIELD_FLAGS(type, name, flags)

/// Adds a settable property field to a class definition.
/// The first parameter specifies the data type of the property field.
/// The second parameter determines the name of the property field. It must be unique within the current class.
/// The third parameter is the name of the setter method to be created for this property field.
#define DECLARE_MODIFIABLE_PROPERTY_FIELD(type, name, setterName) \
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(type, name, setterName, PROPERTY_FIELD_NO_FLAGS)
	
/// Adds a property field to a class definition which is not serializble .
/// The first parameter specifies the data type of the property field.
/// The second parameter determines the name of the property field. It must be unique within the current class.
/// The third parameter is the name of the setter method to be created for this property field.
#define DECLARE_RUNTIME_PROPERTY_FIELD_FLAGS(type, name, setterName, flags) \
	private: \
		static QVariant __read_propfield_##name(const RefMaker* obj) { \
			return static_cast<const ovito_class*>(obj)->_##name.getQVariant(); \
		} \
		static void __write_propfield_##name(RefMaker* obj, const QVariant& newValue) { \
			static_cast<ovito_class*>(obj)->_##name.setQVariant(obj, PROPERTY_FIELD(name), newValue); \
		} \
		static void __save_propfield_##name(const RefMaker* obj, SaveStream& stream) {} \
		static void __load_propfield_##name(RefMaker* obj, LoadStream& stream) {} \
		static void __copy_propfield_##name(RefMaker* obj, const RefMaker* other) { \
			static_cast<ovito_class*>(obj)->_##name.set(obj, PROPERTY_FIELD(name), static_cast<const ovito_class*>(other)->_##name.get()); \
		} \
		enum { __##name##_flags = flags }; \
		static Ovito::NativePropertyFieldDescriptor name##__propdescr_instance; \
	public: \
		static Ovito::NativePropertyFieldDescriptor& PROPERTY_FIELD(name) { \
			return name##__propdescr_instance; \
		} \
		Ovito::RuntimePropertyField<type> _##name; \
		const type& name() const { return _##name; } \
		void setterName(const type& value) { _##name.set(this, PROPERTY_FIELD(name), value); } \
	private:

/// Adds a property field to a class definition which is not serializble .
/// The first parameter specifies the data type of the property field.
/// The second parameter determines the name of the property field. It must be unique within the current class.
/// The third parameter is the name of the setter method to be created for this property field.
#define DECLARE_RUNTIME_PROPERTY_FIELD(type, name, setterName) \
	DECLARE_RUNTIME_PROPERTY_FIELD_FLAGS(type, name, setterName, PROPERTY_FIELD_NO_FLAGS)
	
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
