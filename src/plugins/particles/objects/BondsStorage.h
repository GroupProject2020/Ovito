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


#include <plugins/particles/Particles.h>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>

namespace Ovito { namespace Particles {

/**
 * A bond between two particles.
 */
struct Bond 
{
	/// The index of the first particle.
	/// Note that we are using int instead of size_t here to save some memory.
	size_t index1;

	/// The index of the second particle.
	/// Note that we are using int instead of size_t here to save some memory.
	size_t index2;

	/// If the bond crosses a periodic boundary, this indicates the direction.
	Vector_3<int8_t> pbcShift;

	/// Returns the flipped version of this bond, where the two particles are swapped
	/// and the PBC shift vector is reversed.
	Bond flipped() const { return {index2, index1, -pbcShift}; }

	/// For a pair of bonds, A<->B and B<->A, determines whether this bond
	/// counts as the 'odd' or the 'even' bond of the pair.
	bool isOdd() const {
		// Is this bond connecting two different particles?
		// If yes, it's easy to determine whether it's an even or an odd bond.
		if(index1 > index2) return true;
		else if(index1 < index2) return false;
		// Whether the bond is 'odd' is determined by the PBC shift vector.
		if(pbcShift[0] != 0) return pbcShift[0] < 0;
		if(pbcShift[1] != 0) return pbcShift[1] < 0;
		// A particle shouldn't be bonded to itself unless the bond crosses a periodic cell boundary:
		OVITO_ASSERT(pbcShift != Vector_3<int8_t>::Zero());
		return pbcShift[2] < 0;
	}
};

/**
 * \brief A list of bonds that connect pairs of particles.
 */
class OVITO_PARTICLES_EXPORT BondsStorage : public std::vector<Bond>
{
public:

	/// Writes the stored data to an output stream.
	void saveToStream(SaveStream& stream, bool onlyMetadata) const;

	/// Reads the stored data from an input stream.
	void loadFromStream(LoadStream& stream);

	/// Reduces the size of the storage array, removing bonds for which 
	/// the corresponding bits in the bit array are set.
	void filterResize(const boost::dynamic_bitset<>& mask);	
};

/// Typically, BondsStorage objects are shallow copied. That's why we use a shared_ptr to hold on to them.
using BondsPtr = std::shared_ptr<BondsStorage>;

/// This pointer type is used to indicate that we only need read-only access to the bond data.
using ConstBondsPtr = std::shared_ptr<const BondsStorage>;

/**
 * \brief Helper class that allows to efficiently iterate over the bonds that are adjacent to a particle.
 */
class OVITO_PARTICLES_EXPORT ParticleBondMap
{
public:

	class bond_index_iterator : public boost::iterator_facade<bond_index_iterator, size_t const, boost::forward_traversal_tag, size_t> {
	public:
		bond_index_iterator() : _bondMap(nullptr), _currentIndex(0) {}
		bond_index_iterator(const ParticleBondMap* map, size_t startIndex) :
			_bondMap(map), _currentIndex(startIndex) {}
	private:
		size_t _currentIndex;
		const ParticleBondMap* _bondMap;

		friend class boost::iterator_core_access;

		void increment() {
			_currentIndex = _bondMap->_nextBond[_currentIndex];
		}

		bool equal(const bond_index_iterator& other) const {
			OVITO_ASSERT(_bondMap == other._bondMap);
			return this->_currentIndex == other._currentIndex;
		}

		size_t dereference() const {
			OVITO_ASSERT(_currentIndex < _bondMap->_nextBond.size());
			return _currentIndex / 2;
		}
	};

	class bond_iterator : public boost::iterator_facade<bond_iterator, Bond const, boost::forward_traversal_tag, Bond> {
	public:
		bond_iterator() : _bondMap(nullptr), _currentIndex(0) {}
		bond_iterator(const ParticleBondMap* map, size_t startIndex) :
			_bondMap(map), _currentIndex(startIndex) {}
	private:
		size_t _currentIndex;
		const ParticleBondMap* _bondMap;

		friend class boost::iterator_core_access;

		void increment() {
			_currentIndex = _bondMap->_nextBond[_currentIndex];
		}

		bool equal(const bond_iterator& other) const {
			OVITO_ASSERT(_bondMap == other._bondMap);
			return this->_currentIndex == other._currentIndex;
		}

		Bond dereference() const {
			OVITO_ASSERT(_currentIndex < _bondMap->_nextBond.size());
			const Bond& bond = _bondMap->_bonds[_currentIndex / 2];
			if((_currentIndex & 1) == 0)
				return bond;
			else
				return bond.flipped();
		}
	};


public:

	/// Initializes the helper class.
	ParticleBondMap(const BondsStorage& bonds);

	/// Returns an iterator range over the indices of the bonds adjacent to the given particle.
	/// Returns real indices into the bonds list. Note that bonds can point away from and to the given particle.
	boost::iterator_range<bond_index_iterator> bondIndicesOfParticle(size_t particleIndex) const {
		size_t firstIndex = (particleIndex < _startIndices.size()) ? _startIndices[particleIndex] : endOfListValue();
		return boost::iterator_range<bond_index_iterator>(
				bond_index_iterator(this, firstIndex),
				bond_index_iterator(this, endOfListValue()));
	}

	/// Returns an iterator range over the bonds adjacent to the given particle.
	/// Takes care of reversing bonds that point toward the particle. Thus, all bonds
	/// enumerated by the iterator point away from the given particle.
	boost::iterator_range<bond_iterator> bondsOfParticle(size_t particleIndex) const {
		size_t firstIndex = (particleIndex < _startIndices.size()) ? _startIndices[particleIndex] : endOfListValue();
		return boost::iterator_range<bond_iterator>(
				bond_iterator(this, firstIndex),
				bond_iterator(this, endOfListValue()));
	}
	
	/// Returns the index of a bond in the bonds list if it exists.
	/// Returns the total number of bonds to indicate that the bond does not exist.
	size_t findBond(const Bond& bond) const {
		size_t index = (bond.index1 < _startIndices.size()) ? _startIndices[bond.index1] : endOfListValue();
		for(; index != endOfListValue(); index = _nextBond[index]) {
			const Bond& current_bond = _bonds[index/2];
			if((index & 1) == 0) {
				OVITO_ASSERT(current_bond.index1 == bond.index1);
				if(current_bond.index2 == bond.index2 && current_bond.pbcShift == bond.pbcShift)
					return index/2;
			}
			else {
				OVITO_ASSERT(current_bond.index2 == bond.index1);
				if(current_bond.index1 == bond.index2 && current_bond.pbcShift == -bond.pbcShift)
					return index/2;
			}
		}
		return _bonds.size();
	}

private:

	/// Returns the number of half bonds, which is used to indicate the end of the per-particle bond list.
	size_t endOfListValue() const { return _nextBond.size(); }
	
private:

	/// The bonds storage this map has been created for.
	const BondsStorage& _bonds;

	/// Contains the first bond index for each particle (the head of a linked list).
	std::vector<size_t> _startIndices;

	/// Stores the index of the next bond in the linked list.
	std::vector<size_t> _nextBond;
};

}	// End of namespace
}	// End of namespace


