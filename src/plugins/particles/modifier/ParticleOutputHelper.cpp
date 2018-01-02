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
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <plugins/particles/objects/BondProperty.h>
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
	BondsObject* bondsObj = nullptr;
	for(DataObject* obj : output.objects()) {
		if(ParticleProperty* p = dynamic_object_cast<ParticleProperty>(obj)) {
			if(p->type() == ParticleProperty::PositionProperty) {
				if(posProperty)
					dataset->throwException(PropertyObject::tr("Detected invalid modifier input. There are multiple 'Position' particle properties."));
				posProperty = p;
			}
		}
		else if(BondsObject* b = dynamic_object_cast<BondsObject>(obj)) {
			if(bondsObj)
				dataset->throwException(PropertyObject::tr("Detected invalid modifier input. Its contains multiple bonds arrays."));
			bondsObj = b;
		}
	}
	_outputParticleCount = (posProperty != nullptr) ? posProperty->size() : 0;
	_outputBondCount = (bondsObj != nullptr) ? bondsObj->size() : 0;	
	
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

	// Delete dangling bonds, i.e. those that are incident on a deleted particle.
	boost::dynamic_bitset<> deletedBondsMask;
	size_t newBondCount = 0;
	size_t oldBoundCount = outputBondCount();
	for(DataObject* obj : output().objects()) {
		if(BondsObject* existingBondsObject = dynamic_object_cast<BondsObject>(obj)) {
			// Create copy of bonds object.
			BondsObject* newBondsObject = cloneIfNeeded(existingBondsObject);
			// Remap particle indices of stored bonds and remove dangling bonds.
			deletedBondsMask = newBondsObject->particlesDeleted(mask);
			newBondCount = newBondsObject->size();
		}
	}
	_outputBondCount = newBondCount;

	// Filter bond properties.
	for(DataObject* outobj : output().objects()) {
		if(BondProperty* existingProperty = dynamic_object_cast<BondProperty>(outobj)) {
			OVITO_ASSERT(existingProperty->size() == oldBoundCount);
			BondProperty* newProperty = cloneIfNeeded(existingProperty);
			newProperty->filterResize(deletedBondsMask);
			OVITO_ASSERT(newProperty->size() == newBondCount);
		}
	}

	return deleteCount;
}

/******************************************************************************
* Adds a set of new bonds to the system.
******************************************************************************/
BondsObject* ParticleOutputHelper::addBonds(const BondsPtr& newBonds, BondsDisplay* bondsDisplay, const std::vector<PropertyPtr>& bondProperties)
{
	OVITO_ASSERT(newBonds);

	// Check if there is an existing bonds object coming from upstream.
	OORef<BondsObject> bondsObj = output().findObject<BondsObject>();
	if(!bondsObj) {
		OVITO_ASSERT(outputBondCount() == 0);

		// Create a new output data object.
		bondsObj = new BondsObject(dataset());
		bondsObj->setStorage(newBonds);
		if(bondsDisplay)
			bondsObj->setDisplayObject(bondsDisplay);

		// Insert output object into the pipeline.
		output().addObject(bondsObj);
		_outputBondCount = newBonds->size();

		// Insert bond properties.
		for(const auto& bprop : bondProperties) {
			OVITO_ASSERT(bprop->size() == newBonds->size());
			outputProperty<BondProperty>(bprop);
		}

		return bondsObj;
	}
	else {

		// Duplicate the existing bonds object and append the newly created bonds to it.
		OORef<BondsObject> bondsObjCopy = cloneHelper().cloneObject(bondsObj, false);
		BondsStorage* bonds = bondsObjCopy->modifiableStorage().get();
		OVITO_ASSERT(bonds != nullptr);

		// This is needed to determine which bonds already exist.
		ParticleBondMap bondMap(*bondsObj->storage());

		// Add bonds one by one.
		size_t originalBondCount = bonds->size();
		std::vector<int> mapping(newBonds->size());
		for(size_t bondIndex = 0; bondIndex < newBonds->size(); bondIndex++) {
			// Check if there is already a bond like this.
			const Bond& bond = (*newBonds)[bondIndex];
			auto existingBondIndex = bondMap.findBond(bond);
			if(existingBondIndex == originalBondCount) {
				// Create new bond.
				bonds->push_back(bond);
				mapping[bondIndex] = _outputBondCount;
				_outputBondCount++;
			}
			else {
				mapping[bondIndex] = existingBondIndex;
			}
		}

		// Replace original bonds object with the extended one.
		output().replaceObject(bondsObj, bondsObjCopy);

		// Extend existing bond property arrays.
		for(DataObject* outobj : output().objects()) {
			BondProperty* originalBondPropertyObject = dynamic_object_cast<BondProperty>(outobj);
			if(!originalBondPropertyObject || originalBondPropertyObject->size() != originalBondCount)
				continue;

			// Create a modifiable copy.
			OORef<BondProperty> newBondPropertyObject = cloneHelper().cloneObject(originalBondPropertyObject, false);

			// Extend array.
			newBondPropertyObject->resize(outputBondCount(), true);

			// Replace bond property in pipeline flow state.
			output().replaceObject(originalBondPropertyObject, newBondPropertyObject);
		}

		// Insert new bond properties.
		for(const auto& bprop : bondProperties) {
			OVITO_ASSERT(bprop->size() == newBonds->size());

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

		return bondsObjCopy;
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

	_outputBondCount = newBondCount;

	// Modify bond properties and the BondsObject.
	for(DataObject* obj : output().objects()) {
		if(BondProperty* existingProperty = dynamic_object_cast<BondProperty>(obj)) {
			OVITO_ASSERT(existingProperty->size() == oldBondCount);
			BondProperty* newProperty = cloneIfNeeded(existingProperty);
			newProperty->filterResize(mask);
			OVITO_ASSERT(newProperty->size() == newBondCount);
		}
		else if(BondsObject* existingBondsObject = dynamic_object_cast<BondsObject>(obj)) {
			// Create copy of bonds object.
			BondsObject* newBondsObject = cloneIfNeeded(existingBondsObject);
			newBondsObject->filterResize(mask);
			OVITO_ASSERT(newBondCount == newBondsObject->size());
		}
	}
	_outputBondCount = newBondCount;

	return deleteCount;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

