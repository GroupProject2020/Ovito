////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Alexander Stukowski
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

//
// Standard precompiled header file included by all source files in this module
//

#ifndef __OVITO_PARTICLES_
#define __OVITO_PARTICLES_

#include <ovito/core/Core.h>
#include <ovito/mesh/Mesh.h>
#include <ovito/grid/Grid.h>
#include <ovito/stdobj/StdObj.h>

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
		using namespace Ovito::StdObj;

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
