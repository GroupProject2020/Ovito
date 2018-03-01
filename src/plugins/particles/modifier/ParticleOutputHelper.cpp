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

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/BondsVis.h>
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/particles/objects/ParticleBondMap.h>
#include <core/dataset/DataSet.h>
#include "ParticleOutputHelper.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers)

/******************************************************************************
* Constructor.
******************************************************************************/
ParticleOutputHelper::ParticleOutputHelper(DataSet* dataset, PipelineFlowState& output) : 
	OutputHelper(dataset, output)
{
	// Find the 'Position' particle property and optionally a BondsObject in the input flow state.
	ParticleProperty* posProperty = nullptr;
	BondProperty* topologyProperty = nullptr;
	for(DataObject* obj : output.objects()) {
		if(ParticleProperty* p = dynamic_object_cast<ParticleProperty>(obj)) {
			if(p->type() == ParticleProperty::PositionProperty) {
				if(posProperty)
					dataset->throwException(PropertyObject::tr("Detected invalid modifier input. There are multiple 'Position' particle properties."));
				posProperty = p;
			}
		}
		else if(BondProperty* p = dynamic_object_cast<BondProperty>(obj)) {
			if(p->type() == BondProperty::TopologyProperty) {
				if(topologyProperty)
					dataset->throwException(PropertyObject::tr("Detected invalid modifier input. There are multiple bond topology properties."));
				topologyProperty = p;
			}
		}
	}
	_outputParticleCount = (posProperty != nullptr) ? posProperty->size() : 0;
	_outputBondCount = (topologyProperty != nullptr) ? topologyProperty->size() : 0;
	
	// Verify input, make sure array lengths of particle properties are consistent.
	for(DataObject* obj : output.objects()) {
		if(ParticleProperty* p = dynamic_object_cast<ParticleProperty>(obj)) {
			if(p->size() != outputParticleCount())
				dataset->throwException(PropertyObject::tr("Detected invalid modifier input. Data array size is not the same for all particle properties or property 'Position' is not present."));
		}
		else if(BondProperty* p = dynamic_object_cast<BondProperty>(obj)) {
			if(p->size() != outputBondCount())
				dataset->throwException(PropertyObject::tr("Detected invalid modifier input. Data array size is not the same for all bond properties."));
		}
	}
}

/******************************************************************************
* Deletes the particles for which bits are set in the given bit-mask.
* Returns the number of deleted particles.
******************************************************************************/
size_t ParticleOutputHelper::deleteParticles(const boost::dynamic_bitset<>& mask)
{
	OVITO_ASSERT(mask.size() == outputParticleCount());

	size_t deleteCount = mask.count();
	size_t oldParticleCount = outputParticleCount();
	size_t newParticleCount = oldParticleCount - deleteCount;
	if(deleteCount == 0)
		return 0;	// Nothing to delete.

	_outputParticleCount = newParticleCount;

	// Modify particle properties.
	for(DataObject* obj : output().objects()) {
		if(ParticleProperty* existingProperty = dynamic_object_cast<ParticleProperty>(obj)) {
			OVITO_ASSERT(existingProperty->size() == oldParticleCount);
			ParticleProperty* newProperty = cloneIfNeeded(existingProperty);
			newProperty->filterResize(mask);
			OVITO_ASSERT(newProperty->size() == newParticleCount);
		}
	}

	// Delete dangling bonds, i.e. those that are incident on deleted particles.
	if(BondProperty* topologyProperty = BondProperty::findInState(output(), BondProperty::TopologyProperty)) {

		boost::dynamic_bitset<> deletedBondsMask(topologyProperty->size());
		size_t newBondCount = 0;
		size_t oldBondCount = outputBondCount();
		OVITO_ASSERT(oldBondCount == topologyProperty->size());

		// Build map from old particle indices to new indices.
		std::vector<size_t> indexMap(oldParticleCount);
		auto index = indexMap.begin();
		size_t count = 0;
		for(size_t i = 0; i < oldParticleCount; i++)
			*index++ = mask.test(i) ? std::numeric_limits<size_t>::max() : count++;

		// Remap particle indices of stored bonds and remove dangling bonds.
		BondProperty* newTopology = cloneIfNeeded(topologyProperty);
		for(size_t bondIndex = 0; bondIndex < oldBondCount; bondIndex++) {
			size_t index1 = newTopology->getInt64Component(bondIndex, 0);
			size_t index2 = newTopology->getInt64Component(bondIndex, 1);

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
			newTopology->setInt64Component(bondIndex, 0, indexMap[index1]);
			newTopology->setInt64Component(bondIndex, 1, indexMap[index2]);
		}

		// Delete the marked bonds.
		deleteBonds(deletedBondsMask);
	}

	return deleteCount;
}

/******************************************************************************
* Adds a set of new bonds to the system.
******************************************************************************/
void ParticleOutputHelper::addBonds(const std::vector<Bond>& newBonds, BondsVis* bondsVis, const std::vector<PropertyPtr>& bondProperties)
{
	// Check if there are existing bonds.
	OORef<BondProperty> existingBondsTopology = BondProperty::findInState(output(), BondProperty::TopologyProperty);
	if(!existingBondsTopology) {
		OVITO_ASSERT(outputBondCount() == 0);

		// Create essential bond properties.
		PropertyPtr topologyProperty = BondProperty::createStandardStorage(newBonds.size(), BondProperty::TopologyProperty, false);
		PropertyPtr periodicImageProperty = BondProperty::createStandardStorage(newBonds.size(), BondProperty::PeriodicImageProperty, false);

		// Copy data into property arrays.
		auto t = topologyProperty->dataInt64();
		auto pbc = periodicImageProperty->dataVector3I();
		for(const Bond& bond : newBonds) {
			*t++ = bond.index1;
			*t++ = bond.index2;
			*pbc++ = bond.pbcShift;
		}

		// Insert property objects into the output pipeline state.
		OORef<BondProperty> topologyPropertyObj = BondProperty::createFromStorage(dataset(), topologyProperty);
		OORef<BondProperty> periodicImagePropertyObj = BondProperty::createFromStorage(dataset(), periodicImageProperty);
		if(bondsVis)
			topologyPropertyObj->setVisElement(bondsVis);
		output().addObject(topologyPropertyObj);
		output().addObject(periodicImagePropertyObj);
		setOutputBondCount(newBonds.size());

		// Insert other bond properties.
		for(const auto& bprop : bondProperties) {
			OVITO_ASSERT(bprop->size() == newBonds.size());
			OVITO_ASSERT(bprop->type() != BondProperty::TopologyProperty);
			OVITO_ASSERT(bprop->type() != BondProperty::PeriodicImageProperty);
			outputProperty<BondProperty>(bprop);
		}
	}
	else {

		// This is needed to determine which bonds already exist.
		OORef<BondProperty> existingPeriodicImages = BondProperty::findInState(output(), BondProperty::PeriodicImageProperty);
		ParticleBondMap bondMap(existingBondsTopology->storage(), existingPeriodicImages ? existingPeriodicImages->storage() : nullptr);

		// Check which bonds are new and need to be merged.
		size_t originalBondCount = existingBondsTopology->size();
		std::vector<size_t> mapping(newBonds.size());
		for(size_t bondIndex = 0; bondIndex < newBonds.size(); bondIndex++) {
			// Check if there is already a bond like this.
			const Bond& bond = newBonds[bondIndex];
			auto existingBondIndex = bondMap.findBond(bond);
			if(existingBondIndex == originalBondCount) {
				// It's a new bond.
				mapping[bondIndex] = _outputBondCount;
				_outputBondCount++;
			}
			else {
				// It's an already existing bond.
				mapping[bondIndex] = existingBondIndex;
			}
		}

		// Duplicate the existing property objects.
		OORef<BondProperty> newBondsTopology = cloneHelper().cloneObject(existingBondsTopology, false);
		output().replaceObject(existingBondsTopology, newBondsTopology);

		OORef<BondProperty> newBondsPeriodicImages;
		if(OORef<BondProperty> existingBondsPeriodicImages = BondProperty::findInState(output(), BondProperty::PeriodicImageProperty)) {
			newBondsPeriodicImages = cloneHelper().cloneObject(existingBondsPeriodicImages, false);
			output().replaceObject(existingBondsPeriodicImages, newBondsPeriodicImages);
		}
		else {
			newBondsPeriodicImages = BondProperty::createFromStorage(dataset(), BondProperty::createStandardStorage(outputBondCount(), BondProperty::PeriodicImageProperty, true));
			output().addObject(newBondsPeriodicImages);
		}

		// Copy bonds information into the extended arrays.
		newBondsTopology->resize(outputBondCount(), true);
		newBondsPeriodicImages->resize(outputBondCount(), true);
		for(size_t bondIndex = 0; bondIndex < newBonds.size(); bondIndex++) {
			if(mapping[bondIndex] >= originalBondCount) {
				const Bond& bond = newBonds[bondIndex];
				newBondsTopology->setInt64Component(mapping[bondIndex], 0, bond.index1);
				newBondsTopology->setInt64Component(mapping[bondIndex], 1, bond.index2);
				newBondsPeriodicImages->setVector3I(mapping[bondIndex], bond.pbcShift);
			}
		}

		// Extend existing bond property arrays.
		for(DataObject* outobj : output().objects()) {
			if(BondProperty* originalBondPropertyObject = dynamic_object_cast<BondProperty>(outobj)) {
				if(originalBondPropertyObject == newBondsTopology) continue;
				if(originalBondPropertyObject == newBondsPeriodicImages) continue;
				if(originalBondPropertyObject->size() != originalBondCount) continue;

				// Create a modifiable copy.
				OORef<BondProperty> newBondPropertyObject = cloneHelper().cloneObject(originalBondPropertyObject, false);

				// Extend array.
				newBondPropertyObject->resize(outputBondCount(), true);

				// Replace bond property in pipeline flow state.
				output().replaceObject(originalBondPropertyObject, newBondPropertyObject);
			}
		}

		// Merge new bond properties.
		for(const auto& bprop : bondProperties) {
			OVITO_ASSERT(bprop->size() == newBonds.size());
			OVITO_ASSERT(bprop->type() != BondProperty::TopologyProperty);
			OVITO_ASSERT(bprop->type() != BondProperty::PeriodicImageProperty);

			OORef<BondProperty> propertyObject;

			if(bprop->type() != BondProperty::UserProperty) {
				propertyObject = BondProperty::findInState(output(), (BondProperty::Type)bprop->type());
				if(!propertyObject)
					propertyObject = outputStandardProperty<BondProperty>(bprop->type(), true);
			}
			else {
				propertyObject = outputCustomProperty<BondProperty>(bprop->name(), bprop->dataType(), bprop->componentCount(), bprop->stride(), true);
			}

			// Copy bond property data.
			propertyObject->modifiableStorage()->mappedCopy(*bprop, mapping);
		}
	}
}

/******************************************************************************
* Deletes the bonds for which bits are set in the given bit-mask.
* Returns the number of deleted bonds.
******************************************************************************/
size_t ParticleOutputHelper::deleteBonds(const boost::dynamic_bitset<>& mask)
{
	OVITO_ASSERT(mask.size() == outputBondCount());

	size_t deleteCount = mask.count();
	size_t oldBondCount = outputBondCount();
	size_t newBondCount = oldBondCount - deleteCount;
	if(deleteCount == 0)
		return 0;	// Nothing to delete.

	// Modify bond properties and the BondsObject.
	for(DataObject* obj : output().objects()) {
		if(BondProperty* existingProperty = dynamic_object_cast<BondProperty>(obj)) {
			OVITO_ASSERT(existingProperty->size() == oldBondCount);
			BondProperty* newProperty = cloneIfNeeded(existingProperty);
			newProperty->filterResize(mask);
			OVITO_ASSERT(newProperty->size() == newBondCount);
		}
	}
	setOutputBondCount(newBondCount);

	return deleteCount;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

