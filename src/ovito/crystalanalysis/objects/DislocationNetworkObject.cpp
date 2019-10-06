///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include "DislocationNetworkObject.h"

namespace Ovito { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocationNetworkObject);
DEFINE_PROPERTY_FIELD(DislocationNetworkObject, storage);
DEFINE_REFERENCE_FIELD(DislocationNetworkObject, crystalStructures);
SET_PROPERTY_FIELD_LABEL(DislocationNetworkObject, crystalStructures, "Crystal structures");

/// Holds a shared, empty instance of the DislocationNetwork class,
/// which is used in places where a default storage is needed.
/// This singleton instance is never modified.
static const std::shared_ptr<DislocationNetwork> defaultStorage = std::make_shared<DislocationNetwork>(std::make_shared<ClusterGraph>());

/******************************************************************************
* Constructor.
******************************************************************************/
DislocationNetworkObject::DislocationNetworkObject(DataSet* dataset) : PeriodicDomainDataObject(dataset), _storage(defaultStorage)
{
	// Create the "unidentified" structure.
	OORef<MicrostructurePhase> defaultStructure(new MicrostructurePhase(dataset));
	defaultStructure->setName(tr("Unidentified structure"));
	defaultStructure->setColor(Color(1,1,1));
	defaultStructure->addBurgersVectorFamily(new BurgersVectorFamily(dataset));
	addCrystalStructure(defaultStructure);
}

/******************************************************************************
* Returns the data encapsulated by this object after making sure it is not
* shared with other owners.
******************************************************************************/
const std::shared_ptr<DislocationNetwork>& DislocationNetworkObject::modifiableStorage()
{
	// Copy data storage on write if there is more than one reference to the storage.
	OVITO_ASSERT(storage());
	OVITO_ASSERT(storage().use_count() >= 1);
	if(storage().use_count() > 1)
		_storage.mutableValue() = std::make_shared<DislocationNetwork>(*storage());
	OVITO_ASSERT(storage().use_count() == 1);
	return storage();
}

}	// End of namespace
}	// End of namespace
