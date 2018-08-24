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
#include <plugins/stdobj/util/InputHelper.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/objects/BondProperty.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers)

/**
 * \brief Helper class that allows easy access to particles and bonds.
 */
class OVITO_PARTICLES_EXPORT ParticleInputHelper : public InputHelper
{
public:

	/// Constructor.
	ParticleInputHelper(DataSet* dataset, const PipelineFlowState& input);

	/// Throws an exception if the input does not contain any particle data.
	/// Returns the ParticlesObject.
	ParticlesObject* expectParticles() const;

	/// Throws an exception if the input does not contain any bonds.
	/// Returns the bond topology property.
	BondProperty* expectBonds() const;

	/// Returns the number of particles in the input.
	size_t inputParticleCount() const { return _inputParticleCount; }

	/// Returns the number of bonds in the input.
	size_t inputBondCount() const { return _inputBondCount; }

	/// Returns a vector with the input particles colors.
	std::vector<Color> inputParticleColors(TimePoint time, TimeInterval& validityInterval);

	/// Returns a vector with the input particles radii.
	std::vector<FloatType> inputParticleRadii(TimePoint time, TimeInterval& validityInterval);

	/// Returns a vector with the input bond colors.
	std::vector<Color> inputBondColors(TimePoint time, TimeInterval& validityInterval);

protected:

	/// The number of particles in the input.
	size_t _inputParticleCount;

	/// The number of bonds in the input.
	size_t _inputBondCount;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


