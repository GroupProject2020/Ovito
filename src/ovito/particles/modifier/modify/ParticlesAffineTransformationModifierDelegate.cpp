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
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "ParticlesAffineTransformationModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(ParticlesAffineTransformationModifierDelegate);
IMPLEMENT_OVITO_CLASS(VectorParticlePropertiesAffineTransformationModifierDelegate);

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool ParticlesAffineTransformationModifierDelegate::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus ParticlesAffineTransformationModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	if(const ParticlesObject* inputParticles = state.getObject<ParticlesObject>()) {

		// Make sure we can safely modify the particles object.
		ParticlesObject* outputParticles = state.makeMutable(inputParticles);

		// Create a modifiable copy of the particle position.
		PropertyObject* posProperty = outputParticles->createProperty(ParticlesObject::PositionProperty, true);

		// Determine transformation matrix.
		AffineTransformationModifier* mod = static_object_cast<AffineTransformationModifier>(modifier);
		AffineTransformation tm;
		if(mod->relativeMode())
			tm = mod->transformationTM();
		else
			tm = mod->targetCell() * state.expectObject<SimulationCellObject>()->cellMatrix().inverse();

		if(mod->selectionOnly()) {
			if(const PropertyObject* selProperty = inputParticles->getProperty(ParticlesObject::SelectionProperty)) {
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
	}

	return PipelineStatus::Success;
}

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool VectorParticlePropertiesAffineTransformationModifierDelegate::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	if(const ParticlesObject* particles = input.getObject<ParticlesObject>()) {
		for(const PropertyObject* property : particles->properties()) {
			if(isTransformableProperty(property))
				return true;
		}
	}
	return false;
}

/******************************************************************************
* Decides if the given particle property is one that should be transformed.
******************************************************************************/
bool VectorParticlePropertiesAffineTransformationModifierDelegate::isTransformableProperty(const PropertyObject* property)
{
	return property->type() == ParticlesObject::VelocityProperty ||
		property->type() == ParticlesObject::ForceProperty ||
		property->type() == ParticlesObject::DisplacementProperty;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus VectorParticlePropertiesAffineTransformationModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	// Determine transformation matrix.
	AffineTransformationModifier* mod = static_object_cast<AffineTransformationModifier>(modifier);
	AffineTransformation tm;
	if(mod->relativeMode())
		tm = mod->transformationTM();
	else
		tm = mod->targetCell() * state.expectObject<SimulationCellObject>()->cellMatrix().inverse();

	if(const ParticlesObject* inputParticles = state.getObject<ParticlesObject>()) {

		for(const PropertyObject* inputProperty : inputParticles->properties()) {
			if(isTransformableProperty(inputProperty)) {

				// Make sure we can safely modify the particles object.
				ParticlesObject* outputParticles = state.expectMutableObject<ParticlesObject>();

				PropertyStorage* property = outputParticles->makeMutable(inputProperty)->modifiableStorage().get();
				OVITO_ASSERT(property->dataType() == PropertyStorage::Float);
				OVITO_ASSERT(property->componentCount() == 3);
				if(!mod->selectionOnly()) {
					for(Vector3& v : property->vector3Range())
						v = tm * v;
				}
				else {
					if(const PropertyObject* selProperty = inputParticles->getProperty(ParticlesObject::SelectionProperty)) {
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
