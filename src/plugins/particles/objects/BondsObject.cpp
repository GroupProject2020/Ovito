///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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
#include "BondsObject.h"
#include "BondsDisplay.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(BondsObject);	
DEFINE_PROPERTY_FIELD(BondsObject, storage);

/// Holds a shared, empty instance of the BondsStorage class, 
/// which is used in places where a default storage is needed.
/// This singleton instance is never modified.
static const BondsPtr defaultStorage = std::make_shared<BondsStorage>();

/******************************************************************************
* Default constructor.
******************************************************************************/
BondsObject::BondsObject(DataSet* dataset) : DataObject(dataset), _storage(defaultStorage)
{
	// Attach a display object.
	addDisplayObject(new BondsDisplay(dataset));
}

/******************************************************************************
* Returns the data encapsulated by this object after making sure it is not 
* shared with other owners.
******************************************************************************/
const std::shared_ptr<BondsStorage>& BondsObject::modifiableStorage() 
{
	// Copy data storage on write if there is more than one reference to the storage.
	OVITO_ASSERT(storage());
	OVITO_ASSERT(storage().use_count() >= 1);
	if(storage().use_count() > 1)
		_storage.mutableValue() = std::make_shared<BondsStorage>(*storage());
	OVITO_ASSERT(storage().use_count() == 1);
	return storage();
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void BondsObject::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	DataObject::saveToStream(stream, excludeRecomputableData);

	stream.beginChunk(0x01);
	storage()->saveToStream(stream, excludeRecomputableData);
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void BondsObject::loadFromStream(ObjectLoadStream& stream)
{
	DataObject::loadFromStream(stream);

	stream.expectChunk(0x01);
	modifiableStorage()->loadFromStream(stream);
	stream.closeChunk();
}

/******************************************************************************
* Remaps the bonds after some of the particles have been deleted.
* Dangling bonds are removed and the list of deleted bonds is returned as a bit array.
******************************************************************************/
boost::dynamic_bitset<> BondsObject::particlesDeleted(const boost::dynamic_bitset<>& deletedParticlesMask)
{
	// Build map that maps old particle indices to new indices.
	std::vector<size_t> indexMap(deletedParticlesMask.size());
	auto index = indexMap.begin();
	size_t oldParticleCount = deletedParticlesMask.size();
	size_t newParticleCount = 0;
	for(size_t i = 0; i < deletedParticlesMask.size(); i++)
		*index++ = deletedParticlesMask.test(i) ? std::numeric_limits<size_t>::max() : newParticleCount++;

	boost::dynamic_bitset<> deletedBondsMask(storage()->size());

	auto result = modifiableStorage()->begin();
	auto bond = modifiableStorage()->begin();
	auto last = modifiableStorage()->end();
	size_t bondIndex = 0;
	for(; bond != last; ++bond, ++bondIndex) {
		// Remove invalid bonds, i.e. whose particle indices are out of bounds.
		if(bond->index1 >= oldParticleCount || bond->index2 >= oldParticleCount) {
			deletedBondsMask.set(bondIndex);
			continue;
		}

		// Remove dangling bonds whose particles have gone.
		if(deletedParticlesMask.test(bond->index1) || deletedParticlesMask.test(bond->index2)) {
			deletedBondsMask.set(bondIndex);
			continue;
		}

		// Keep bond and remap particle indices.
		result->pbcShift = bond->pbcShift;
		result->index1 = indexMap[bond->index1];
		result->index2 = indexMap[bond->index2];
		++result;
	}
	modifiableStorage()->erase(result, last);
	notifyDependents(ReferenceEvent::TargetChanged);
	
	return deletedBondsMask;
}

}	// End of namespace
}	// End of namespace
