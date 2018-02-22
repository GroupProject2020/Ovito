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

		/// Gives the property class the opportunity to set up a newly created property object.
		virtual void prepareNewProperty(PropertyObject* property) const override;
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
		LengthProperty = PropertyStorage::FirstSpecificProperty,
		TopologyProperty,
		PeriodicImageProperty,
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

/**
 * A helper data structure describing a single bond between two particles.
 */
struct Bond 
{
	/// The index of the first particle.
	/// Note that we are using int instead of size_t here to save some memory.
	size_t index1;

	/// The index of the second particle.
	/// Note that we are using int instead of size_t here to save some memory.
	size_t index2;

	/// If the bond crosses a periodic boundary, this indicates the direction.
	Vector3I pbcShift;

	/// Returns the flipped version of this bond, where the two particles are swapped
	/// and the PBC shift vector is reversed.
	Bond flipped() const { return Bond{ index2, index1, -pbcShift }; }

	/// For a pair of bonds, A<->B and B<->A, determines whether this bond
	/// counts as the 'odd' or the 'even' bond of the pair.
	bool isOdd() const {
		// Is this bond connecting two different particles?
		// If yes, it's easy to determine whether it's an even or an odd bond.
		if(index1 > index2) return true;
		else if(index1 < index2) return false;
		// Whether the bond is 'odd' is determined by the PBC shift vector.
		if(pbcShift[0] != 0) return pbcShift[0] < 0;
		if(pbcShift[1] != 0) return pbcShift[1] < 0;
		// A particle shouldn't be bonded to itself unless the bond crosses a periodic cell boundary:
		OVITO_ASSERT(pbcShift != Vector3I::Zero());
		return pbcShift[2] < 0;
	}
};

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::BondPropertyReference);
