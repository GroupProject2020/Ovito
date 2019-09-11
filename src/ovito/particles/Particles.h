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

//
// Standard precompiled header file included by all source files in this module
//

#ifndef __OVITO_PARTICLES_
#define __OVITO_PARTICLES_

#include <ovito/core/Core.h>
#include <ovito/mesh/Mesh.h>
#include <ovito/grid/Grid.h>

/*! \namespace Ovito::Particles
    \brief This root namespace of the particles plugin.
*/
/*! \namespace Ovito::Particles::Import
    \brief This namespace contains basic classes for importing particle data.
*/
/*! \namespace Ovito::Particles::Import::Formats
    \brief This namespace contains particle data importers for various file formats.
*/
/*! \namespace Ovito::Particles::Export
    \brief This namespace contains basic classes for exporting particle data.
*/
/*! \namespace Ovito::Particles::Export::Formats
    \brief This namespace contains particle data exporters for various file formats.
*/
/*! \namespace Ovito::Particles::Modifiers
    \brief This namespace contains modifiers for particle data.
*/
/*! \namespace Ovito::Particles::Modifiers::Analysis
    \brief This namespace contains analysis modifiers for particle systems.
*/
/*! \namespace Ovito::Particles::Modifiers::Coloring
    \brief This namespace contains color-related modifiers for particle systems.
*/
/*! \namespace Ovito::Particles::Modifiers::Modify
    \brief This namespace contains modifiers for particle systems.
*/
/*! \namespace Ovito::Particles::Modifiers::Properties
    \brief This namespace contains modifiers that modify particle properties.
*/
/*! \namespace Ovito::Particles::Modifiers::Selection
    \brief This namespace contains modifiers that select particles.
*/
/*! \namespace Ovito::Particles::Util
    \brief This namespace contains particle-related utility classes.
*/

namespace Ovito {
	namespace Particles {

		using namespace Ovito::Mesh;
		using namespace Ovito::Grid;

		class ParticleType;
		class ParticlesObject;
		class BondType;
		class BondsObject;
		class ParticlesVis;
		class BondsVis;
		class VectorVis;
		class ParticleBondMap;
		class TrajectoryObject;

		OVITO_BEGIN_INLINE_NAMESPACE(Import)
			class ParticleFrameData;
			class ParticleImporter;
			class InputColumnMapping;
		OVITO_END_INLINE_NAMESPACE

		OVITO_BEGIN_INLINE_NAMESPACE(Export)
		OVITO_END_INLINE_NAMESPACE

		OVITO_BEGIN_INLINE_NAMESPACE(Util)
			class NearestNeighborFinder;
			class CutoffNeighborFinder;
		OVITO_END_INLINE_NAMESPACE
	}
}

#endif
