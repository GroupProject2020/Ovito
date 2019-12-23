////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>

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
				if(!boost::equal(prop->crange<qlonglong>(), _particleIdentifiers->crange<qlonglong>()))
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
