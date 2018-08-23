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
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "WrapPeriodicImagesModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(WrapPeriodicImagesModifier);

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool WrapPeriodicImagesModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObjectOfType<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState WrapPeriodicImagesModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = input;
	ParticleInputHelper pih(dataset(), input);
	ParticleOutputHelper poh(dataset(), output, modApp);

	SimulationCellObject* simCellObj = pih.expectSimulationCell();
	std::array<bool, 3> pbc = simCellObj->pbcFlags();
	if(!pbc[0] && !pbc[1] && !pbc[2])
		return PipelineStatus(PipelineStatus::Warning, tr("No periodic boundary conditions are enabled for the simulation cell."));

	if(simCellObj->is2D())
		 throwException(tr("In the current program version, this modifier only supports three-dimensional simulation cells."));

	const AffineTransformation& simCell = simCellObj->cellMatrix();
	if(std::abs(simCell.determinant()) < FLOATTYPE_EPSILON)
		 throwException(tr("The simulation cell is degenerate."));
	AffineTransformation inverseSimCell = simCell.inverse();

	pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);
	ParticleProperty* posProperty = poh.outputStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty, true);

	// Wrap bonds by adjusting their PBC shift vectors.
	if(BondProperty* topologyProperty = BondProperty::findInState(poh.output(), BondProperty::TopologyProperty)) {
		BondProperty* periodicImageProperty = poh.outputStandardProperty<BondProperty>(BondProperty::PeriodicImageProperty, true);
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
						- (int)floor(inverseSimCell.prodrow(p1, dim))
						+ (int)floor(inverseSimCell.prodrow(p2, dim)));
				}
			}
		}
	}

	// Wrap particles coordinates
	for(size_t dim = 0; dim < 3; dim++) {
		if(pbc[dim]) {
			for(Point3& p : posProperty->point3Range()) {
				if(FloatType n = floor(inverseSimCell.prodrow(p, dim)))
					p -= simCell.column(dim) * n;
			}
		}
	}

	return output;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
