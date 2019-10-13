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
#include <ovito/particles/util/ParticleExpressionEvaluator.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/BondsObject.h>
#include "ParticlesExpressionSelectionModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)

IMPLEMENT_OVITO_CLASS(ParticlesExpressionSelectionModifierDelegate);
IMPLEMENT_OVITO_CLASS(BondsExpressionSelectionModifierDelegate);

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> ParticlesExpressionSelectionModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
	if(input.containsObject<ParticlesObject>())
		return { DataObjectReference(&ParticlesObject::OOClass()) };
	return {};
}


/******************************************************************************
* Looks up the container for the properties in the input pipeline state.
******************************************************************************/
PropertyContainer* ParticlesExpressionSelectionModifierDelegate::getOutputPropertyContainer(PipelineFlowState& outputState) const
{
	return outputState.expectMutableObject<ParticlesObject>();
}

/******************************************************************************
* Creates and initializes the expression evaluator object.
******************************************************************************/
std::unique_ptr<PropertyExpressionEvaluator> ParticlesExpressionSelectionModifierDelegate::initializeExpressionEvaluator(const QStringList& expressions, const PipelineFlowState& inputState, int animationFrame)
{
	std::unique_ptr<ParticleExpressionEvaluator> evaluator = std::make_unique<ParticleExpressionEvaluator>();
	evaluator->initialize(expressions, inputState, animationFrame);
	return evaluator;
}

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> BondsExpressionSelectionModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
    if(const ParticlesObject* particles = input.getObject<ParticlesObject>()) {
        if(particles->bonds())
       		return { DataObjectReference(&ParticlesObject::OOClass()) };
    }
    return {};
}

/******************************************************************************
* Looks up the container for the properties in the output pipeline state.
******************************************************************************/
PropertyContainer* BondsExpressionSelectionModifierDelegate::getOutputPropertyContainer(PipelineFlowState& outputState) const
{
	ParticlesObject* particles = outputState.expectMutableObject<ParticlesObject>();
	particles->expectBonds();
	return particles->makeBondsMutable();
}

/******************************************************************************
* Creates and initializes the expression evaluator object.
******************************************************************************/
std::unique_ptr<PropertyExpressionEvaluator> BondsExpressionSelectionModifierDelegate::initializeExpressionEvaluator(const QStringList& expressions, const PipelineFlowState& inputState, int animationFrame)
{
	std::unique_ptr<BondExpressionEvaluator> evaluator = std::make_unique<BondExpressionEvaluator>();
	evaluator->initialize(expressions, inputState, animationFrame);
	return evaluator;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
