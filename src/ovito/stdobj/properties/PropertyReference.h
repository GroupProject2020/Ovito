////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#pragma once


#include <ovito/stdobj/StdObj.h>

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
	PropertyReference(PropertyContainerClassPtr pclass, int typeId, int vectorComponent = -1);

	/// \brief Constructs a reference to a user-defined property.
	PropertyReference(PropertyContainerClassPtr pclass, const QString& name, int vectorComponent = -1) : _containerClass(pclass), _name(name), _vectorComponent(vectorComponent) {
		OVITO_ASSERT(pclass);
		OVITO_ASSERT(!_name.isEmpty());
	}

	/// \brief Constructs a reference based on an existing PropertyObject.
	PropertyReference(PropertyContainerClassPtr pclass, const PropertyObject* property, int vectorComponent = -1);

	/// \brief Returns the type of property being referenced.
	int type() const { return _type; }

	/// \brief Gets the human-readable name of the referenced property.
	/// \return The property name.
	const QString& name() const { return _name; }

	/// Return the class of the referenced property.
	PropertyContainerClassPtr containerClass() const { return _containerClass; }

	/// Returns the selected component index.
	int vectorComponent() const { return _vectorComponent; }

	/// Selects a component index if the property is a vector property.
	void setVectorComponent(int index) { _vectorComponent = index; }

	/// \brief Compares two references for equality.
	bool operator==(const PropertyReference& other) const {
		if(containerClass() != other.containerClass()) return false;
		if(type() != other.type()) return false;
		if(vectorComponent() != other.vectorComponent()) return false;
		if(type() != 0) return true;
		return name() == other.name();
	}

	/// \brief Compares two references for inequality.
	bool operator!=(const PropertyReference& other) const { return !(*this == other); }

	/// \brief Returns true if this reference does not point to any property.
	/// \return true if this is a default-constructed PropertyReference.
	bool isNull() const { return type() == 0 && name().isEmpty(); }

	/// \brief Returns the display name of the referenced property including the optional vector component.
	QString nameWithComponent() const;

	/// Finds the referenced property in the given property container object.
	const PropertyObject* findInContainer(const PropertyContainer* container) const;

	/// Returns a new property reference that uses the same name as the current one, but with a different property container class.
	PropertyReference convertToContainerClass(PropertyContainerClassPtr containerClass) const;

private:

	/// The class of property container.
	PropertyContainerClassPtr _containerClass = nullptr;

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
 * Encapsulates a reference to a property from a specific container class.
 */
template<class PropertyContainerType>
class TypedPropertyReference : public PropertyReference
{
public:

	/// \brief Default constructor. Creates a null reference.
	TypedPropertyReference() = default;

	/// \brief Conversion copy constructor.
	TypedPropertyReference(const PropertyReference& other) : PropertyReference(other) {}

	/// \brief Conversion move constructor.
	TypedPropertyReference(PropertyReference&& other) : PropertyReference(std::move(other)) {}

	/// \brief Constructs a reference to a standard property.
	TypedPropertyReference(int typeId, int vectorComponent = -1) : PropertyReference(&PropertyContainerType::OOClass(), typeId, vectorComponent) {}

	/// \brief Constructs a reference to a user-defined property.
	TypedPropertyReference(const QString& name, int vectorComponent = -1) : PropertyReference(&PropertyContainerType::OOClass(), name, vectorComponent) {}

	/// \brief Constructs a reference based on an existing PropertyObject.
	TypedPropertyReference(const PropertyObject* property, int vectorComponent = -1) : PropertyReference(&PropertyContainerType::OOClass(), property, vectorComponent) {}

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
