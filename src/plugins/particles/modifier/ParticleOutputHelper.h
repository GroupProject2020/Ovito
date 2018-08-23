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
#include <plugins/stdobj/util/OutputHelper.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/objects/BondProperty.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers)

/**
 * \brief Helper class that allows easy manipulation of particles and bonds.
 */
class OVITO_PARTICLES_EXPORT ParticleOutputHelper : public OutputHelper
{
public:

	/// Constructor.
	ParticleOutputHelper(DataSet* dataset, PipelineFlowState& output, PipelineObject* dataSource);

	/// Returns the number of particles in the output.
	size_t outputParticleCount() const { return _outputParticleCount; }

	/// Sets the number of particles in the output.
	void setOutputParticleCount(size_t count) { _outputParticleCount = count; }

	/// Returns the number of bonds in the output.
	size_t outputBondCount() const { return _outputBondCount; }

	/// Sets the number of bonds in the output.
	void setOutputBondCount(size_t count) { _outputBondCount = count; }

	/// Deletes the particles for which bits are set in the given bit-mask.
	/// Returns the number of deleted particles.
	size_t deleteParticles(const boost::dynamic_bitset<>& mask);
	
	/// Deletes the bonds for which bits are set in the given bit-mask.
	/// Returns the number of deleted bonds.
	size_t deleteBonds(const boost::dynamic_bitset<>& mask);
	
	/// Adds a set of new bonds to the system.
	void addBonds(const std::vector<Bond>& newBonds, BondsVis* bondsVis, const std::vector<PropertyPtr>& bondProperties = {});

protected:

	/// The number of particles in the output.
	size_t _outputParticleCount;

	/// The number of bonds in the output.
	size_t _outputBondCount;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
