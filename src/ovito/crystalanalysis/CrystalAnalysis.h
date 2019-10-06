///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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

#ifndef __OVITO_CRYSTALANALYSIS_
#define __OVITO_CRYSTALANALYSIS_

#include <ovito/particles/Particles.h>
#include <ovito/mesh/Mesh.h>
#include <ovito/stdobj/StdObj.h>
#include <ovito/grid/Grid.h>

namespace Ovito {
	namespace CrystalAnalysis {

		using namespace Ovito::Particles;
		using namespace Ovito::Mesh;
		using namespace Ovito::StdObj;
		using namespace Ovito::Grid;

		class MicrostructurePhase;
		class BurgersVectorFamily;
		class DislocationVis;
		class DislocationNetworkObject;
		class ClusterGraphObject;
		class ClusterGraph;
		class DislocationNetwork;
		struct DislocationNode;
		struct DislocationSegment;
		class Microstructure;
		class MicrostructureData;
	}
}

#endif
