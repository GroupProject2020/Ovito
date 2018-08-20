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


#include <plugins/stdobj/StdObj.h>
#include <plugins/stdobj/simcell/SimulationCell.h>
#include <core/dataset/pipeline/PipelineFlowState.h>

#include <muParser.h>
#include <boost/utility.hpp>
#include <boost/optional.hpp>

namespace Ovito { namespace StdObj {

/**
 * \brief Helper class that evaluates one or more math expressions for every data element.
 */
class OVITO_STDOBJ_EXPORT PropertyExpressionEvaluator
{
	Q_DECLARE_TR_FUNCTIONS(PropertyExpressionEvaluator);

public:

	/// \brief Constructor.
	PropertyExpressionEvaluator() = default;

	/// Specifies the expressions to be evaluated for each particle and creates the input variables.
	void initialize(const QStringList& expressions, const PipelineFlowState& inputState, const PropertyClass& propertyClass, const QString& bundle = QString(), int animationFrame = 0);

	/// Specifies the expressions to be evaluated for each particle and creates the input variables.
	void initialize(const QStringList& expressions, const std::vector<ConstPropertyPtr>& inputProperties, const SimulationCell* simCell, const QVariantMap& attributes, int animationFrame = 0);

	/// Initializes the parser object and evaluates the expressions for every particle.
	void evaluate(const std::function<void(size_t,size_t,double)>& callback, const std::function<bool(size_t)>& filter = std::function<bool(size_t)>());

	/// Returns the maximum number of threads used to evaluate the expression (or 0 if all processor cores are used).
	size_t maxThreadCount() const { return _maxThreadCount; }

	/// Sets The maximum number of threads used to evaluate the expression (or 0 if all processor cores should be used).
	void setMaxThreadCount(size_t count) { _maxThreadCount = count; }

	/// Returns the number of input data element.
	size_t elementCount() const { return _elementCount; }

	/// Returns the list of expressions.
	const std::vector<std::string>& expression() const { return _expressions; }

	/// Returns the list of available input variables.
	QStringList inputVariableNames() const;

	/// Returns a human-readable text listing the input variables.
	QString inputVariableTable() const;

	/// Returns the stored simulation cell information.
	const SimulationCell& simCell() const { return _simCell; }

	/// Sets the name of the variable that provides the index of the current element.
	void setIndexVarName(QString name) { _indexVarName = std::move(name); }

	/// Returns the name of the variable that provides the index of the current element.
	const QString& indexVarName() const { return _indexVarName; }

	/// Returns whether a variable is being referenced in one of the expressions.
	bool isVariableUsed(const char* varName);

	/// Returns whether the expression depends on animation time.
	bool isTimeDependent() { return isVariableUsed("Frame"); }

	/// Registers a new input variable whose value is recomputed for each data element.
	void registerComputedVariable(const QString& variableName, std::function<double(size_t)> function, QString description = QString(), int variableClass = 0) {
		ExpressionVariable v;
		v.type = DERIVED_PROPERTY;
		v.name = variableName.toStdString();
		v.function = std::move(function);
		v.description = description;
		v.variableClass = variableClass;
		addVariable(std::move(v));
	}

	/// Registers a new input variable whose value is uniform.
	void registerGlobalParameter(const QString& variableName, double value, QString description = QString()) {
		ExpressionVariable v;
		v.type = GLOBAL_PARAMETER;
		v.name = variableName.toStdString();
		v.value = value;
		v.description = description;
		addVariable(std::move(v));
	}

	/// Registers a new input variable whose value is constant.
	void registerConstant(const QString& variableName, double value, QString description = QString()) {
		ExpressionVariable v;
		v.type = CONSTANT;
		v.name = variableName.toStdString();
		v.value = value;
		v.description = description;
		addVariable(std::move(v));
	}

	/// Registers a new input variable whose value is reflects the current element index.
	void registerIndexVariable(const QString& variableName, int variableClass, QString description = QString()) {
		ExpressionVariable v;
		v.type = ELEMENT_INDEX;
		v.name = variableName.toStdString();
		v.variableClass = variableClass;
		v.description = description;
		addVariable(std::move(v));
	}

	/// Registers a list of expression variables that refer to input properties.
	void registerPropertyVariables(const std::vector<ConstPropertyPtr>& inputProperties, int variableClass, const char* namePrefix = nullptr);

protected:

	enum ExpressionVariableType {
		FLOAT_PROPERTY,
		INT_PROPERTY,
		INT64_PROPERTY,
		DERIVED_PROPERTY,
		ELEMENT_INDEX,
		GLOBAL_PARAMETER,
		CONSTANT
	};

	/// Data structure representing an input variable.
	struct ExpressionVariable {
		/// Indicates whether this variable has been successfully registered with the muParser.
		bool isRegistered = false;
		/// Indicates whether this variable is referenced by at least one of the expressions.
		bool isReferenced = false;
		/// The variable's value for the current data element.
		double value;
		/// Pointer into the property storage.
		const char* dataPointer;
		/// Data array stride in the property storage.
		size_t stride;
		/// The type of variable.
		ExpressionVariableType type;
		/// The original name of the variable.
		std::string name;
		/// The name of the variable as registered with the muparser.
		std::string mangledName;
		/// Human-readable description.
		QString description;
		/// A function that computes the variable's value for each data element.
		std::function<double(size_t)> function;
		/// Reference the original property that contains the data.
		ConstPropertyPtr property;
		/// Indicates whether this variable is a caller-defined element variable.
		int variableClass = 0;
		
		/// Retrieves the value of the variable and stores it in the memory location passed to muparser.
		void updateValue(size_t elementIndex);
	};

public:

	/// One instance of this class is created per thread.
	class Worker : boost::noncopyable {
	public:

		/// Initializes the worker instance.
		Worker(PropertyExpressionEvaluator& evaluator);

		/// Evaluates the expression for a specific data element and a specific vector component.
		double evaluate(size_t elementIndex, size_t component);

		/// Returns the storage address of a variable value.
		double* variableAddress(const char* varName) {
			for(ExpressionVariable& var : _variables) {
				if(var.name == varName)
					return &var.value;
			}
			OVITO_ASSERT(false);
			return nullptr;
		}

		// Returns whether the given variable is being referenced in one of the expressions.
		bool isVariableUsed(const char* varName) const {
			for(const ExpressionVariable& var : _variables)
				if(var.name == varName && var.isReferenced) return true;
			return false;
		}
		
		/// Updates the stored value of variables that depends on the current element index.
		void updateVariables(int variableClass, size_t elementIndex) {
			for(ExpressionVariable& v : _variables) {
				if(v.variableClass == variableClass)
					v.updateValue(elementIndex);
			}
		}

	private:

		/// The worker routine.
		void run(size_t startIndex, size_t endIndex, std::function<void(size_t,size_t,double)> callback, std::function<bool(size_t)> filter);

		/// List of parser objects used by this thread.
		std::vector<mu::Parser> _parsers;

		/// List of input variables used by the parsers of this thread.
		std::vector<ExpressionVariable> _variables;

		/// The index of the last data element for which the expressions were evaluated.
		size_t _lastElementIndex = std::numeric_limits<size_t>::max();

		/// Error message reported by one of the parser objects (remains empty on success).
		QString _errorMsg;

		friend class PropertyExpressionEvaluator;
	};

protected:

	/// Initializes the list of input variables from the given input state.
	virtual void createInputVariables(const std::vector<ConstPropertyPtr>& inputProperties, const SimulationCell* simCell, const QVariantMap& attributes, int animationFrame);

	/// Registers an input variable if the name does not exist yet.
	size_t addVariable(ExpressionVariable v);

	/// The list of expression that should be evaluated for each data element.
	std::vector<std::string> _expressions;

	/// The list of input variables that can be referenced in the expressions.
	std::vector<ExpressionVariable> _variables;

	/// Indicates whether the list of referenced variables has been determined.
	bool _referencedVariablesKnown = false;

	/// The number of input data elements.
	size_t _elementCount = 0;

	/// List of characters allowed in variable names.
	static QByteArray _validVariableNameChars;

	/// The maximum number of threads used to evaluate the expression.
	size_t _maxThreadCount = 0;

	/// The name of the variable that provides the index of the current element.
	QString _indexVarName;

	/// Human-readable name describing the data elements, e.g. "particles".
	QString _elementDescriptionName;

	/// The simulation cell information.
	SimulationCell _simCell;
};

}	// End of namespace
}	// End of namespace
