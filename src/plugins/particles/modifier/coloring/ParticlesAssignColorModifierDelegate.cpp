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
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/objects/VectorDisplay.h>
#include <core/dataset/pipeline/OutputHelper.h>
#include "ParticlesAssignColorModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)





/******************************************************************************
* Creates the property object that will receive the computed colors.
******************************************************************************/
PropertyObject* ParticlesAssignColorModifierDelegate::createOutputColorProperty(TimePoint time, InputHelper& ih, OutputHelper& oh, bool initializeWithExistingColors)
{
    PropertyObject* colorProperty = oh.outputStandardProperty<ParticleProperty>(ParticleProperty::ColorProperty, false);
    if(initializeWithExistingColors) {
        ParticleInputHelper pih(dataset(), ih.input());
        const std::vector<Color> colors = pih.inputParticleColors(time, oh.output().mutableStateValidity());
        OVITO_ASSERT(colors.size() == colorProperty->size());
        std::copy(colors.cbegin(), colors.cend(), colorProperty->dataColor());
    }
    return colorProperty;
}

/******************************************************************************
* Returns whether this slice function can be applied to the given input data.
******************************************************************************/
bool ParticleVectorsAssignColorModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	for(DataObject* obj : input.objects()) {
        if(dynamic_object_cast<VectorDisplay>(obj->displayObject()))
            return true;
    }
    return false;
}
    
/******************************************************************************
* Creates the property object that will receive the computed colors.
******************************************************************************/
PropertyObject* ParticleVectorsAssignColorModifierDelegate::createOutputColorProperty(TimePoint time, InputHelper& ih, OutputHelper& oh, bool initializeWithExistingColors)
{
    PropertyObject* colorProperty = oh.outputStandardProperty<ParticleProperty>(ParticleProperty::VectorColorProperty, false);
    if(initializeWithExistingColors) {
        VectorDisplay* vectorDisplay = dynamic_object_cast<VectorDisplay>(colorProperty->displayObject());
        if(vectorDisplay) {
            std::fill(colorProperty->dataColor(), colorProperty->dataColor() + colorProperty->size(), vectorDisplay->arrowColor());
        }
    }
    return colorProperty;
}

/******************************************************************************
* Creates the property object that will receive the computed colors.
******************************************************************************/
PropertyObject* BondsAssignColorModifierDelegate::createOutputColorProperty(TimePoint time, InputHelper& ih, OutputHelper& oh, bool initializeWithExistingColors)
{
    PropertyObject* colorProperty = oh.outputStandardProperty<BondProperty>(BondProperty::ColorProperty, false);
    if(initializeWithExistingColors) {
        ParticleInputHelper pih(dataset(), ih.input());
        const std::vector<Color> colors = pih.inputBondColors(time, oh.output().mutableStateValidity());
        OVITO_ASSERT(colors.size() == colorProperty->size());
        std::copy(colors.cbegin(), colors.cend(), colorProperty->dataColor());
    }
    return colorProperty;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
