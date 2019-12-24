////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "ParticlesAffineTransformationModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(ParticlesAffineTransformationModifierDelegate);
IMPLEMENT_OVITO_CLASS(VectorParticlePropertiesAffineTransformationModifierDelegate);

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> ParticlesAffineTransformationModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
	if(input.containsObject<ParticlesObject>())
		return { DataObjectReference(&ParticlesObject::OOClass()) };
	return {};
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
		PropertyObject* posProperty = outputParticles->expectMutableProperty(ParticlesObject::PositionProperty);

		// Determine transformation matrix.
		AffineTransformationModifier* mod = static_object_cast<AffineTransformationModifier>(modifier);
		AffineTransformation tm;
		if(mod->relativeMode())
			tm = mod->transformationTM();
		else
			tm = mod->targetCell() * state.expectObject<SimulationCellObject>()->cellMatrix().inverse();

		if(mod->selectionOnly()) {
			if(const PropertyObject* selProperty = inputParticles->getProperty(ParticlesObject::SelectionProperty)) {
				const int* s = selProperty->cdata<int>();
				for(Point3& p : posProperty->range<Point3>()) {
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
				for(Point3& p : posProperty->range<Point3>())
					p += translation;
			}
			else {
				for(Point3& p : posProperty->range<Point3>())
					p = tm * p;
			}
		}
	}

	return PipelineStatus::Success;
}

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> VectorParticlePropertiesAffineTransformationModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
	if(const ParticlesObject* particles = input.getObject<ParticlesObject>()) {
		for(const PropertyObject* property : particles->properties()) {
			if(isTransformableProperty(property))
				return { DataObjectReference(&ParticlesObject::OOClass()) };
		}
	}
	return {};
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
					for(Vector3& v : property->range<Vector3>())
						v = tm * v;
				}
				else {
					if(const PropertyObject* selProperty = inputParticles->getProperty(ParticlesObject::SelectionProperty)) {
						const int* s = selProperty->cdata<int>();
						for(Vector3& v : property->range<Vector3>()) {
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
