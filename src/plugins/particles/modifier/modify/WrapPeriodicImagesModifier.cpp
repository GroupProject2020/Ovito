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
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <core/dataset/DataSet.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include "WrapPeriodicImagesModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(WrapPeriodicImagesModifier);

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool WrapPeriodicImagesModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState WrapPeriodicImagesModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = input;
	ParticleInputHelper pih(dataset(), input);
	ParticleOutputHelper poh(dataset(), output);

	SimulationCellObject* simCellObj = pih.expectSimulationCell();
	std::array<bool, 3> pbc = simCellObj->pbcFlags();
	if(!pbc[0] && !pbc[1] && !pbc[2])
		return PipelineStatus(PipelineStatus::Warning, tr("The simulation cell has no periodic boundary conditions."));

	if(simCellObj->is2D())
		 throwException(tr("In the current program version this modifier only supports three-dimensional simulation cells."));

	AffineTransformation simCell = simCellObj->cellMatrix();
	if(std::abs(simCell.determinant()) < FLOATTYPE_EPSILON)
		 throwException(tr("The simulation cell is degenerated."));
	AffineTransformation inverseSimCell = simCell.inverse();

	pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);
	ParticleProperty* posProperty = poh.outputStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty, true);

	// Wrap bonds
	for(DataObject* obj : output.objects()) {
		if(BondsObject* bondsObj = dynamic_object_cast<BondsObject>(obj)) {
			bondsObj = poh.cloneIfNeeded(bondsObj);

			// Wrap bonds by adjusting their shift vectors.
			for(Bond& bond : *bondsObj->modifiableStorage()) {
				if(bond.index1 >= posProperty->size() || bond.index2 >= posProperty->size())
					continue;
				const Point3& p1 = posProperty->getPoint3(bond.index1);
				const Point3& p2 = posProperty->getPoint3(bond.index2);
				for(size_t dim = 0; dim < 3; dim++) {
					if(pbc[dim]) {
						bond.pbcShift[dim] -= (int8_t)floor(inverseSimCell.prodrow(p1, dim));
						bond.pbcShift[dim] += (int8_t)floor(inverseSimCell.prodrow(p2, dim));
					}
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
