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

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/stdobj/properties/PropertyExpressionEvaluator.h>

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
		PropertyExpressionEvaluator::initialize(expressions, inputState, inputState.expectObject<ParticlesObject>(), animationFrame);
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
		PropertyExpressionEvaluator::initialize(expressions, inputState, inputState.expectObject<ParticlesObject>()->expectBonds(), animationFrame);
	}
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
