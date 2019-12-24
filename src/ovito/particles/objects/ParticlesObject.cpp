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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesVis.h>
#include <ovito/particles/objects/BondType.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include "ParticlesObject.h"
#include "ParticlesVis.h"
#include "BondsVis.h"
#include "VectorVis.h"
#include "ParticleBondMap.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(ParticlesObject);
DEFINE_REFERENCE_FIELD(ParticlesObject, bonds);
SET_PROPERTY_FIELD_LABEL(ParticlesObject, bonds, "Bonds");

/******************************************************************************
* Constructor.
******************************************************************************/
ParticlesObject::ParticlesObject(DataSet* dataset) : PropertyContainer(dataset)
{
	// Attach a visualization element for rendering the particles.
	addVisElement(new ParticlesVis(dataset));
}

/******************************************************************************
* Duplicates the BondsObject if it is shared with other particle objects.
* After this method returns, the BondsObject is exclusively owned by the
* container and can be safely modified without unwanted side effects.
******************************************************************************/
BondsObject* ParticlesObject::makeBondsMutable()
{
    OVITO_ASSERT(bonds());
    return makeMutable(bonds());
}

/******************************************************************************
* Convinience method that makes sure that there is a BondsObject.
* Throws an exception if there isn't.
******************************************************************************/
const BondsObject* ParticlesObject::expectBonds() const
{
    if(!bonds())
		throwException(tr("There are no bonds."));
	return bonds();
}

/******************************************************************************
* Convinience method that makes sure that there is a BondsObject and the
* bond topology property. Throws an exception if there isn't.
******************************************************************************/
const PropertyObject* ParticlesObject::expectBondsTopology() const
{
    return expectBonds()->expectProperty(BondsObject::TopologyProperty);
}

/******************************************************************************
* Deletes the particles for which bits are set in the given bit-mask.
* Returns the number of deleted particles.
******************************************************************************/
size_t ParticlesObject::deleteElements(const boost::dynamic_bitset<>& mask)
{
	OVITO_ASSERT(mask.size() == elementCount());

	size_t deleteCount = mask.count();
	size_t oldParticleCount = elementCount();
	size_t newParticleCount = oldParticleCount - deleteCount;
	if(deleteCount == 0)
		return 0;	// Nothing to delete.

	// Delete the particles.
	PropertyContainer::deleteElements(mask);

	// Delete dangling bonds, i.e. those that are incident on deleted particles.
    if(bonds()) {
        // Make sure we can safely modify the bonds object.
        BondsObject* mutableBonds = makeBondsMutable();

        size_t newBondCount = 0;
        size_t oldBondCount = mutableBonds->elementCount();
        boost::dynamic_bitset<> deletedBondsMask(oldBondCount);

        // Build map from old particle indices to new indices.
        std::vector<size_t> indexMap(oldParticleCount);
        auto index = indexMap.begin();
        size_t count = 0;
        for(size_t i = 0; i < oldParticleCount; i++)
            *index++ = mask.test(i) ? std::numeric_limits<size_t>::max() : count++;

        // Remap particle indices of stored bonds and remove dangling bonds.
        if(const PropertyObject* topologyProperty = mutableBonds->getTopology()) {
            PropertyPtr mutableTopology = mutableBonds->makeMutable(topologyProperty)->modifiableStorage();
			for(size_t bondIndex = 0; bondIndex < oldBondCount; bondIndex++) {
                size_t index1 = mutableTopology->get<qlonglong>(bondIndex, 0);
                size_t index2 = mutableTopology->get<qlonglong>(bondIndex, 1);

                // Remove invalid bonds, i.e. whose particle indices are out of bounds.
                if(index1 >= oldParticleCount || index2 >= oldParticleCount) {
                    deletedBondsMask.set(bondIndex);
                    continue;
                }

                // Remove dangling bonds whose particles have gone.
                if(mask.test(index1) || mask.test(index2)) {
                    deletedBondsMask.set(bondIndex);
                    continue;
                }

                // Keep bond and remap particle indices.
                mutableTopology->set<qlonglong>(bondIndex, 0, indexMap[index1]);
                mutableTopology->set<qlonglong>(bondIndex, 1, indexMap[index2]);
            }

            // Delete the marked bonds.
            mutableBonds->deleteElements(deletedBondsMask);
        }
    }

	return deleteCount;
}

/******************************************************************************
* Adds a set of new bonds to the particle system.
******************************************************************************/
void ParticlesObject::addBonds(const std::vector<Bond>& newBonds, BondsVis* bondsVis, const std::vector<PropertyPtr>& bondProperties, const BondType* bondType)
{
	// Check if there are existing bonds.
	if(!bonds() || !bonds()->getProperty(BondsObject::TopologyProperty)) {
		// Create the bonds object.
		setBonds(new BondsObject(dataset()));

		// Create essential bond properties.
		PropertyPtr topologyProperty = BondsObject::OOClass().createStandardStorage(newBonds.size(), BondsObject::TopologyProperty, false);
		PropertyPtr periodicImageProperty = BondsObject::OOClass().createStandardStorage(newBonds.size(), BondsObject::PeriodicImageProperty, false);
		PropertyPtr bondTypeProperty = bondType ? BondsObject::OOClass().createStandardStorage(newBonds.size(), BondsObject::TypeProperty, false) : nullptr;

		// Copy data into property arrays.
		auto t = topologyProperty->data<qlonglong>(0,0);
		auto pbc = periodicImageProperty->data<Vector3I>();
		for(const Bond& bond : newBonds) {
			OVITO_ASSERT(bond.index1 < elementCount());
			OVITO_ASSERT(bond.index2 < elementCount());
			*t++ = bond.index1;
			*t++ = bond.index2;
			*pbc++ = bond.pbcShift;
		}

		// Insert property objects into the output pipeline state.
		PropertyObject* topologyPropertyObj = bonds()->createProperty(topologyProperty);
		PropertyObject* periodicImagePropertyObj = bonds()->createProperty(periodicImageProperty);
		if(bondTypeProperty) {
			boost::fill(bondTypeProperty->range<int>(), bondType->numericId());
			PropertyObject* bondTypePropertyObj = bonds()->createProperty(bondTypeProperty);
			bondTypePropertyObj->addElementType(bondType);
		}

		// Insert other bond properties.
		for(const auto& bprop : bondProperties) {
			OVITO_ASSERT(bprop->size() == newBonds.size());
			OVITO_ASSERT(bprop->type() != BondsObject::TopologyProperty);
			OVITO_ASSERT(bprop->type() != BondsObject::PeriodicImageProperty);
			OVITO_ASSERT(!bondTypeProperty || bprop->type() != BondsObject::TypeProperty);
			bonds()->createProperty(bprop);
		}
	}
	else {
		BondsObject* bonds = makeBondsMutable();

		// This is needed to determine which bonds already exist.
		ParticleBondMap bondMap(*bonds);

		// Check which bonds are new and need to be merged.
		size_t originalBondCount = bonds->elementCount();
		size_t outputBondCount = originalBondCount;
		std::vector<size_t> mapping(newBonds.size());
		for(size_t bondIndex = 0; bondIndex < newBonds.size(); bondIndex++) {
			// Check if there is already a bond like this.
			const Bond& bond = newBonds[bondIndex];
			auto existingBondIndex = bondMap.findBond(bond);
			if(existingBondIndex == originalBondCount) {
				// It's a new bond.
				mapping[bondIndex] = outputBondCount;
				outputBondCount++;
			}
			else {
				// It's an already existing bond.
				mapping[bondIndex] = existingBondIndex;
			}
		}

		// Resize the existing property arrays.
		bonds->setElementCount(outputBondCount);

		PropertyObject* newBondsTopology = bonds->expectMutableProperty(BondsObject::TopologyProperty);
		PropertyObject* newBondsPeriodicImages = bonds->createProperty(BondsObject::PeriodicImageProperty, true);
		PropertyObject* newBondTypeProperty = bondType ? bonds->createProperty(BondsObject::TypeProperty, true) : nullptr;

		if(newBondTypeProperty) {
			newBondTypeProperty->addElementType(bondType);
		}

		// Copy bonds information into the extended arrays.
		for(size_t bondIndex = 0; bondIndex < newBonds.size(); bondIndex++) {
			if(mapping[bondIndex] >= originalBondCount) {
				const Bond& bond = newBonds[bondIndex];
				OVITO_ASSERT(bond.index1 < elementCount());
				OVITO_ASSERT(bond.index2 < elementCount());
				newBondsTopology->set<qlonglong>(mapping[bondIndex], 0, bond.index1);
				newBondsTopology->set<qlonglong>(mapping[bondIndex], 1, bond.index2);
				newBondsPeriodicImages->set<Vector3I>(mapping[bondIndex], bond.pbcShift);
				if(newBondTypeProperty) newBondTypeProperty->set<int>(mapping[bondIndex], bondType->numericId());
			}
		}

		// Initialize property values of new bonds.
		for(PropertyObject* bondPropertyObject : bonds->properties()) {
			if(bondPropertyObject->type() == BondsObject::ColorProperty) {
				const std::vector<ColorA>& colors = inputBondColors(true);
				OVITO_ASSERT(colors.size() == bondPropertyObject->size());
				std::transform(colors.cbegin() + originalBondCount, colors.cend(), bondPropertyObject->data<Color>(originalBondCount), 
					[](const ColorA& c) { return Color(c.r(), c.g(), c.b()); });
			}
		}

		// Merge new bond properties.
		for(const auto& bprop : bondProperties) {
			OVITO_ASSERT(bprop->size() == newBonds.size());
			OVITO_ASSERT(bprop->type() != BondsObject::TopologyProperty);
			OVITO_ASSERT(bprop->type() != BondsObject::PeriodicImageProperty);
			OVITO_ASSERT(!bondType || bprop->type() != BondsObject::TypeProperty);

			OORef<PropertyObject> propertyObject;

			if(bprop->type() != BondsObject::UserProperty) {
				propertyObject = bonds->createProperty(bprop->type(), true);
			}
			else {
				propertyObject = bonds->createProperty(bprop->name(), bprop->dataType(), bprop->componentCount(), bprop->stride(), true);
			}

			// Copy bond property data.
			propertyObject->modifiableStorage()->mappedCopy(*bprop, mapping);
		}
	}

	if(bondsVis)
		bonds()->setVisElement(bondsVis);
}

/******************************************************************************
* Returns a vector with the input particle colors.
******************************************************************************/
std::vector<ColorA> ParticlesObject::inputParticleColors() const
{
	std::vector<ColorA> colors(elementCount());

	// Obtain the particle vis element.
	if(ParticlesVis* particleVis = visElement<ParticlesVis>()) {

		// Query particle colors from vis element.
		particleVis->particleColors(colors,
				getProperty(ParticlesObject::ColorProperty),
				getProperty(ParticlesObject::TypeProperty),
				nullptr,
				getProperty(ParticlesObject::TransparencyProperty));

		return colors;
	}

	std::fill(colors.begin(), colors.end(), ColorA(1,1,1,1));
	return colors;
}

/******************************************************************************
* Returns a vector with the input bond colors.
******************************************************************************/
std::vector<ColorA> ParticlesObject::inputBondColors(bool ignoreExistingColorProperty) const
{
	// Obtain the bonds vis element.
    if(bonds()) {
		if(BondsVis* bondsVis = bonds()->visElement<BondsVis>()) {

			// Additionally, look up the particles vis element.
			ParticlesVis* particleVis = visElement<ParticlesVis>();

			// Query half-bond colors from vis element.
			std::vector<ColorA> halfBondColors = bondsVis->halfBondColors(
					elementCount(),
					bonds()->getProperty(BondsObject::TopologyProperty),
					!ignoreExistingColorProperty ? bonds()->getProperty(BondsObject::ColorProperty) : nullptr,
					bonds()->getProperty(BondsObject::TypeProperty),
					nullptr, // No selection highlighting needed here
					nullptr, // No transparency needed here
					particleVis,
					getProperty(ParticlesObject::ColorProperty),
					getProperty(ParticlesObject::TypeProperty));
			OVITO_ASSERT(bonds()->elementCount() * 2 == halfBondColors.size());

			// Map half-bond colors to full bond colors.
			std::vector<ColorA> colors(bonds()->elementCount());
			auto ci = halfBondColors.cbegin();
			for(ColorA& co : colors) {
				co = ColorA(ci->r(), ci->g(), ci->b(), 1);
				ci += 2;
			}
			return colors;
		}
    	return std::vector<ColorA>(bonds()->elementCount(), ColorA(1,1,1,1));
    }
	return {};
}

/******************************************************************************
* Returns a vector with the input particle radii.
******************************************************************************/
std::vector<FloatType> ParticlesObject::inputParticleRadii() const
{
	std::vector<FloatType> radii(elementCount());

	// Obtain the particle vis element.
	if(ParticlesVis* particleVis = visElement<ParticlesVis>()) {

		// Query particle radii from vis element.
		particleVis->particleRadii(radii,
				getProperty(ParticlesObject::RadiusProperty),
				getProperty(ParticlesObject::TypeProperty));

		return radii;
	}

	std::fill(radii.begin(), radii.end(), FloatType(1));
	return radii;
}


/******************************************************************************
* Gives the property class the opportunity to set up a newly created
* property object.
******************************************************************************/
void ParticlesObject::OOMetaClass::prepareNewProperty(PropertyObject* property) const
{
	if(property->type() == ParticlesObject::DisplacementProperty) {
		OORef<VectorVis> vis = new VectorVis(property->dataset());
		vis->setObjectTitle(tr("Displacements"));
		if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
			vis->loadUserDefaults();
		vis->setEnabled(false);
		property->addVisElement(vis);
	}
	else if(property->type() == ParticlesObject::ForceProperty) {
		OORef<VectorVis> vis = new VectorVis(property->dataset());
		vis->setObjectTitle(tr("Forces"));
		if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
			vis->loadUserDefaults();
		vis->setEnabled(false);
		vis->setReverseArrowDirection(false);
		vis->setArrowPosition(VectorVis::Base);
		property->addVisElement(vis);
	}
	else if(property->type() == ParticlesObject::DipoleOrientationProperty) {
		OORef<VectorVis> vis = new VectorVis(property->dataset());
		vis->setObjectTitle(tr("Dipoles"));
		if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
			vis->loadUserDefaults();
		vis->setEnabled(false);
		vis->setReverseArrowDirection(false);
		vis->setArrowPosition(VectorVis::Center);
		property->addVisElement(vis);
	}
}

/******************************************************************************
* Creates a storage object for standard particle properties.
******************************************************************************/
PropertyPtr ParticlesObject::OOMetaClass::createStandardStorage(size_t particleCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath) const
{
	int dataType;
	size_t componentCount;
	size_t stride;

	switch(type) {
	case TypeProperty:
	case StructureTypeProperty:
	case SelectionProperty:
	case CoordinationProperty:
	case MoleculeTypeProperty:
		dataType = PropertyStorage::Int;
		componentCount = 1;
		stride = sizeof(int);
		break;
	case IdentifierProperty:
	case ClusterProperty:
	case MoleculeProperty:
		dataType = PropertyStorage::Int64;
		componentCount = 1;
		stride = sizeof(qlonglong);
		break;
	case PositionProperty:
	case DisplacementProperty:
	case VelocityProperty:
	case ForceProperty:
	case DipoleOrientationProperty:
	case AngularVelocityProperty:
	case AngularMomentumProperty:
	case TorqueProperty:
	case AsphericalShapeProperty:
		dataType = PropertyStorage::Float;
		componentCount = 3;
		stride = sizeof(Vector3);
		OVITO_ASSERT(stride == sizeof(Point3));
		break;
	case ColorProperty:
	case VectorColorProperty:
		dataType = PropertyStorage::Float;
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Color));
		break;
	case PotentialEnergyProperty:
	case KineticEnergyProperty:
	case TotalEnergyProperty:
	case RadiusProperty:
	case MassProperty:
	case ChargeProperty:
	case TransparencyProperty:
	case SpinProperty:
	case DipoleMagnitudeProperty:
	case CentroSymmetryProperty:
	case DisplacementMagnitudeProperty:
	case VelocityMagnitudeProperty:
		dataType = PropertyStorage::Float;
		componentCount = 1;
		stride = sizeof(FloatType);
		break;
	case StressTensorProperty:
	case StrainTensorProperty:
	case ElasticStrainTensorProperty:
	case StretchTensorProperty:
		dataType = PropertyStorage::Float;
		componentCount = 6;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(SymmetricTensor2));
		break;
	case DeformationGradientProperty:
	case ElasticDeformationGradientProperty:
		dataType = PropertyStorage::Float;
		componentCount = 9;
		stride = componentCount * sizeof(FloatType);
		break;
	case OrientationProperty:
	case RotationProperty:
		dataType = PropertyStorage::Float;
		componentCount = 4;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Quaternion));
		break;
	case PeriodicImageProperty:
		dataType = PropertyStorage::Int;
		componentCount = 3;
		stride = componentCount * sizeof(int);
		break;
	default:
		OVITO_ASSERT_MSG(false, "ParticlesObject::createStandardStorage()", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard property type: %1").arg(type));
	}

	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));

	// Allocate the storage array.
	PropertyPtr property = std::make_shared<PropertyStorage>(particleCount, dataType, componentCount, stride,
								propertyName, false, type, componentNames);

	// Initialize memory if requested.
	if(initializeMemory && !containerPath.empty()) {
		// Certain standard properties need to be initialized with default values determined by the attached visual elements.
		if(type == ColorProperty) {
			if(const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(containerPath.back())) {
				const std::vector<ColorA>& colors = particles->inputParticleColors();
				OVITO_ASSERT(colors.size() == property->size());
				boost::transform(colors, property->data<Color>(), [](const ColorA& c) { return Color(c.r(), c.g(), c.b()); });
				initializeMemory = false;
			}
		}
		else if(type == RadiusProperty) {
			if(const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(containerPath.back())) {
				const std::vector<FloatType>& radii = particles->inputParticleRadii();
				OVITO_ASSERT(radii.size() == property->size());
				boost::copy(radii, property->data<FloatType>());
				initializeMemory = false;
			}
		}
		else if(type == VectorColorProperty) {
			if(const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(containerPath.back())) {
				for(const PropertyObject* p : particles->properties()) {
					if(VectorVis* vectorVis = dynamic_object_cast<VectorVis>(p->visElement())) {
						property->fill(vectorVis->arrowColor());
						initializeMemory = false;
						break;
					}
				}
			}
		}
	}

	if(initializeMemory) {
		// Default-initialize property values with zeros.
		std::memset(property->data<void>(), 0, property->size() * property->stride());
	}

	return property;
}

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void ParticlesObject::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	// Enable automatic conversion of a ParticlePropertyReference to a generic PropertyReference and vice versa.
	QMetaType::registerConverter<ParticlePropertyReference, PropertyReference>();
	QMetaType::registerConverter<PropertyReference, ParticlePropertyReference>();

	setPropertyClassDisplayName(tr("Particles"));
	setElementDescriptionName(QStringLiteral("particles"));
	setPythonName(QStringLiteral("particles"));

	const QStringList emptyList;
	const QStringList xyzList = QStringList() << "X" << "Y" << "Z";
	const QStringList rgbList = QStringList() << "R" << "G" << "B";
	const QStringList symmetricTensorList = QStringList() << "XX" << "YY" << "ZZ" << "XY" << "XZ" << "YZ";
	const QStringList tensorList = QStringList() << "XX" << "YX" << "ZX" << "XY" << "YY" << "ZY" << "XZ" << "YZ" << "ZZ";
	const QStringList quaternionList = QStringList() << "X" << "Y" << "Z" << "W";

	registerStandardProperty(TypeProperty, tr("Particle Type"), PropertyStorage::Int, emptyList, tr("Particle types"));
	registerStandardProperty(SelectionProperty, tr("Selection"), PropertyStorage::Int, emptyList);
	registerStandardProperty(ClusterProperty, tr("Cluster"), PropertyStorage::Int64, emptyList);
	registerStandardProperty(CoordinationProperty, tr("Coordination"), PropertyStorage::Int, emptyList);
	registerStandardProperty(PositionProperty, tr("Position"), PropertyStorage::Float, xyzList, tr("Particle positions"));
	registerStandardProperty(ColorProperty, tr("Color"), PropertyStorage::Float, rgbList, tr("Particle colors"));
	registerStandardProperty(DisplacementProperty, tr("Displacement"), PropertyStorage::Float, xyzList, tr("Displacements"));
	registerStandardProperty(DisplacementMagnitudeProperty, tr("Displacement Magnitude"), PropertyStorage::Float, emptyList);
	registerStandardProperty(VelocityProperty, tr("Velocity"), PropertyStorage::Float, xyzList, tr("Velocities"));
	registerStandardProperty(PotentialEnergyProperty, tr("Potential Energy"), PropertyStorage::Float, emptyList);
	registerStandardProperty(KineticEnergyProperty, tr("Kinetic Energy"), PropertyStorage::Float, emptyList);
	registerStandardProperty(TotalEnergyProperty, tr("Total Energy"), PropertyStorage::Float, emptyList);
	registerStandardProperty(RadiusProperty, tr("Radius"), PropertyStorage::Float, emptyList, tr("Radii"));
	registerStandardProperty(StructureTypeProperty, tr("Structure Type"), PropertyStorage::Int, emptyList, tr("Structure types"));
	registerStandardProperty(IdentifierProperty, tr("Particle Identifier"), PropertyStorage::Int64, emptyList, tr("Particle identifiers"));
	registerStandardProperty(StressTensorProperty, tr("Stress Tensor"), PropertyStorage::Float, symmetricTensorList);
	registerStandardProperty(StrainTensorProperty, tr("Strain Tensor"), PropertyStorage::Float, symmetricTensorList);
	registerStandardProperty(DeformationGradientProperty, tr("Deformation Gradient"), PropertyStorage::Float, tensorList);
	registerStandardProperty(OrientationProperty, tr("Orientation"), PropertyStorage::Float, quaternionList);
	registerStandardProperty(ForceProperty, tr("Force"), PropertyStorage::Float, xyzList);
	registerStandardProperty(MassProperty, tr("Mass"), PropertyStorage::Float, emptyList);
	registerStandardProperty(ChargeProperty, tr("Charge"), PropertyStorage::Float, emptyList);
	registerStandardProperty(PeriodicImageProperty, tr("Periodic Image"), PropertyStorage::Int, xyzList);
	registerStandardProperty(TransparencyProperty, tr("Transparency"), PropertyStorage::Float, emptyList);
	registerStandardProperty(DipoleOrientationProperty, tr("Dipole Orientation"), PropertyStorage::Float, xyzList);
	registerStandardProperty(DipoleMagnitudeProperty, tr("Dipole Magnitude"), PropertyStorage::Float, emptyList);
	registerStandardProperty(AngularVelocityProperty, tr("Angular Velocity"), PropertyStorage::Float, xyzList);
	registerStandardProperty(AngularMomentumProperty, tr("Angular Momentum"), PropertyStorage::Float, xyzList);
	registerStandardProperty(TorqueProperty, tr("Torque"), PropertyStorage::Float, xyzList);
	registerStandardProperty(SpinProperty, tr("Spin"), PropertyStorage::Float, emptyList);
	registerStandardProperty(CentroSymmetryProperty, tr("Centrosymmetry"), PropertyStorage::Float, emptyList);
	registerStandardProperty(VelocityMagnitudeProperty, tr("Velocity Magnitude"), PropertyStorage::Float, emptyList);
	registerStandardProperty(MoleculeProperty, tr("Molecule Identifier"), PropertyStorage::Int64, emptyList);
	registerStandardProperty(AsphericalShapeProperty, tr("Aspherical Shape"), PropertyStorage::Float, xyzList);
	registerStandardProperty(VectorColorProperty, tr("Vector Color"), PropertyStorage::Float, rgbList, tr("Vector colors"));
	registerStandardProperty(ElasticStrainTensorProperty, tr("Elastic Strain"), PropertyStorage::Float, symmetricTensorList);
	registerStandardProperty(ElasticDeformationGradientProperty, tr("Elastic Deformation Gradient"), PropertyStorage::Float, tensorList);
	registerStandardProperty(RotationProperty, tr("Rotation"), PropertyStorage::Float, quaternionList);
	registerStandardProperty(StretchTensorProperty, tr("Stretch Tensor"), PropertyStorage::Float, symmetricTensorList);
	registerStandardProperty(MoleculeTypeProperty, tr("Molecule Type"), PropertyStorage::Float, emptyList, tr("Molecule types"));
}

/******************************************************************************
* Returns the index of the element that was picked in a viewport.
******************************************************************************/
std::pair<size_t, ConstDataObjectPath> ParticlesObject::OOMetaClass::elementFromPickResult(const ViewportPickResult& pickResult) const
{
	// Check if a particle was picked.
	if(const ParticlePickInfo* pickInfo = dynamic_object_cast<ParticlePickInfo>(pickResult.pickInfo())) {
		if(const ParticlesObject* particles = pickInfo->pipelineState().getObject<ParticlesObject>()) {
			size_t particleIndex = pickInfo->particleIndexFromSubObjectID(pickResult.subobjectId());
			if(particleIndex < particles->elementCount())
				return std::make_pair(particleIndex, ConstDataObjectPath({particles}));
		}
	}

	return std::pair<size_t, ConstDataObjectPath>(std::numeric_limits<size_t>::max(), {});
}

/******************************************************************************
* Tries to remap an index from one property container to another, considering the
* possibility that elements may have been added or removed.
******************************************************************************/
size_t ParticlesObject::OOMetaClass::remapElementIndex(const ConstDataObjectPath& source, size_t elementIndex, const ConstDataObjectPath& dest) const
{
	const ParticlesObject* sourceParticles = static_object_cast<ParticlesObject>(source.back());
	const ParticlesObject* destParticles = static_object_cast<ParticlesObject>(dest.back());

	// If unique IDs are available try to use them to look up the particle in the other data collection.
	if(const PropertyObject* sourceIdentifiers = sourceParticles->getProperty(ParticlesObject::IdentifierProperty)) {
		if(const PropertyObject* destIdentifiers = destParticles->getProperty(ParticlesObject::IdentifierProperty)) {
			qlonglong id = sourceIdentifiers->get<qlonglong>(elementIndex);
			size_t mappedId = boost::find(destIdentifiers->crange<qlonglong>(), id) - destIdentifiers->cdata<qlonglong>();
			if(mappedId != destIdentifiers->size())
				return mappedId;
		}
	}

	// Next, try to use the position to find the right particle in the other data collection.
	if(const PropertyObject* sourcePositions = sourceParticles->getProperty(ParticlesObject::PositionProperty)) {
		if(const PropertyObject* destPositions = destParticles->getProperty(ParticlesObject::PositionProperty)) {
			const Point3& pos = sourcePositions->get<Point3>(elementIndex);
			size_t mappedId = boost::find(destPositions->crange<Point3>(), pos) - destPositions->cdata<Point3>();
			if(mappedId != destPositions->size())
				return mappedId;
		}
	}

	// Give up.
	return PropertyContainerClass::remapElementIndex(source, elementIndex, dest);
}

/******************************************************************************
* Determines which elements are located within the given
* viewport fence region (=2D polygon).
******************************************************************************/
boost::dynamic_bitset<> ParticlesObject::OOMetaClass::viewportFenceSelection(const QVector<Point2>& fence, const ConstDataObjectPath& objectPath, PipelineSceneNode* node, const Matrix4& projectionTM) const
{
	const ParticlesObject* particles = static_object_cast<ParticlesObject>(objectPath.back());
	if(const PropertyObject* posProperty = particles->getProperty(ParticlesObject::PositionProperty)) {

		if(!particles->visElement() || particles->visElement()->isEnabled() == false)
			node->throwException(tr("Cannot select particles while the corresponding visual element is disabled. Please enable the display of particles first."));

		boost::dynamic_bitset<> fullSelection(posProperty->size());
		QMutex mutex;
		parallelForChunks(posProperty->size(), [pos = posProperty->cdata<Point3>(), &projectionTM, &fence, &mutex, &fullSelection](size_t startIndex, size_t chunkSize) {
			boost::dynamic_bitset<> selection(fullSelection.size());
			auto p = pos + startIndex;
			for(size_t index = startIndex; chunkSize != 0; chunkSize--, index++, ++p) {

				// Project particle center to screen coordinates.
				Point3 projPos = projectionTM * (*p);

				// Perform z-clipping.
				if(std::abs(projPos.z()) >= FloatType(1))
					continue;

				// Perform point-in-polygon test.
				int intersectionsLeft = 0;
				int intersectionsRight = 0;
				for(auto p2 = fence.constBegin(), p1 = p2 + (fence.size()-1); p2 != fence.constEnd(); p1 = p2++) {
					if(p1->y() == p2->y()) continue;
					if(projPos.y() >= p1->y() && projPos.y() >= p2->y()) continue;
					if(projPos.y() < p1->y() && projPos.y() < p2->y()) continue;
					FloatType xint = (projPos.y() - p2->y()) / (p1->y() - p2->y()) * (p1->x() - p2->x()) + p2->x();
					if(xint >= projPos.x())
						intersectionsRight++;
					else
						intersectionsLeft++;
				}
				if(intersectionsRight & 1)
					selection.set(index);
			}
			// Transfer thread-local results to output bit array.
			QMutexLocker locker(&mutex);
			fullSelection |= selection;
		});

		return fullSelection;
	}

	// Give up.
	return PropertyContainerClass::viewportFenceSelection(fence, objectPath, node, projectionTM);
}

}	// End of namespace
}	// End of namespace
