////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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


#include <ovito/particles/Particles.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include "BondsObject.h"

namespace Ovito { namespace Particles {

/**
 * \brief This data object type is a container for particle properties.
 */
class OVITO_PARTICLES_EXPORT ParticlesObject : public PropertyContainer
{
	/// Define a new property metaclass for particle containers.
	class ParticlesObjectClass : public PropertyContainerClass
	{
	public:
		/// Inherit constructor from base class.
		using PropertyContainerClass::PropertyContainerClass;

		/// \brief Create a storage object for standard particle properties.
		virtual PropertyPtr createStandardStorage(size_t elementCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath = {}) const override;

		/// Indicates whether this kind of property container supports picking of individual elements in the viewports.
		virtual bool supportsViewportPicking() const override { return true; }

		/// Returns the index of the element that was picked in a viewport.
		virtual std::pair<size_t, ConstDataObjectPath> elementFromPickResult(const ViewportPickResult& pickResult) const override;

		/// Tries to remap an index from one property container to another, considering the possibility that
		/// elements may have been added or removed.
		virtual size_t remapElementIndex(const ConstDataObjectPath& source, size_t elementIndex, const ConstDataObjectPath& dest) const override;

		/// Determines which elements are located within the given viewport fence region (=2D polygon).
		virtual boost::dynamic_bitset<> viewportFenceSelection(const QVector<Point2>& fence, const ConstDataObjectPath& objectPath, PipelineSceneNode* node, const Matrix4& projectionTM) const override;

	protected:

		/// Is called by the system after construction of the meta-class instance.
		virtual void initialize() override;

		/// Gives the property class the opportunity to set up a newly created property object.
		virtual void prepareNewProperty(PropertyObject* property) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(ParticlesObject, ParticlesObjectClass);
	Q_CLASSINFO("DisplayName", "Particles");

public:

	/// \brief The list of standard particle properties.
	enum Type {
		UserProperty = PropertyStorage::GenericUserProperty,	//< This is reserved for user-defined properties.
		SelectionProperty = PropertyStorage::GenericSelectionProperty,
		ColorProperty = PropertyStorage::GenericColorProperty,
		TypeProperty = PropertyStorage::GenericTypeProperty,
		IdentifierProperty = PropertyStorage::GenericIdentifierProperty,
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
		MoleculeTypeProperty,
		StrandProperty
	};

	/// \brief Constructor.
	Q_INVOKABLE ParticlesObject(DataSet* dataset);

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("Particles"); }

	/// Deletes the particles for which bits are set in the given bit-mask.
	/// Returns the number of deleted particles.
	virtual size_t deleteElements(const boost::dynamic_bitset<>& mask) override;

	/// Duplicates the BondsObject if it is shared with other particle objects.
	/// After this method returns, the BondsObject is exclusively owned by the ParticlesObject and
	/// can be safely modified without expected side effects.
	BondsObject* makeBondsMutable();

	/// Convinience method that makes sure that there is a BondsObject.
	const BondsObject* expectBonds() const;

	/// Convinience method that makes sure that there is a BondsObject and the bond topology property.
	const PropertyObject* expectBondsTopology() const;

	/// Adds a set of new bonds to the particle system.
	void addBonds(const std::vector<Bond>& newBonds, BondsVis* bondsVis, const std::vector<PropertyPtr>& bondProperties = {}, const BondType* bondType = nullptr);

	/// Returns a vector with the input particle colors.
	std::vector<ColorA> inputParticleColors() const;

	/// Returns a vector with the input particle radii.
	std::vector<FloatType> inputParticleRadii() const;

	/// Returns a vector with the input bond colors.
	std::vector<ColorA> inputBondColors(bool ignoreExistingColorProperty = false) const;

private:

	/// The bonds object.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(BondsObject, bonds, setBonds);
};


/**
 * Encapsulates a reference to a particle property.
 */
using ParticlePropertyReference = TypedPropertyReference<ParticlesObject>;

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::ParticlePropertyReference);
