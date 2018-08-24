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

#pragma once


#include <plugins/particles/Particles.h>
#include <plugins/stdobj/properties/PropertyContainer.h>
#include "BondProperty.h"

namespace Ovito { namespace Particles {

/**
 * \brief This data object type is a container for bond properties.
 */
class OVITO_PARTICLES_EXPORT BondsObject : public PropertyContainer
{
	Q_OBJECT
	OVITO_CLASS(BondsObject)
	
public:

	/// \brief Constructor.
	Q_INVOKABLE BondsObject(DataSet* dataset);

	/// Returns the title of this object.
	virtual QString objectTitle() override { return tr("Bonds"); }

	/// Returns the class of properties that this container can store.
	virtual const PropertyClass& propertyClass() const override { return BondProperty::OOClass(); }

	/// Convinience method that returns the bond topology property.
	BondProperty* getTopology() const { return getProperty<BondProperty>(BondProperty::TopologyProperty); }

	/// Deletes the bonds for which bits are set in the given bit-mask.
	/// Returns the number of deleted bonds.
	size_t deleteBonds(const boost::dynamic_bitset<>& mask);
};

}	// End of namespace
}	// End of namespace
