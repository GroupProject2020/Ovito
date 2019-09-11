///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <ovito/particles/Particles.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include "ParticleExpressionEvaluator.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Initializes the list of input variables from the given input state.
******************************************************************************/
void ParticleExpressionEvaluator::createInputVariables(const std::vector<ConstPropertyPtr>& inputProperties, const SimulationCell* simCell, const QVariantMap& attributes, int animationFrame)
{
	PropertyExpressionEvaluator::createInputVariables(inputProperties, simCell, attributes, animationFrame);

	// Create computed variables for reduced particle coordinates.
	if(simCell) {
		auto iter = std::find_if(inputProperties.begin(), inputProperties.end(), [](const ConstPropertyPtr& property) {
			return property->type() == ParticlesObject::PositionProperty;
		});
		if(iter != inputProperties.end()) {
			ConstPropertyPtr posProperty = *iter;
			SimulationCell cellData = *simCell;
			registerComputedVariable("ReducedPosition.X", [posProperty,cellData](size_t particleIndex) -> double {
				return cellData.inverseMatrix().prodrow(posProperty->getPoint3(particleIndex), 0);
			});
			registerComputedVariable("ReducedPosition.Y", [posProperty,cellData](size_t particleIndex) -> double {
				return cellData.inverseMatrix().prodrow(posProperty->getPoint3(particleIndex), 1);
			});
			registerComputedVariable("ReducedPosition.Z", [posProperty,cellData](size_t particleIndex) -> double {
				return cellData.inverseMatrix().prodrow(posProperty->getPoint3(particleIndex), 2);
			});
		}
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
