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

#include <plugins/particles/Particles.h>
#include <plugins/mesh/Mesh.h>

namespace Ovito {
	namespace Plugins {
		namespace CrystalAnalysis {
			
			using namespace Ovito::Particles;
			using namespace Ovito::Mesh;

			class StructurePattern;
			class BurgersVectorFamily;
			class PatternCatalog;
			class DislocationDisplay;
			class DislocationNetworkObject;
			class ClusterGraphObject;
			class ClusterGraph;
			class DislocationNetwork;
			class Microstructure;
			class PartitionMesh;
			class PartitionMeshDisplay;
			struct DislocationNode;
			struct DislocationSegment;
		}
	}
}

#endif
