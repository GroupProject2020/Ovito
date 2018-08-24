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
#include "ParticleProperty.h"
#include "BondsObject.h"

namespace Ovito { namespace Particles {

/**
 * \brief This data object type is a container for particle properties.
 */
class OVITO_PARTICLES_EXPORT ParticlesObject : public PropertyContainer
{
	Q_OBJECT
	OVITO_CLASS(ParticlesObject)
	
public:

	/// \brief Constructor.
	Q_INVOKABLE ParticlesObject(DataSet* dataset);

	/// Returns the title of this object.
	virtual QString objectTitle() override { return tr("Particles"); }

	/// Returns the class of properties that this container can store.
	virtual const PropertyClass& propertyClass() const override { return ParticleProperty::OOClass(); }

	/// Deletes the particles for which bits are set in the given bit-mask.
	/// Returns the number of deleted particles.
	size_t deleteParticles(const boost::dynamic_bitset<>& mask);

	/// Duplicates the BondsObject if it is shared with other particle objects.
	/// After this method returns, all BondsObject are exclusively owned by the container and 
	/// can be safely modified without unwanted side effects.
	BondsObject* makeBondsUnique();

private:

	/// The bonds object.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(BondsObject, bonds, setBonds);
};

}	// End of namespace
}	// End of namespace
