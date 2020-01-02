////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include "WrapPeriodicImagesModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(WrapPeriodicImagesModifier);

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool WrapPeriodicImagesModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
void WrapPeriodicImagesModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	const SimulationCellObject* simCellObj = state.expectObject<SimulationCellObject>();
	std::array<bool, 3> pbc = simCellObj->pbcFlags();
	if(!pbc[0] && !pbc[1] && !pbc[2]) {
		state.setStatus(PipelineStatus(PipelineStatus::Warning, tr("No periodic boundary conditions are enabled for the simulation cell.")));
		return;
	}

	if(simCellObj->is2D())
		 throwException(tr("In the current program version, this modifier only supports three-dimensional simulation cells."));

	const AffineTransformation& simCell = simCellObj->cellMatrix();
	if(std::abs(simCell.determinant()) < FLOATTYPE_EPSILON)
		 throwException(tr("The simulation cell is degenerate."));
	AffineTransformation inverseSimCell = simCell.inverse();

	// Make a modifiable copy of the particles object.
	ParticlesObject* outputParticles = state.expectMutableObject<ParticlesObject>();
	outputParticles->verifyIntegrity();

	// Make a modifiable copy of the particle position property.
	PropertyAccess<Point3> posProperty = outputParticles->expectMutableProperty(ParticlesObject::PositionProperty);

	// Wrap bonds by adjusting their PBC shift vectors.
	if(outputParticles->bonds()) {
		if(ConstPropertyAccess<ParticleIndexPair> topologyProperty = outputParticles->bonds()->getProperty(BondsObject::TopologyProperty)) {
			outputParticles->makeBondsMutable();
			PropertyAccess<Vector3I> periodicImageProperty = outputParticles->bonds()->createProperty(BondsObject::PeriodicImageProperty, true);
			for(size_t bondIndex = 0; bondIndex < topologyProperty.size(); bondIndex++) {
				size_t particleIndex1 = topologyProperty[bondIndex][0];
				size_t particleIndex2 = topologyProperty[bondIndex][1];
				if(particleIndex1 >= posProperty.size() || particleIndex2 >= posProperty.size())
					continue;
				const Point3& p1 = posProperty[particleIndex1];
				const Point3& p2 = posProperty[particleIndex2];
				for(size_t dim = 0; dim < 3; dim++) {
					if(pbc[dim]) {
						periodicImageProperty[bondIndex][dim] +=
							  (int)std::floor(inverseSimCell.prodrow(p2, dim))
							- (int)std::floor(inverseSimCell.prodrow(p1, dim));
					}
				}
			}
		}
	}

	// Wrap particles coordinates.
	for(size_t dim = 0; dim < 3; dim++) {
		if(pbc[dim]) {
			for(Point3& p : posProperty) {
				if(FloatType n = std::floor(inverseSimCell.prodrow(p, dim)))
					p -= simCell.column(dim) * n;
			}
		}
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
