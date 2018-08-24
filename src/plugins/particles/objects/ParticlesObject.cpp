///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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
#include <core/oo/CloneHelper.h>
#include "ParticlesObject.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(ParticlesObject);
DEFINE_REFERENCE_FIELD(ParticlesObject, bonds);
SET_PROPERTY_FIELD_LABEL(ParticlesObject, bonds, "Bonds");

/******************************************************************************
* Constructor.
******************************************************************************/
ParticlesObject::ParticlesObject(DataSet* dataset) : PropertyContainer(dataset)
{
}

/******************************************************************************
* Duplicates the BondsObject if it is shared with other particle objects.
* After this method returns, all BondsObject are exclusively owned by the 
* container and can be safely modified without unwanted side effects.
******************************************************************************/
BondsObject* ParticlesObject::makeBondsUnique()
{
    if(bonds()->numberOfStrongReferences() > 1) {
		setBonds(CloneHelper().cloneObject(bonds(), false));
        OVITO_ASSERT(bonds()->numberOfStrongReferences() == 1);
    }
    return bonds();
}

/******************************************************************************
* Deletes the particles for which bits are set in the given bit-mask.
* Returns the number of deleted particles.
******************************************************************************/
size_t ParticlesObject::deleteParticles(const boost::dynamic_bitset<>& mask)
{
	OVITO_ASSERT(mask.size() == elementCount());

	size_t deleteCount = mask.count();
	size_t oldParticleCount = elementCount();
	size_t newParticleCount = oldParticleCount - deleteCount;
	if(deleteCount == 0)
		return 0;	// Nothing to delete.

    // Make sure the properties can be safely modified.
    makePropertiesUnique();

	// Modify particle properties.
	for(PropertyObject* property : properties()) {
        OVITO_ASSERT(property->size() == oldParticleCount);
        property->filterResize(mask);
        OVITO_ASSERT(property->size() == newParticleCount);
	}
    OVITO_ASSERT(elementCount() == newParticleCount);

	// Delete dangling bonds, i.e. those that are incident on deleted particles.
    if(bonds()) {
        // Make sure we can safely modify the bonds object.
        makeBondsUnique();

        size_t newBondCount = 0;
        size_t oldBondCount = bonds()->elementCount();
        boost::dynamic_bitset<> deletedBondsMask(oldBondCount);

        // Build map from old particle indices to new indices.
        std::vector<size_t> indexMap(oldParticleCount);
        auto index = indexMap.begin();
        size_t count = 0;
        for(size_t i = 0; i < oldParticleCount; i++)
            *index++ = mask.test(i) ? std::numeric_limits<size_t>::max() : count++;

        // Make sure we can sefly modify the bond properties.
        bonds()->makePropertiesUnique();

        // Remap particle indices of stored bonds and remove dangling bonds.
        if(BondProperty* topologyProperty = bonds()->getTopology()) {
            for(size_t bondIndex = 0; bondIndex < oldBondCount; bondIndex++) {
                size_t index1 = topologyProperty->getInt64Component(bondIndex, 0);
                size_t index2 = topologyProperty->getInt64Component(bondIndex, 1);

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
                topologyProperty->setInt64Component(bondIndex, 0, indexMap[index1]);
                topologyProperty->setInt64Component(bondIndex, 1, indexMap[index2]);
            }

            // Delete the marked bonds.
            bonds()->deleteBonds(deletedBondsMask);
        }
    }

	return deleteCount;
}

}	// End of namespace
}	// End of namespace
