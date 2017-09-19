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
#include <core/dataset/data/properties/PropertyObject.h>
#include <core/dataset/data/properties/PropertyClass.h>
#include <core/dataset/data/properties/PropertyReference.h>

namespace Ovito { namespace Particles {

/**
 * \brief Stores a particle property.
 */
class OVITO_PARTICLES_EXPORT ParticleProperty : public PropertyObject
{
public:

	/// Define a new property metaclass for particle properties.
	class OVITO_PARTICLES_EXPORT ParticlePropertyClass : public PropertyClass 
	{
	public:

		/// Inherit constructor from base class.
		using PropertyClass::PropertyClass;
		
		/// \brief Create a storage object for standard particle properties.
		virtual PropertyPtr createStandardStorage(size_t elementCount, int type, bool initializeMemory) const override;
				
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
	OVITO_CLASS_META(ParticleProperty, ParticlePropertyClass)

public:

	/// \brief The list of standard particle properties.
	enum Type {
		UserProperty = PropertyStorage::GenericUserProperty,	//< This is reserved for user-defined properties.
		SelectionProperty = PropertyStorage::GenericSelectionProperty,
		ColorProperty = PropertyStorage::GenericColorProperty,
		TypeProperty = PropertyStorage::GenericTypeProperty,
		PositionProperty = PropertyStorage::FirstSpecificProperty,
		DisplacementProperty,
		DisplacementMagnitudeProperty,
		PotentialEnergyProperty,
		KineticEnergyProperty,
		TotalEnergyProperty,
		VelocityProperty,
		RadiusProperty,
		ClusterProperty,
		CoordinationProperty,
		StructureTypeProperty,
		IdentifierProperty,
		StressTensorProperty,
		StrainTensorProperty,
		DeformationGradientProperty,
		OrientationProperty,
		ForceProperty,
		MassProperty,
		ChargeProperty,
		PeriodicImageProperty,
		TransparencyProperty,
		DipoleOrientationProperty,
		DipoleMagnitudeProperty,
		AngularVelocityProperty,
		AngularMomentumProperty,
		TorqueProperty,
		SpinProperty,
		CentroSymmetryProperty,
		VelocityMagnitudeProperty,
		MoleculeProperty,
		AsphericalShapeProperty,
		VectorColorProperty,
		ElasticStrainTensorProperty,
		ElasticDeformationGradientProperty,
		RotationProperty,
		StretchTensorProperty,
		MoleculeTypeProperty
	};

public:

	/// \brief Creates a particle property object.
	Q_INVOKABLE ParticleProperty(DataSet* dataset);

	/// \brief Returns the type of this property.
	Type type() const { return static_cast<Type>(PropertyObject::type()); }

	/// This helper method returns a standard particle property (if present) from the given pipeline state.
	static ParticleProperty* findInState(const PipelineFlowState& state, ParticleProperty::Type type) {
		return static_object_cast<ParticleProperty>(OOClass().findInState(state, type));
	}

	/// This helper method returns a specific user-defined particle property (if present) from the given pipeline state.
	static ParticleProperty* findInState(const PipelineFlowState& state, const QString& name) {
		return static_object_cast<ParticleProperty>(OOClass().findInState(state, name));
	}

	/// Create a storage object for standard particle properties.
	static PropertyPtr createStandardStorage(size_t elementCount, ParticleProperty::Type type, bool initializeMemory) {
		return OOClass().createStandardStorage(elementCount, type, initializeMemory);
	}

	/// Creates a new instace of the property object type.
	static OORef<ParticleProperty> createFromStorage(DataSet* dataset, const PropertyPtr& storage) {
		return static_object_cast<ParticleProperty>(OOClass().createFromStorage(dataset, storage));
	}
};


/**
 * Encapsulates a reference to a particle property. 
 */
using ParticlePropertyReference = TypedPropertyReference<ParticleProperty>;

}	// End of namespace
}	// End of namespace

