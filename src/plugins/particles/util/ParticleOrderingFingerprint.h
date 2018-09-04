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
#include <plugins/particles/objects/ParticlesObject.h>
#include <core/dataset/pipeline/PipelineFlowState.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util)

/**
 * \brief Helper class used to detect changes in the storage ordering of particles.
 * 
 * Modifiers can use this class to detect if the storage ordering or the number of of input particles
 * have changed, rendering any previously computed results invalid. 
 */
class OVITO_PARTICLES_EXPORT ParticleOrderingFingerprint
{
public:

	/// Constructor.
	ParticleOrderingFingerprint(const ParticlesObject* particles) : 
		_particleCount(particles->elementCount()),
		_particleIdentifiers(particles->getPropertyStorage(ParticlesObject::IdentifierProperty)) {}

	/// Returns the number of particles for which this object was constructed.
	size_t particleCount() const { return _particleCount; }

	/// Returns true if the particle number and the storage order have changed 
	/// with respect to the state from which this object was constructed.
	bool hasChanged(const ParticlesObject* particles) const {
		if(_particleCount != particles->elementCount()) 
			return true;
		if(const PropertyObject* prop = particles->getProperty(ParticlesObject::IdentifierProperty)) {
			if(!_particleIdentifiers) 
				return true;
			if(prop->storage() != _particleIdentifiers) { 
				if(!std::equal(prop->constDataInt64(), prop->constDataInt64() + prop->size(),
						_particleIdentifiers->constDataInt64(), _particleIdentifiers->constDataInt64() + _particleIdentifiers->size()))
					return true;
			}
		}
		else if(_particleIdentifiers) {
			return true;
		}
		return false;
	}

private:

	/// The total number of particles.
	size_t _particleCount;

	/// The list of particle IDs (if available).
	ConstPropertyPtr _particleIdentifiers;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
