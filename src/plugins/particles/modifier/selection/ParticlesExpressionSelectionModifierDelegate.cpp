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
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/stdobj/util/OutputHelper.h>
#include "ParticlesExpressionSelectionModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)

IMPLEMENT_OVITO_CLASS(ParticlesExpressionSelectionModifierDelegate);
IMPLEMENT_OVITO_CLASS(BondsExpressionSelectionModifierDelegate);

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool ParticlesExpressionSelectionModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObjectOfType<ParticleProperty>() != nullptr;
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
* Creates the output selection property object.
******************************************************************************/
PropertyObject* ParticlesExpressionSelectionModifierDelegate::createOutputSelectionProperty(OutputHelper& oh)
{
	return oh.outputStandardProperty<ParticleProperty>(ParticleProperty::SelectionProperty);
}

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool BondsExpressionSelectionModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObjectOfType<BondProperty>() != nullptr;
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

/******************************************************************************
* Creates the output selection property object.
******************************************************************************/
PropertyObject* BondsExpressionSelectionModifierDelegate::createOutputSelectionProperty(OutputHelper& oh)
{
	return oh.outputStandardProperty<BondProperty>(BondProperty::SelectionProperty);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
