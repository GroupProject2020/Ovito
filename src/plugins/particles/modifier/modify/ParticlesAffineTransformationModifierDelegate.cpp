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
#include <core/dataset/DataSet.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include "ParticlesAffineTransformationModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(ParticlesAffineTransformationModifierDelegate);
IMPLEMENT_OVITO_CLASS(VectorParticlePropertiesAffineTransformationModifierDelegate);

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool ParticlesAffineTransformationModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus ParticlesAffineTransformationModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp)
{
	AffineTransformationModifier* mod = static_object_cast<AffineTransformationModifier>(modifier);

	ParticleInputHelper pih(dataset(), input);
	ParticleOutputHelper poh(dataset(), output);
	
	if(!pih.inputStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty))
		return PipelineStatus::Success;

	ParticleProperty* posProperty = poh.outputStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty, true);

	AffineTransformation tm;
	if(mod->relativeMode())
		tm = mod->transformationTM();
	else
		tm = mod->targetCell() * pih.expectSimulationCell()->cellMatrix().inverse();
	
	if(mod->selectionOnly()) {
		ParticleProperty* selProperty = pih.inputStandardProperty<ParticleProperty>(ParticleProperty::SelectionProperty);
		if(selProperty) {
			const int* s = selProperty->constDataInt();
			for(Point3& p : posProperty->point3Range()) {
				if(*s++)
					p = tm * p;
			}
		}
	}
	else {
		// Check if the matrix describes a pure translation. If yes, we can
		// simply add vectors instead of computing full matrix products.
		Vector3 translation = tm.translation();
		if(tm == AffineTransformation::translation(translation)) {
			for(Point3& p : posProperty->point3Range())
				p += translation;
		}
		else {
			for(Point3& p : posProperty->point3Range())
				p = tm * p;
		}
	}
	
	return PipelineStatus::Success;
}

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool VectorParticlePropertiesAffineTransformationModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	for(DataObject* obj : input.objects()) {
		if(ParticleProperty* property = dynamic_object_cast<ParticleProperty>(obj)) {
			if(isTransformableProperty(property))
				return true;
		}
	}
	return false;
}

/******************************************************************************
* Decides if the given particle property is one that should be transformed.
******************************************************************************/
bool VectorParticlePropertiesAffineTransformationModifierDelegate::isTransformableProperty(ParticleProperty* property)
{
	return property->type() == ParticleProperty::VelocityProperty ||
		property->type() == ParticleProperty::ForceProperty ||
		property->type() == ParticleProperty::DisplacementProperty;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus VectorParticlePropertiesAffineTransformationModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp)
{
	AffineTransformationModifier* mod = static_object_cast<AffineTransformationModifier>(modifier);

	ParticleInputHelper pih(dataset(), input);
	ParticleOutputHelper poh(dataset(), output);
	
	AffineTransformation tm;
	if(mod->relativeMode())
		tm = mod->transformationTM();
	else
		tm = mod->targetCell() * pih.expectSimulationCell()->cellMatrix().inverse();
	
	for(DataObject* obj : output.objects()) {
		if(ParticleProperty* inputProperty = dynamic_object_cast<ParticleProperty>(obj)) {
			if(isTransformableProperty(inputProperty)) {

				PropertyStorage* property = poh.cloneIfNeeded(inputProperty)->modifiableStorage().get();
				OVITO_ASSERT(property->dataType() == qMetaTypeId<FloatType>());
				OVITO_ASSERT(property->componentCount() == 3);
				if(!mod->selectionOnly()) {
					for(Vector3& v : property->vector3Range())
						v = tm * v;
				}
				else {
					ParticleProperty* selProperty = pih.inputStandardProperty<ParticleProperty>(ParticleProperty::SelectionProperty);
					if(selProperty) {
						const int* s = selProperty->constDataInt();
						for(Vector3& v : property->vector3Range()) {
							if(*s++)
								v = tm * v;
						}
					}
				}
			}
		}
	}
	
	return PipelineStatus::Success;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
