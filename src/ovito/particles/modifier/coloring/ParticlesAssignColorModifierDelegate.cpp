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
#include "ParticlesAssignColorModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(ParticlesAssignColorModifierDelegate);
IMPLEMENT_OVITO_CLASS(ParticleVectorsAssignColorModifierDelegate);
IMPLEMENT_OVITO_CLASS(BondsAssignColorModifierDelegate);

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier 
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> ParticlesAssignColorModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const 
{
	if(input.containsObject<ParticlesObject>())
		return { DataObjectReference(&ParticlesObject::OOClass()) };
	return {};
}

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier 
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> ParticleVectorsAssignColorModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const 
{
    if(const ParticlesObject* particles = input.getObject<ParticlesObject>()) {
        for(const PropertyObject* property : particles->properties()) {
            if(property->visElement<VectorVis>() != nullptr)
                return { DataObjectReference(&ParticlesObject::OOClass()) };
        }
    }
	return {};
}

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier 
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> BondsAssignColorModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const 
{
    if(const ParticlesObject* particles = input.getObject<ParticlesObject>()) {
        if(particles->bonds())
       		return { DataObjectReference(&ParticlesObject::OOClass()) };
    }
    return {};
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
