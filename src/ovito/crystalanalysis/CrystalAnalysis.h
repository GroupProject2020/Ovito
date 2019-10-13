////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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
