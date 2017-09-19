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
#include <plugins/particles/util/ParticleExpressionEvaluator.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/viewport/Viewport.h>
#include <core/dataset/scene/ObjectNode.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "ExpressionSelectionModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)


DEFINE_PROPERTY_FIELD(ExpressionSelectionModifier, expression, "Expression");
SET_PROPERTY_FIELD_LABEL(ExpressionSelectionModifier, expression, "Boolean expression");

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool ExpressionSelectionModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState ExpressionSelectionModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = input;
	ParticleOutputHelper poh(dataset(), output);
	
	// The current animation frame number.
	int currentFrame = dataset()->animationSettings()->timeToFrame(time);

	// Initialize the evaluator class.
	ParticleExpressionEvaluator evaluator;
	evaluator.initialize(QStringList(expression()), input, currentFrame);

	// Save list of available input variables, which will be displayed in the modifier's UI.
	_variableNames = evaluator.inputVariableNames();
	_variableTable = evaluator.inputVariableTable();

	// If the user has not yet entered an expression let him know which
	// data channels can be used in the expression.
	if(expression().isEmpty())
		return PipelineStatus(PipelineStatus::Warning, tr("Please enter a boolean expression."));

	// Check if expression contain an assignment ('=' operator).
	// This should be considered an error, because the user is probably referring the comparison operator '=='.
	if(expression().contains(QRegExp("[^=!><]=(?!=)")))
		throwException("The expression contains the assignment operator '='. Please use the comparison operator '==' instead.");

	// The number of selected particles.
	std::atomic_size_t nselected(0);

	// Get the deep copy of the output selection property.
	const PropertyPtr& selProperty = poh.outputStandardProperty<ParticleProperty>(ParticleProperty::SelectionProperty)->modifiableStorage();

	if(poh.outputParticleCount() != 0) {
		evaluator.evaluate([&selProperty, &nselected](size_t particleIndex, size_t componentIndex, double value) {
			if(value) {
				selProperty->setInt(particleIndex, 1);
				++nselected;
			}
			else {
				selProperty->setInt(particleIndex, 0);
			}
		});
	}

	if(evaluator.isTimeDependent())
		output.intersectStateValidity(time);

	QString statusMessage = tr("%1 out of %2 particles selected (%3%)").arg(nselected.load()).arg(poh.outputParticleCount()).arg((FloatType)nselected.load() * 100 / std::max(1,(int)poh.outputParticleCount()), 0, 'f', 1);
	output.setStatus(PipelineStatus(PipelineStatus::Success, std::move(statusMessage)));

	output.attributes().insert(QStringLiteral("SelectExpression.num_selected"), QVariant::fromValue(nselected.load()));

	return output;
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void ExpressionSelectionModifier::initializeModifier(ModifierApplication* modApp)
{
	Modifier::initializeModifier(modApp);

	// Build list of available input variables.
	PipelineFlowState input = modApp->evaluateInputPreliminary();
	ParticleExpressionEvaluator evaluator;
	evaluator.initialize(QStringList(), input);
	_variableNames = evaluator.inputVariableNames();
	_variableTable = evaluator.inputVariableTable();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
