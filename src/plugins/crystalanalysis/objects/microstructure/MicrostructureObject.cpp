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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include "MicrostructureObject.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(MicrostructureObject);
DEFINE_PROPERTY_FIELD(MicrostructureObject, storage);

/// Holds a shared, empty instance of the Microstructure class, 
/// which is used in places where a default storage is needed.
/// This singleton instance is never modified.
static const std::shared_ptr<Microstructure> defaultStorage = std::make_shared<Microstructure>(std::make_shared<ClusterGraph>());

/******************************************************************************
* Constructor.
******************************************************************************/
MicrostructureObject::MicrostructureObject(DataSet* dataset) : PeriodicDomainDataObject(dataset), _storage(defaultStorage)
{
}

/******************************************************************************
* Returns the data encapsulated by this object after making sure it is not 
* shared with other owners.
******************************************************************************/
const std::shared_ptr<Microstructure>& MicrostructureObject::modifiableStorage() 
{
	// Copy data storage on write if there is more than one reference to the storage.
	OVITO_ASSERT(storage());
	OVITO_ASSERT(storage().use_count() >= 1);
	if(storage().use_count() > 1)
		_storage.mutableValue() = std::make_shared<Microstructure>(*storage());
	OVITO_ASSERT(storage().use_count() == 1);
	return storage();
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
