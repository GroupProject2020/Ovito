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

		class ParticleFrameData;
		class ParticleImporter;
		class InputColumnMapping;
		
		class NearestNeighborFinder;
		class CutoffNeighborFinder;
	}
}

#endif
