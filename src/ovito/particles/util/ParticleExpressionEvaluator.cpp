////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
				return cellData.inverseMatrix().prodrow(posProperty->get<Point3>(particleIndex), 0);
			});
			registerComputedVariable("ReducedPosition.Y", [posProperty,cellData](size_t particleIndex) -> double {
				return cellData.inverseMatrix().prodrow(posProperty->get<Point3>(particleIndex), 1);
			});
			registerComputedVariable("ReducedPosition.Z", [posProperty,cellData](size_t particleIndex) -> double {
				return cellData.inverseMatrix().prodrow(posProperty->get<Point3>(particleIndex), 2);
			});
		}
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
