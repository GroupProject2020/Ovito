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

#pragma once


#include <plugins/stdobj/StdObj.h>
#include <core/dataset/data/DataObject.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>

namespace Ovito { namespace StdObj {
	
/**
 * \brief A closed triangle mesh representing a surface.
 */
class OVITO_STDOBJ_EXPORT PeriodicDomainDataObject : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(PeriodicDomainDataObject)
	
protected:

	/// \brief Constructor.
	PeriodicDomainDataObject(DataSet* dataset);

private:

	/// The domain the object is embedded in.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(SimulationCellObject, domain, setDomain, PROPERTY_FIELD_ALWAYS_DEEP_COPY);

	/// The planar cuts applied to object.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QVector<Plane3>, cuttingPlanes, setCuttingPlanes);
};

}	// End of namespace
}	// End of namespace
