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

#include <plugins/particles/Particles.h>
#include "BondsStorage.h"
#include "BondsObject.h"

namespace Ovito { namespace Particles {

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void BondsStorage::saveToStream(SaveStream& stream, bool onlyMetadata) const
{
	stream.beginChunk(0x01);
	if(!onlyMetadata) {
		stream.writeSizeT(size());
		stream.writeSizeT(sizeof(Bond));
		stream.write(data(), size() * sizeof(Bond));
	}
	else {
		stream.writeSizeT(0);
		stream.writeSizeT(sizeof(Bond));
	}
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void BondsStorage::loadFromStream(LoadStream& stream)
{
	stream.expectChunk(0x01);
	size_t bondCount;
	stream.readSizeT(bondCount);
	resize(bondCount);
	size_t bondSize;
	stream.readSizeT(bondSize);
	if(bondSize != sizeof(Bond) && bondCount != 0)
		throw Exception(BondsObject::tr("Data type size mismatch in stored bond list."));
	stream.read(data(), size() * sizeof(Bond));
	stream.closeChunk();
}

/******************************************************************************
* Reduces the size of the storage array, removing elements for which 
* the corresponding bits in the bit array are set.
******************************************************************************/
void BondsStorage::filterResize(const boost::dynamic_bitset<>& mask)
{
	OVITO_ASSERT(size() == mask.size());
	// Find the first bond to delete:
	size_t i;
	for(i = 0; i != mask.size(); i++)
		if(mask.test(i)) break;
	if(i == mask.size()) return;
	// Continue from here:
	auto b = begin() + i;
	for(; i != mask.size(); ++i) {
		if(!mask.test(i))
			*b++ = std::move((*this)[i]);
	}
	erase(b, end());
}

/******************************************************************************
* Initializes the helper class.
******************************************************************************/
ParticleBondMap::ParticleBondMap(const BondsStorage& bonds) :
	_bonds(bonds),
	_nextBond(bonds.size()*2, bonds.size()*2)
{
	size_t bondIndex = bonds.size() - 1;
	for(auto bond = bonds.crbegin(); bond != bonds.crend(); ++bond, bondIndex--) {
		if(bond->index1 >= _startIndices.size())
			_startIndices.resize(bond->index1 + 1, endOfListValue());
		if(bond->index2 >= _startIndices.size())
			_startIndices.resize(bond->index2 + 1, endOfListValue());

		size_t evenIndex = bondIndex * 2;
		size_t oddIndex  = evenIndex + 1;
		_nextBond[evenIndex] = _startIndices[bond->index1];
		_nextBond[oddIndex]  = _startIndices[bond->index2];
		_startIndices[bond->index1] = evenIndex;
		_startIndices[bond->index2] = oddIndex;
	}
}


}	// End of namespace
}	// End of namespace
