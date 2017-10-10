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

#pragma once


#include <plugins/particles/Particles.h>
#include <core/dataset/data/DataObject.h>
#include "BondsStorage.h"

namespace Ovito { namespace Particles {

/**
 * \brief Stores the bonds between particles.
 */
class OVITO_PARTICLES_EXPORT BondsObject : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(BondsObject)

public:

	/// \brief Constructor.
	Q_INVOKABLE BondsObject(DataSet* dataset);

	/// \brief Returns the title of this object.
	virtual QString objectTitle() override { return tr("Bonds"); }

	/// Deletes all bonds.
	void clear() {
		modifiableStorage()->clear();
		notifyDependents(ReferenceEvent::TargetChanged);
	}

	/// Returns the number of bonds.
	size_t size() const {
		return storage()->size();
	}

	/// Returns the data encapsulated by this object after making sure it is not shared with other owners.
	const std::shared_ptr<BondsStorage>& modifiableStorage();

	/// Inserts a new bond into the list.
	void addBond(size_t index1, size_t index2, Vector_3<int8_t> pbcShift = Vector_3<int8_t>::Zero()) {
		modifiableStorage()->push_back(Bond{ index1, index2, pbcShift });
		notifyDependents(ReferenceEvent::TargetChanged);
	}

	/// Reduces the size of the storage array, removing bonds for which 
	/// the corresponding bits in the bit array are set.
	void filterResize(const boost::dynamic_bitset<>& mask) {
		modifiableStorage()->filterResize(mask);
		notifyDependents(ReferenceEvent::TargetChanged);
	}

	/// Remaps the bonds after some of the particles have been deleted.
	/// Dangling bonds are removed and the list of deleted bonds is returned as a bit array.
	boost::dynamic_bitset<> particlesDeleted(const boost::dynamic_bitset<>& deletedParticlesMask);

	/// \brief Returns whether this object, when returned as an editable sub-object by another object,
	///        should be displayed in the modification stack.
	virtual bool isSubObjectEditable() const override { return false; }

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

private:

	/// The internal data.
	DECLARE_RUNTIME_PROPERTY_FIELD(std::shared_ptr<BondsStorage>, storage, setStorage);
};

}	// End of namespace
}	// End of namespace


