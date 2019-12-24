////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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

#include <ovito/particles/Particles.h>
#include "ParticleBondMap.h"
#include "ParticlesObject.h"

namespace Ovito { namespace Particles {

/******************************************************************************
* Initializes the helper class.
******************************************************************************/
ParticleBondMap::ParticleBondMap(ConstPropertyPtr bondTopology, ConstPropertyPtr bondPeriodicImages) :
	_bondTopology(std::move(bondTopology)),
	_bondPeriodicImages(std::move(bondPeriodicImages)),
	_nextBond(_bondTopology->size()*2, _bondTopology->size()*2)
{
	for(size_t bondIndex = _bondTopology->size(); bondIndex-- != 0; ) {
		size_t index1 = _bondTopology->get<qlonglong>(bondIndex, 0);
		size_t index2 = _bondTopology->get<qlonglong>(bondIndex, 1);
		if(index1 >= _startIndices.size())
			_startIndices.resize(index1 + 1, endOfListValue());
		if(index2 >= _startIndices.size())
			_startIndices.resize(index2 + 1, endOfListValue());

		size_t evenIndex = bondIndex * 2;
		size_t oddIndex  = evenIndex + 1;
		_nextBond[evenIndex] = _startIndices[index1];
		_nextBond[oddIndex]  = _startIndices[index2];
		_startIndices[index1] = evenIndex;
		_startIndices[index2] = oddIndex;
	}
}

/******************************************************************************
* Initializes the helper class.
******************************************************************************/
ParticleBondMap::ParticleBondMap(const BondsObject& bonds) :
	ParticleBondMap(bonds.expectProperty(BondsObject::TopologyProperty)->storage(), bonds.getPropertyStorage(BondsObject::PeriodicImageProperty))
{
}

}	// End of namespace
}	// End of namespace
