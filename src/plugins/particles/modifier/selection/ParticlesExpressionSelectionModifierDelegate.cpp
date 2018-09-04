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

#include <plugins/particles/Particles.h>
#include <plugins/particles/util/ParticleExpressionEvaluator.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <plugins/particles/objects/BondsObject.h>
#include "ParticlesExpressionSelectionModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)

IMPLEMENT_OVITO_CLASS(ParticlesExpressionSelectionModifierDelegate);
IMPLEMENT_OVITO_CLASS(BondsExpressionSelectionModifierDelegate);

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool ParticlesExpressionSelectionModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.containsObject<ParticlesObject>();
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
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool BondsExpressionSelectionModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	if(const ParticlesObject* particles = input.getObject<ParticlesObject>())
		return particles->bonds() != nullptr;
	return false;
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
