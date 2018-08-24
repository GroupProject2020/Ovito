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
#include "BondsObject.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(BondsObject);

/******************************************************************************
* Constructor.
******************************************************************************/
BondsObject::BondsObject(DataSet* dataset) : PropertyContainer(dataset)
{
}

/******************************************************************************
* Deletes the bonds for which bits are set in the given bit-mask.
* Returns the number of deleted bonds.
******************************************************************************/
size_t BondsObject::deleteBonds(const boost::dynamic_bitset<>& mask)
{
	OVITO_ASSERT(mask.size() == elementCount());

	size_t deleteCount = mask.count();
	size_t oldBondCount = elementCount();
	size_t newBondCount = oldBondCount - deleteCount;
	if(deleteCount == 0)
		return 0;	// Nothing to delete.

    // Make sure the properties can be safely modified.
    makePropertiesUnique();

	// Modify bond properties.
	for(PropertyObject* property : properties()) {
        OVITO_ASSERT(property->size() == oldBondCount);
        property->filterResize(mask);
        OVITO_ASSERT(property->size() == newBondCount);
	}
    OVITO_ASSERT(elementCount() == newBondCount);

	return deleteCount;
}

}	// End of namespace
}	// End of namespace
