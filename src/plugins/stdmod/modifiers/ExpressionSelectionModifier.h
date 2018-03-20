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


#include <plugins/stdmod/StdMod.h>
#include <core/dataset/pipeline/DelegatingModifier.h>
#include <plugins/stdobj/properties/PropertyExpressionEvaluator.h>

namespace Ovito { namespace StdMod {

/**
 * \brief Base class for ExpressionSelectionModifier delegates that operate on different kinds of data.
 */
class OVITO_STDMOD_EXPORT ExpressionSelectionModifierDelegate : public ModifierDelegate
{
	OVITO_CLASS(ExpressionSelectionModifierDelegate)
	Q_OBJECT
	
public:
	
	/// \brief Applies the modifier operation to the data in a pipeline flow state.
	virtual PipelineStatus apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp) override;
	
protected:

	/// Abstract class constructor.
	using ModifierDelegate::ModifierDelegate;

	/// Creates and initializes the expression evaluator object.
	virtual std::unique_ptr<PropertyExpressionEvaluator> initializeExpressionEvaluator(const QStringList& expressions, const PipelineFlowState& inputState, int animationFrame) = 0;

	/// Creates the output selection property object.
	virtual PropertyObject* createOutputSelectionProperty(OutputHelper& oh) = 0;
};

/**
 * \brief Selects elements according to a user-defined Boolean expression.
 */
class OVITO_STDMOD_EXPORT ExpressionSelectionModifier : public DelegatingModifier
{
	/// Give this modifier class its own metaclass.
	class ExpressionSelectionModifierClass : public DelegatingModifier::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using DelegatingModifier::OOMetaClass::OOMetaClass;

		/// Return the metaclass of delegates for this modifier type. 
		virtual const ModifierDelegate::OOMetaClass& delegateMetaclass() const override { return ExpressionSelectionModifierDelegate::OOClass(); }
	};

	Q_OBJECT
	OVITO_CLASS_META(ExpressionSelectionModifier, ExpressionSelectionModifierClass)
	Q_CLASSINFO("DisplayName", "Expression selection");
	Q_CLASSINFO("ModifierCategory", "Selection");

public:

	/// Constructor.
	Q_INVOKABLE ExpressionSelectionModifier(DataSet* dataset);

	/// \brief Returns the list of available input variables.
	const QStringList& inputVariableNames() const { return _variableNames; }

	/// \brief Returns a human-readable text listing the input variables.
	const QString& inputVariableTable() const { return _variableTable; }

	/// \brief Stores the given information about the available input variables in the modifier.
	void setVariablesInfo(QStringList variableNames, QString variableTable) {
		if(variableNames != _variableNames || variableTable != _variableTable) {
			_variableNames = std::move(variableNames);
			_variableTable = std::move(variableTable);
			notifyDependents(ReferenceEvent::ObjectStatusChanged);
		}
	}

private:

	/// The user expression for selecting elements.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, expression, setExpression);

	/// The list of input variables during the last evaluation.
	QStringList _variableNames;

	/// Human-readable text listing the input variables during the last evaluation.
	QString _variableTable;
};

}	// End of namespace
}	// End of namespace
