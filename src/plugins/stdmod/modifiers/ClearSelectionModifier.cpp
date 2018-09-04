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

#include <plugins/stdmod/StdMod.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include <plugins/stdobj/properties/PropertyContainer.h>
#include "ClearSelectionModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ClearSelectionModifier);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ClearSelectionModifier::ClearSelectionModifier(DataSet* dataset) : GenericPropertyModifier(dataset)
{
	// Operate on particles by default.
	setDefaultSubject(QStringLiteral("Particles"), QStringLiteral("ParticlesObject"));	
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState ClearSelectionModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	if(!subject())
		throwException(tr("No input element type selected."));
	
	PipelineFlowState output = input;

   	PropertyContainer* container = output.expectMutableLeafObject(subject());
	if(const PropertyObject* selProperty = container->getProperty(PropertyStorage::GenericSelectionProperty))
		container->removeProperty(selProperty);
    
	return output;
}

}	// End of namespace
}	// End of namespace
