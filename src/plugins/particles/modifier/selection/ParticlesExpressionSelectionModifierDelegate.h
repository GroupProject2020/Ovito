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
#include <plugins/stdmod/modifiers/ExpressionSelectionModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)

/**
 * \brief Delegate for the ExpressionSelectionModifier that operates on particles.
 */
class ParticlesExpressionSelectionModifierDelegate : public ExpressionSelectionModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public ExpressionSelectionModifierDelegate::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using ExpressionSelectionModifierDelegate::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier delegate can operate on the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("particles"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(ParticlesExpressionSelectionModifierDelegate, OOMetaClass)
	Q_CLASSINFO("DisplayName", "Particles");

public:

	/// Constructor.
	Q_INVOKABLE ParticlesExpressionSelectionModifierDelegate(DataSet* dataset) : ExpressionSelectionModifierDelegate(dataset) {}

	/// Creates and initializes the expression evaluator object.
	virtual std::unique_ptr<PropertyExpressionEvaluator> initializeExpressionEvaluator(const QStringList& expressions, const PipelineFlowState& inputState, int animationFrame) override;

	/// Creates the output selection property object.
	virtual PropertyObject* createOutputSelectionProperty(OutputHelper& oh) override;
};

/**
 * \brief Delegate for the ExpressionSelectionModifier that operates on bonds.
 */
class BondsExpressionSelectionModifierDelegate : public ExpressionSelectionModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public ExpressionSelectionModifierDelegate::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using ExpressionSelectionModifierDelegate::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier delegate can operate on the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("bonds"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(BondsExpressionSelectionModifierDelegate, OOMetaClass)
	Q_CLASSINFO("DisplayName", "Bonds");

public:

	/// Constructor.
	Q_INVOKABLE BondsExpressionSelectionModifierDelegate(DataSet* dataset) : ExpressionSelectionModifierDelegate(dataset) {}

	/// Creates and initializes the expression evaluator object.
	virtual std::unique_ptr<PropertyExpressionEvaluator> initializeExpressionEvaluator(const QStringList& expressions, const PipelineFlowState& inputState, int animationFrame) override;

	/// Creates the output selection property object.
	virtual PropertyObject* createOutputSelectionProperty(OutputHelper& oh) override;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
