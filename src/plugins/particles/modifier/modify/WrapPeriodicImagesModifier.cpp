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

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
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

	// Make a modifiable copy of the particle position property.
	PropertyPtr posProperty = outputParticles->expectMutableProperty(ParticlesObject::PositionProperty)->modifiableStorage();

	// Wrap bonds by adjusting their PBC shift vectors.
	if(outputParticles->bonds()) {
		if(ConstPropertyPtr topologyProperty = outputParticles->bonds()->getPropertyStorage(BondsObject::TopologyProperty)) {
			outputParticles->makeBondsMutable();
			PropertyObject* periodicImageProperty = outputParticles->bonds()->createProperty(BondsObject::PeriodicImageProperty, true);
			for(size_t bondIndex = 0; bondIndex < topologyProperty->size(); bondIndex++) {
				size_t particleIndex1 = topologyProperty->getInt64Component(bondIndex, 0);
				size_t particleIndex2 = topologyProperty->getInt64Component(bondIndex, 1);
				if(particleIndex1 >= posProperty->size() || particleIndex2 >= posProperty->size())
					continue;
				const Point3& p1 = posProperty->getPoint3(particleIndex1);
				const Point3& p2 = posProperty->getPoint3(particleIndex2);
				for(size_t dim = 0; dim < 3; dim++) {
					if(pbc[dim]) {
						periodicImageProperty->setIntComponent(bondIndex, dim, periodicImageProperty->getIntComponent(bondIndex, dim)
							- (int)std::floor(inverseSimCell.prodrow(p1, dim))
							+ (int)std::floor(inverseSimCell.prodrow(p2, dim)));
					}
				}
			}
		}
	}

	// Wrap particles coordinates.
	for(size_t dim = 0; dim < 3; dim++) {
		if(pbc[dim]) {
			for(Point3& p : posProperty->point3Range()) {
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
