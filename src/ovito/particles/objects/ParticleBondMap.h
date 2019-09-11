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


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/BondsObject.h>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>
#include <utility>

namespace Ovito { namespace Particles {

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
			size_t bindex = _currentIndex / 2;
			Bond bond = { (size_t)_bondMap->_bondTopology->getInt64Component(bindex, 0), (size_t)_bondMap->_bondTopology->getInt64Component(bindex, 1),
								_bondMap->_bondPeriodicImages ? _bondMap->_bondPeriodicImages->getVector3I(bindex) : Vector3I::Zero() };
			if(_currentIndex & 1) {
				std::swap(bond.index1, bond.index2);
				bond.pbcShift = -bond.pbcShift;
			}
			return bond;
		}
	};


public:

	/// Initializes the helper class.
	ParticleBondMap(const BondsObject& bonds);

	/// Initializes the helper class.
	ParticleBondMap(ConstPropertyPtr bondTopology, ConstPropertyPtr bondPeriodicImages = nullptr);

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
			if((index & 1) == 0) {
				OVITO_ASSERT(_bondTopology->getInt64Component(index/2, 0) == bond.index1);
				if(_bondTopology->getInt64Component(index/2, 1) == bond.index2 && (!_bondPeriodicImages || _bondPeriodicImages->getVector3I(index/2) == bond.pbcShift))
					return index/2;
			}
			else {
				OVITO_ASSERT(_bondTopology->getInt64Component(index/2, 1) == bond.index1);
				if(_bondTopology->getInt64Component(index/2, 0) == bond.index2 && (!_bondPeriodicImages || _bondPeriodicImages->getVector3I(index/2) == -bond.pbcShift))
					return index/2;
			}
		}
		return _bondTopology->size();
	}

private:

	/// Returns the number of half bonds, which is used to indicate the end of the per-particle bond list.
	size_t endOfListValue() const { return _nextBond.size(); }

private:

	/// The bond property containing the bond definitions.
	const ConstPropertyPtr _bondTopology;

	/// The bond property containing PBC shift vectors.
	const ConstPropertyPtr _bondPeriodicImages;

	/// Contains the first bond index for each particle (the head of a linked list).
	std::vector<size_t> _startIndices;

	/// Stores the index of the next bond in the linked list.
	std::vector<size_t> _nextBond;
};

}	// End of namespace
}	// End of namespace


