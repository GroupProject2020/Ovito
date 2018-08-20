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


#include <plugins/stdobj/StdObj.h>

namespace Ovito { namespace StdObj {

/**
 * \brief A generic reference to a property.
 */
class OVITO_STDOBJ_EXPORT PropertyReference
{
public:

	/// \brief Default constructor. Creates a null reference.
	PropertyReference() = default;
	
	/// \brief Constructs a reference to a standard property.
	PropertyReference(PropertyClassPtr pclass, int typeId, int vectorComponent = -1, const QString& bundle = QString());

	/// \brief Constructs a reference to a user-defined property.
	PropertyReference(PropertyClassPtr pclass, const QString& name, int vectorComponent = -1, const QString& bundle = QString()) : _propertyClass(pclass), _bundle(bundle), _name(name), _vectorComponent(vectorComponent) { 
		OVITO_ASSERT(pclass); 
		OVITO_ASSERT(!_name.isEmpty()); 
	}
	
	/// \brief Constructs a reference based on an existing PropertyObject.
	PropertyReference(PropertyObject* property, int vectorComponent = -1);

	/// \brief Returns the type of property being referenced.
	int type() const { return _type; }

	/// \brief Gets the human-readable name of the referenced property.
	/// \return The property name.
	const QString& name() const { return _name; }

	/// \brief Gets the identifier of the bundle the property belongs to.
	/// \return The identifier of the property bundle.
	const QString& bundle() const { return _bundle; }

	/// Return the class of the referenced property.
	PropertyClassPtr propertyClass() const { return _propertyClass; }
	
	/// Returns the selected component index.
	int vectorComponent() const { return _vectorComponent; }

	/// Selects a component index if the property is a vector property.
	void setVectorComponent(int index) { _vectorComponent = index; }

	/// \brief Compares two references for equality.
	bool operator==(const PropertyReference& other) const {
		if(propertyClass() != other.propertyClass()) return false;
		if(bundle() != other.bundle()) return false;
		if(type() != other.type()) return false;
		if(vectorComponent() != other.vectorComponent()) return false;
		if(type() != 0) return true;
		return name() == other.name();
	}

	/// \brief Compares two references for inequality.
	bool operator!=(const PropertyReference& other) const { return !(*this == other); }

	/// \brief Returns true if this reference does not point to any particle property.
	/// \return true if this is a default constructed ParticlePropertyReference.
	bool isNull() const { return type() == 0 && name().isEmpty(); }

	/// \brief Returns the display name of the referenced property including the optional vector component.
	QString nameWithComponent() const;

	/// Finds the referenced property in the given pipeline state.
	PropertyObject* findInState(const PipelineFlowState& state) const;

	/// Returns a new property reference that uses the same name as the current one, but with a different property class.
	PropertyReference convertToPropertyClass(PropertyClassPtr pclass) const;
	
private:

	/// The class of property.
	PropertyClassPtr _propertyClass = nullptr;

	/// The identifier of the property bundle (optional).
	QString _bundle;

	/// The type of the property.
	int _type = 0;

	/// The human-readable name of the property.
	QString _name;

	/// The zero-based component index if the property is a vector property (or zero if not a vector property).
	int _vectorComponent = -1;

	friend OVITO_STDOBJ_EXPORT SaveStream& operator<<(SaveStream& stream, const PropertyReference& r);
	friend OVITO_STDOBJ_EXPORT LoadStream& operator>>(LoadStream& stream, PropertyReference& r);
};

/// Writes a PropertyReference to an output stream.
/// \relates PropertyReference
extern OVITO_STDOBJ_EXPORT SaveStream& operator<<(SaveStream& stream, const PropertyReference& r);

/// Reads a PropertyReference from an input stream.
/// \relates PropertyReference
extern OVITO_STDOBJ_EXPORT LoadStream& operator>>(LoadStream& stream, PropertyReference& r);

/**
 * Encapsulates a reference to a property from a specific class. 
 */
template<class PropertyObjectType>
class TypedPropertyReference : public PropertyReference
{
public:

	/// \brief Default constructor. Creates a null reference.
	TypedPropertyReference() = default;

	/// \brief Conversion copy  constructor. 
	TypedPropertyReference(const PropertyReference& other) : PropertyReference(other) {}

	/// \brief Conversion move constructor. 
	TypedPropertyReference(PropertyReference&& other) : PropertyReference(std::move(other)) {}
		
	/// \brief Constructs a reference to a standard property.
	TypedPropertyReference(int typeId, int vectorComponent = -1, const QString& bundle = QString()) : PropertyReference(&PropertyObjectType::OOClass(), typeId, vectorComponent, bundle) {}

	/// \brief Constructs a reference to a user-defined property.
	TypedPropertyReference(const QString& name, int vectorComponent = -1, const QString& bundle = QString()) : PropertyReference(&PropertyObjectType::OOClass(), name, vectorComponent, bundle) {}
	
	/// \brief Constructs a reference based on an existing PropertyObject.
	TypedPropertyReference(PropertyObjectType* property, int vectorComponent = -1) : PropertyReference(property, vectorComponent) {
		OVITO_ASSERT(property->getOOClass().isDerivedFrom(PropertyObjectType::OOClass()));
	}

	/// Finds the referenced property in the given pipeline state.
	PropertyObjectType* findInState(const PipelineFlowState& state) const {
		return static_object_cast<PropertyObjectType>(PropertyReference::findInState(state));
	}

	friend SaveStream& operator<<(SaveStream& stream, const TypedPropertyReference& r) {
		return stream << static_cast<const PropertyReference&>(r);
	}

	friend LoadStream& operator>>(LoadStream& stream, TypedPropertyReference& r) {
		return stream >> static_cast<PropertyReference&>(r);
	}
};

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::StdObj::PropertyReference);
