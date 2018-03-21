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

#pragma once


#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/stdobj/properties/PropertyExpressionEvaluator.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief Helper class that evaluates one or more math expressions for every particle.
 *
 * This class is used by the ComputePropertyModifier and the ExpressionSelectionModifier.
 */
class OVITO_PARTICLES_EXPORT ParticleExpressionEvaluator : public PropertyExpressionEvaluator
{
public:

	/// Constructor.
	ParticleExpressionEvaluator() {
		setIndexVarName("ParticleIndex");
	}

	using PropertyExpressionEvaluator::initialize;

	/// Specifies the expressions to be evaluated for each particle and creates the input variables.
	void initialize(const QStringList& expressions, const PipelineFlowState& inputState, int animationFrame = 0) {
		PropertyExpressionEvaluator::initialize(expressions, inputState, ParticleProperty::OOClass(), animationFrame);
	}
	
protected:

	/// Initializes the list of input variables from the given input state.
	virtual void createInputVariables(const std::vector<ConstPropertyPtr>& inputProperties, const SimulationCell* simCell, const QVariantMap& attributes, int animationFrame) override;
};

/**
 * \brief Helper class that evaluates one or more math expressions for every bond.
 */
class OVITO_PARTICLES_EXPORT BondExpressionEvaluator : public PropertyExpressionEvaluator
{
public:

	/// Constructor.
	BondExpressionEvaluator() {
		setIndexVarName("BondIndex");
	}

	using PropertyExpressionEvaluator::initialize;

	/// Specifies the expressions to be evaluated for each bond and creates the input variables.
	void initialize(const QStringList& expressions, const PipelineFlowState& inputState, int animationFrame = 0) {
		PropertyExpressionEvaluator::initialize(expressions, inputState, BondProperty::OOClass(), animationFrame);
	}
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
