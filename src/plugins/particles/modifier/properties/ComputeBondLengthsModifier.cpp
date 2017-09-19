///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <core/dataset/data/simcell/SimulationCellObject.h>
#include "ComputeBondLengthsModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)

IMPLEMENT_OVITO_CLASS(ComputeBondLengthsModifier);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ComputeBondLengthsModifier::ComputeBondLengthsModifier(DataSet* dataset) : Modifier(dataset)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool ComputeBondLengthsModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<BondsObject>() != nullptr;
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState ComputeBondLengthsModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Inputs:
	ParticleInputHelper pih(dataset(), input);
	ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);
	BondsObject* bondsObj = pih.expectBonds();
	SimulationCellObject* simCell = input.findObject<SimulationCellObject>();
	AffineTransformation cellMatrix = simCell ? simCell->cellMatrix() : AffineTransformation::Identity();

	// Outputs:
	PipelineFlowState output = input;
	ParticleOutputHelper poh(dataset(), output);
	BondProperty* lengthProperty = poh.outputStandardProperty<BondProperty>(BondProperty::LengthProperty, false);

	// Perform bond length calculation.
	parallelFor(bondsObj->size(), [posProperty, bondsObj, simCell, &cellMatrix, lengthProperty](size_t bondIndex) {
		const Bond& bond = (*bondsObj->storage())[bondIndex];
		if(posProperty->size() > bond.index1 && posProperty->size() > bond.index2) {
			const Point3& p1 = posProperty->getPoint3(bond.index1);
			const Point3& p2 = posProperty->getPoint3(bond.index2);
			Vector3 delta = p2 - p1;
			if(simCell) {
				if(bond.pbcShift.x()) delta += cellMatrix.column(0) * (FloatType)bond.pbcShift.x();
				if(bond.pbcShift.y()) delta += cellMatrix.column(1) * (FloatType)bond.pbcShift.y();
				if(bond.pbcShift.z()) delta += cellMatrix.column(2) * (FloatType)bond.pbcShift.z();
			}
			lengthProperty->setFloat(bondIndex, delta.length());
		}
		else lengthProperty->setFloat(bondIndex, 0);
	});

	return output;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
