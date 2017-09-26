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


#include <plugins/particles/Particles.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include <plugins/stdobj/properties/PropertyClass.h>
#include <plugins/stdobj/properties/PropertyReference.h>

namespace Ovito { namespace Particles {

/**
 * \brief Stores a bond property.
 */
class OVITO_PARTICLES_EXPORT BondProperty : public PropertyObject
{
public:

	/// Define a new property metaclass for bond properties.
	class OVITO_PARTICLES_EXPORT BondPropertyClass : public PropertyClass 
	{
	public:

		/// Inherit constructor from base class.
		using PropertyClass::PropertyClass;
		
		/// \brief Create a storage object for standard bond properties.
		virtual PropertyPtr createStandardStorage(size_t bondsCount, int type, bool initializeMemory) const override;
				
		/// Returns the number of elements in a property for the given data state.
		virtual size_t elementCount(const PipelineFlowState& state) const override;
		
		/// Determines if the data elements which this property class applies to are present for the given data state.
		virtual bool isDataPresent(const PipelineFlowState& state) const override;
		
	protected:

		/// Is called by the system after construction of the meta-class instance.
		virtual void initialize() override;
	};

	Q_OBJECT
	OVITO_CLASS_META(BondProperty, BondPropertyClass)

public:

	/// \brief The list of standard bond properties.
	enum Type {
		UserProperty = PropertyStorage::GenericUserProperty,	//< This is reserved for user-defined properties.
		SelectionProperty = PropertyStorage::GenericSelectionProperty,
		ColorProperty = PropertyStorage::GenericColorProperty,
		TypeProperty = PropertyStorage::GenericTypeProperty,
		LengthProperty = PropertyStorage::FirstSpecificProperty
	};

public:

	/// \brief Creates a bond property object.
	Q_INVOKABLE BondProperty(DataSet* dataset);

	/// \brief Returns the type of this property.
	Type type() const { return static_cast<Type>(PropertyObject::type()); }

	/// This helper method returns a standard bond property (if present) from the given pipeline state.
	static BondProperty* findInState(const PipelineFlowState& state, BondProperty::Type type) {
		return static_object_cast<BondProperty>(OOClass().findInState(state, type));
	}

	/// This helper method returns a specific user-defined bond property (if present) from the given pipeline state.
	static BondProperty* findInState(const PipelineFlowState& state, const QString& name) {
		return static_object_cast<BondProperty>(OOClass().findInState(state, name));
	}

	/// Create a storage object for standard bond properties.
	static PropertyPtr createStandardStorage(size_t elementCount, BondProperty::Type type, bool initializeMemory) {
		return OOClass().createStandardStorage(elementCount, type, initializeMemory);
	}

	/// Creates a new instace of the property object type.
	static OORef<BondProperty> createFromStorage(DataSet* dataset, const PropertyPtr& storage) {
		return static_object_cast<BondProperty>(OOClass().createFromStorage(dataset, storage));
	}
};

/**
 * Encapsulates a reference to a bond property. 
 */
using BondPropertyReference = TypedPropertyReference<BondProperty>;


}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::BondPropertyReference);
