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
#include <ovito/stdmod/modifiers/ExpressionSelectionModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)

using namespace Ovito::StdMod;

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

		/// Indicates which data objects in the given input data collection the modifier delegate is able to operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const override;

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("particles"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(ParticlesExpressionSelectionModifierDelegate, OOMetaClass)
	Q_CLASSINFO("DisplayName", "Particles");

public:

	/// Constructor.
	Q_INVOKABLE ParticlesExpressionSelectionModifierDelegate(DataSet* dataset) : ExpressionSelectionModifierDelegate(dataset) {}

	/// Looks up the container for the properties in the output pipeline state.
	virtual PropertyContainer* getOutputPropertyContainer(PipelineFlowState& outputState) const override;

	/// Creates and initializes the expression evaluator object.
	virtual std::unique_ptr<PropertyExpressionEvaluator> initializeExpressionEvaluator(const QStringList& expressions, const PipelineFlowState& inputState, int animationFrame) override;
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

		/// Indicates which data objects in the given input data collection the modifier delegate is able to operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const override;

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("bonds"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(BondsExpressionSelectionModifierDelegate, OOMetaClass)
	Q_CLASSINFO("DisplayName", "Bonds");

public:

	/// Constructor.
	Q_INVOKABLE BondsExpressionSelectionModifierDelegate(DataSet* dataset) : ExpressionSelectionModifierDelegate(dataset) {}

	/// Looks up the container for the properties in the output pipeline state.
	virtual PropertyContainer* getOutputPropertyContainer(PipelineFlowState& outputState) const override;

	/// Creates and initializes the expression evaluator object.
	virtual std::unique_ptr<PropertyExpressionEvaluator> initializeExpressionEvaluator(const QStringList& expressions, const PipelineFlowState& inputState, int animationFrame) override;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
