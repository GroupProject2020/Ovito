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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/VectorVis.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include "ParticlesColorCodingModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(ParticlesColorCodingModifierDelegate);
IMPLEMENT_OVITO_CLASS(ParticleVectorsColorCodingModifierDelegate);
IMPLEMENT_OVITO_CLASS(BondsColorCodingModifierDelegate);

/******************************************************************************
* Returns whether this function can be applied to the given input data.
******************************************************************************/
bool ParticleVectorsColorCodingModifierDelegate::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
    if(const ParticlesObject* particles = input.getObject<ParticlesObject>()) {
        for(const PropertyObject* property : particles->properties()) {
            if(property->visElement<VectorVis>() != nullptr)
                return true;
        }
    }
	return false;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
