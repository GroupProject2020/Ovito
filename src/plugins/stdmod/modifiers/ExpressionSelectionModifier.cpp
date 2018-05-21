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

#include <plugins/stdmod/StdMod.h>
#include <plugins/stdobj/properties/PropertyExpressionEvaluator.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include <plugins/stdobj/util/OutputHelper.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/viewport/Viewport.h>
#include <core/dataset/scene/PipelineSceneNode.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "ExpressionSelectionModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ExpressionSelectionModifier);
DEFINE_PROPERTY_FIELD(ExpressionSelectionModifier, expression);
SET_PROPERTY_FIELD_LABEL(ExpressionSelectionModifier, expression, "Boolean expression");

IMPLEMENT_OVITO_CLASS(ExpressionSelectionModifierDelegate);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ExpressionSelectionModifier::ExpressionSelectionModifier(DataSet* dataset) : DelegatingModifier(dataset)
{
	// Let this modifier operate on particles by default.
	createDefaultModifierDelegate(ExpressionSelectionModifierDelegate::OOClass(), QStringLiteral("ParticlesExpressionSelectionModifierDelegate"));
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus ExpressionSelectionModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp)
{
	ExpressionSelectionModifier* expressionMod = static_object_cast<ExpressionSelectionModifier>(modifier);

	// The current animation frame number.
	int currentFrame = dataset()->animationSettings()->timeToFrame(time);

	// Initialize the evaluator class.
	std::unique_ptr<PropertyExpressionEvaluator> evaluator = initializeExpressionEvaluator(QStringList(expressionMod->expression()), input, currentFrame);

	// Save list of available input variables, which will be displayed in the modifier's UI.
	expressionMod->setVariablesInfo(evaluator->inputVariableNames(), evaluator->inputVariableTable());

	// If the user has not yet entered an expression let him know which
	// data channels can be used in the expression.
	if(expressionMod->expression().isEmpty())
		return PipelineStatus(PipelineStatus::Warning, tr("Please enter a Boolean expression."));

	// Check if expression contain an assignment ('=' operator).
	// This should be considered an error, because the user is probably referring the comparison operator '=='.
	if(expressionMod->expression().contains(QRegExp("[^=!><]=(?!=)")))
		throwException("The expression contains the assignment operator '='. Please use the comparison operator '==' instead.");

	// The number of selected elements.
	std::atomic_size_t nselected(0);

	// Generate the output selection property.
	OutputHelper oh(dataset(), output);
	const PropertyPtr& selProperty = createOutputSelectionProperty(oh)->modifiableStorage();

	// Evaluate Boolean expression for every input data element.
	evaluator->evaluate([&selProperty, &nselected](size_t elementIndex, size_t componentIndex, double value) {
		if(value) {
			selProperty->setInt(elementIndex, 1);
			++nselected;
		}
		else {
			selProperty->setInt(elementIndex, 0);
		}
	});

	// If the expression contains a time-dependent term, then we have to restrict the validity interval
	// of the generated selection to the current animation time.
	if(evaluator->isTimeDependent())
		output.intersectStateValidity(time);

	// Report the total number of selected elements as a pipeline attribute.
	oh.outputAttribute(QStringLiteral("SelectExpression.num_selected"), QVariant::fromValue(nselected.load()));

	// Update status display in the UI.
	QString statusMessage = tr("%1 out of %2 elements selected (%3%)").arg(nselected.load()).arg(selProperty->size()).arg((FloatType)nselected.load() * 100 / std::max((size_t)1,selProperty->size()), 0, 'f', 1);
	return PipelineStatus(PipelineStatus::Success, std::move(statusMessage));
}

}	// End of namespace
}	// End of namespace
