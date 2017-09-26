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

#include <plugins/stdobj/StdObj.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include "PeriodicDomainDataObject.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(PeriodicDomainDataObject);
DEFINE_REFERENCE_FIELD(PeriodicDomainDataObject, domain);
DEFINE_PROPERTY_FIELD(PeriodicDomainDataObject, cuttingPlanes);

/******************************************************************************
* Constructor.
******************************************************************************/
PeriodicDomainDataObject::PeriodicDomainDataObject(DataSet* dataset) : DataObject(dataset)
{
}

}	// End of namespace
}	// End of namespace
