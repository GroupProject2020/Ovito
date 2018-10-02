///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include <core/utilities/units/UnitsManager.h>
#include "SimulationCellObject.h"
#include "SimulationCellVis.h"

namespace Ovito { namespace StdObj {
	
IMPLEMENT_OVITO_CLASS(SimulationCellObject);
DEFINE_PROPERTY_FIELD(SimulationCellObject, cellMatrix);
DEFINE_PROPERTY_FIELD(SimulationCellObject, pbcX);
DEFINE_PROPERTY_FIELD(SimulationCellObject, pbcY);
DEFINE_PROPERTY_FIELD(SimulationCellObject, pbcZ);
DEFINE_PROPERTY_FIELD(SimulationCellObject, is2D);
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, cellMatrix, "Cell matrix");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, pbcX, "Periodic boundary conditions (X)");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, pbcY, "Periodic boundary conditions (Y)");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, pbcZ, "Periodic boundary conditions (Z)");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, is2D, "2D");
SET_PROPERTY_FIELD_UNITS(SimulationCellObject, cellMatrix, WorldParameterUnit);

/******************************************************************************
* Creates the storage for the internal parameters.
******************************************************************************/
void SimulationCellObject::init(DataSet* dataset)
{
	// Attach a visualization element for rendering the simulation box.
	addVisElement(new SimulationCellVis(dataset));
}

/******************************************************************************
* Puts the simulation cell into a writable state.
* In the writable state, the Python binding layer will allow write access to 
* the cell's internal data.
******************************************************************************/
void SimulationCellObject::makeWritableFromPython()
{
	if(!isSafeToModify())
		throwException(tr("Modifying the cell data is not allowed, because the SimulationCell object is currently shared by more than one data collection. Please explicitly request a mutable version of the SimulationCell object by using the '_' notation."));
	_isWritableFromPython++; 
}

}	// End of namespace
}	// End of namespace
