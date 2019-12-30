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

#include <ovito/stdobj/StdObj.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include "PropertyExpressionEvaluator.h"

namespace Ovito { namespace StdObj {

/// List of characters allowed in variable names.
QByteArray PropertyExpressionEvaluator::_validVariableNameChars("0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.@");

/******************************************************************************
* Specifies the expressions to be evaluated for each data element and create the
* list of input variables.
******************************************************************************/
void PropertyExpressionEvaluator::initialize(const QStringList& expressions, const PipelineFlowState& state, const PropertyContainer* container, int animationFrame)
{
	// Build list of properties that will be made available as expression variables.
	std::vector<ConstPropertyPtr> inputProperties;
	for(const PropertyObject* property : container->properties())
		inputProperties.push_back(property->storage());
	_elementDescriptionName = container->getOOMetaClass().elementDescriptionName();

	// Get simulation cell information.
	SimulationCell simCell;
	const SimulationCellObject* simCellObj = state.getObject<SimulationCellObject>();
	if(simCellObj) simCell = simCellObj->data();

	// Call overloaded function.
	initialize(expressions, inputProperties, simCellObj ? &simCell : nullptr, state.buildAttributesMap(), animationFrame);
}

/******************************************************************************
* Specifies the expressions to be evaluated for each data element and create the
* list of input variables.
******************************************************************************/
void PropertyExpressionEvaluator::initialize(const QStringList& expressions, const std::vector<ConstPropertyPtr>& inputProperties, const SimulationCell* simCell, const QVariantMap& attributes, int animationFrame)
{
	// Determine number of input elements.
	_elementCount = inputProperties.empty() ? 0 : inputProperties.front()->size();
	_referencedVariablesKnown = false;

	// Create list of input variables.
	createInputVariables(inputProperties, simCell, attributes, animationFrame);

	// Copy expression strings into internal array.
	_expressions.resize(expressions.size());
	std::transform(expressions.begin(), expressions.end(), _expressions.begin(), [](const QString& s) -> std::string { return s.toStdString(); });
}

/******************************************************************************
* Initializes the list of input variables from the given input state.
******************************************************************************/
void PropertyExpressionEvaluator::createInputVariables(const std::vector<ConstPropertyPtr>& inputProperties, const SimulationCell* simCell, const QVariantMap& attributes, int animationFrame)
{
	// Register the list of expression variables that refer to input properties.
	registerPropertyVariables(inputProperties, 0);

	// Create index variable.
	if(!_indexVarName.isEmpty())
		registerIndexVariable(_indexVarName, 0, tr("zero-based"));

	// Create constant variables.
	ExpressionVariable constVar;

	// Number of elements
	registerGlobalParameter("N", elementCount(), tr("total number of %1").arg(_elementDescriptionName.isEmpty() ? tr("elements") : _elementDescriptionName));

	// Animation frame
	registerGlobalParameter("Frame", animationFrame, tr("animation frame number"));

	// Global attributes
	for(auto entry = attributes.constBegin(); entry != attributes.constEnd(); ++entry) {
		if(entry.value().type() == QVariant::String)
			continue;

		if(entry.value().canConvert<double>())
			registerGlobalParameter(entry.key(), entry.value().toDouble());
		else if(entry.value().canConvert<long>())
			registerGlobalParameter(entry.key(), entry.value().value<long>());
	}

	if(simCell) {
		// Store simulation cell data.
		_simCell = *simCell;

		// Cell volume
		registerGlobalParameter("CellVolume", simCell->volume3D(), tr("simulation cell volume"));

		// Cell size
		registerGlobalParameter("CellSize.X", std::abs(simCell->matrix().column(0).x()), tr("size along X"));
		registerGlobalParameter("CellSize.Y", std::abs(simCell->matrix().column(1).y()), tr("size along Y"));
		registerGlobalParameter("CellSize.Z", std::abs(simCell->matrix().column(2).z()), tr("size along Z"));
	}

	// Constant pi
	registerConstant("pi", M_PI, QStringLiteral("%1...").arg(M_PI));
}

/******************************************************************************
* Registers the list of expression variables that refer to input properties.
******************************************************************************/
void PropertyExpressionEvaluator::registerPropertyVariables(const std::vector<ConstPropertyPtr>& inputProperties, int variableClass, const char* namePrefix)
{
	int propertyIndex = 1;
	for(const ConstPropertyPtr& property : inputProperties) {
		ExpressionVariable v;

		// Properties with custom data type are not supported by this modifier.
		if(property->dataType() == PropertyStorage::Int)
			v.type = INT_PROPERTY;
		else if(property->dataType() == PropertyStorage::Int64)
			v.type = INT64_PROPERTY;
		else if(property->dataType() == PropertyStorage::Float)
			v.type = FLOAT_PROPERTY;
		else
			continue;
		v.property = property;
		v.variableClass = variableClass;
		v.propertyArray = property;

		// Derive a valid variable name from the property name by removing all invalid characters.
		QString propertyName = property->name();
		// If the name is empty, generate one.
		if(propertyName.isEmpty())
			propertyName = QString("Property%1").arg(propertyIndex);
		// If the name starts with a number, prepend an underscore.
		else if(propertyName[0].isDigit())
			propertyName.prepend(QChar('_'));

		for(size_t k = 0; k < property->componentCount(); k++) {

			QString fullPropertyName = propertyName;
			if(property->componentNames().size() == property->componentCount())
				fullPropertyName += "." + property->componentNames()[k];
			if(!namePrefix)
				v.name = fullPropertyName.toStdString();
			else
				v.name = namePrefix + fullPropertyName.toStdString();

			// Initialize data pointer into property storage.
			v.dataPointer = v.propertyArray.cdata(k);
			v.stride = v.propertyArray.stride();

			// Register variable.
			addVariable(v);
		}

		propertyIndex++;
	}
}

/******************************************************************************
* Registers an input variable if the name does not exist yet.
******************************************************************************/
size_t PropertyExpressionEvaluator::addVariable(ExpressionVariable v)
{
	// Replace invalid characters in variable name with an underscore.
	v.mangledName.clear();
	v.mangledName.reserve(v.name.size());
	for(char c : v.name) {
		// Remove whitespace from variable names.
		if(c <= ' ') continue;
		// Replace other invalid characters in variable names with an underscore.
		v.mangledName.push_back(_validVariableNameChars.contains(c) ? c : '_');
	}
	if(!v.mangledName.empty()) {
		// Check if mangled name is unique.
		if(std::none_of(_variables.begin(), _variables.end(), [&v](const ExpressionVariable& v2) -> bool { return v2.mangledName == v.mangledName; })) {
			v.isRegistered = true;
		}
	}
	_referencedVariablesKnown = false;
	_variables.push_back(std::move(v));
	return _variables.size() - 1;
}

/******************************************************************************
* Returns the list of available input variables.
******************************************************************************/
QStringList PropertyExpressionEvaluator::inputVariableNames() const
{
	QStringList vlist;
	for(const ExpressionVariable& v : _variables) {
		if(v.isRegistered)
			vlist << QString::fromStdString(v.mangledName);
	}
	return vlist;
}

/******************************************************************************
* Returns whether a variable is being referenced in one of the expressions.
******************************************************************************/
bool PropertyExpressionEvaluator::isVariableUsed(const char* varName)
{
	if(!_referencedVariablesKnown) {
		Worker worker(*this);
		_variables = worker._variables;
		_referencedVariablesKnown = true;
	}
	for(const ExpressionVariable& var : _variables) {
		if(var.name == varName && var.isReferenced)
			return true;
	}
	return false;
}

/******************************************************************************
* Initializes the parser object and evaluates the expressions for every data element
******************************************************************************/
void PropertyExpressionEvaluator::evaluate(const std::function<void(size_t,size_t,double)>& callback, const std::function<bool(size_t)>& filter)
{
	// Make sure initialize() has been called.
	OVITO_ASSERT(!_variables.empty());

	// Determine the number of parallel threads to use.
	size_t nthreads = Application::instance()->idealThreadCount();
	if(maxThreadCount() != 0 && nthreads > maxThreadCount()) nthreads = maxThreadCount();
	if(elementCount() == 0)
		return;
	else if(elementCount() < 100) // Not worth spawning multiple threads for so few elements.
		nthreads = 1;
	else if(nthreads > elementCount())
		nthreads = elementCount();

	if(nthreads == 1) {
		Worker worker(*this);
		_variables = worker._variables;
		_referencedVariablesKnown = true;
		worker.run(0, elementCount(), callback, filter);
		if(worker._errorMsg.isEmpty() == false)
			throw Exception(worker._errorMsg);
	}
	else if(nthreads > 1) {
		parallelForChunks(elementCount(), [this, &callback, &filter](size_t startIndex, size_t chunkSize) {
			Worker worker(*this);
			if(startIndex == 0) {
				_variables = worker._variables;
				_referencedVariablesKnown = true;
			}
			worker.run(startIndex, startIndex + chunkSize, callback, filter);
			if(worker._errorMsg.isEmpty() == false)
				throw Exception(worker._errorMsg);
		});
	}
}

/******************************************************************************
* Initializes the parser objects of this thread.
******************************************************************************/
PropertyExpressionEvaluator::Worker::Worker(PropertyExpressionEvaluator& evaluator)
{
	_parsers.resize(evaluator._expressions.size());

	// Make a per-thread copy of the input variables.
	_variables = evaluator._variables;

	// Reset variable flags.
	for(ExpressionVariable& v : _variables)
		v.isReferenced = false;

	auto parser = _parsers.begin();
	auto expr = evaluator._expressions.cbegin();
	for(size_t i = 0; i < evaluator._expressions.size(); i++, ++parser, ++expr) {

		if(expr->empty()) {
			if(evaluator._expressions.size() > 1)
				throw Exception(tr("Expression %1 is empty.").arg(i+1));
			else
				throw Exception(tr("Expression is empty."));
		}

		try {

			// Configure parser to accept alpha-numeric characters and '.' in variable names.
			parser->DefineNameChars(_validVariableNameChars.constData());

			// Define some extra math functions.
			parser->DefineFun("fmod", static_cast<double (*)(double,double)>(fmod), false);

			// Let the muParser process the math expression.
			parser->SetExpr(*expr);

			// Register input variables.
			for(ExpressionVariable& v : _variables) {
				if(v.isRegistered)
					parser->DefineVar(v.mangledName, &v.value);
			}

			// Query list of used variables.
			for(const auto& vname : parser->GetUsedVar()) {
				for(ExpressionVariable& var : _variables) {
					if(var.isRegistered && var.mangledName == vname.first) {
						var.isReferenced = true;
					}
				}
			}
		}
		catch(mu::Parser::exception_type& ex) {
			throw Exception(QString::fromStdString(ex.GetMsg()));
		}
	}
}

/******************************************************************************
* The worker routine.
******************************************************************************/
void PropertyExpressionEvaluator::Worker::run(size_t startIndex, size_t endIndex, std::function<void(size_t,size_t,double)> callback, std::function<bool(size_t)> filter)
{
	try {
		for(size_t i = startIndex; i < endIndex; i++) {
			if(filter && !filter(i))
				continue;

			for(size_t j = 0; j < _parsers.size(); j++) {
				// Evaluate expression for the current data element.
				callback(i, j, evaluate(i, j));
			}
		}
	}
	catch(const Exception& ex) {
		_errorMsg = ex.message();
	}
}

/******************************************************************************
* The innermost evaluation routine.
******************************************************************************/
double PropertyExpressionEvaluator::Worker::evaluate(size_t elementIndex, size_t component)
{
	OVITO_ASSERT(component < _parsers.size());
	try {
		if(elementIndex != _lastElementIndex) {
			_lastElementIndex = elementIndex;

			// Update variable values for the current data element.
			updateVariables(0, elementIndex);
		}

		// Evaluate expression for the current data element.
		return _parsers[component].Eval();
	}
	catch(const mu::Parser::exception_type& ex) {
		throw Exception(QString::fromStdString(ex.GetMsg()));
	}
}

/******************************************************************************
* Retrieves the value of the variable and stores it in the memory location
* passed to muparser.
******************************************************************************/
void PropertyExpressionEvaluator::ExpressionVariable::updateValue(size_t elementIndex)
{
	if(!isReferenced)
		return;

	switch(type) {
	case FLOAT_PROPERTY:
		if(elementIndex < property->size())
			value = *reinterpret_cast<const FloatType*>(dataPointer + stride * elementIndex);
		break;
	case INT_PROPERTY:
		if(elementIndex < property->size())
			value = *reinterpret_cast<const int*>(dataPointer + stride * elementIndex);
		break;
	case INT64_PROPERTY:
		if(elementIndex < property->size())
			value = *reinterpret_cast<const qlonglong*>(dataPointer + stride * elementIndex);
		break;
	case ELEMENT_INDEX:
		value = elementIndex;
		break;
	case DERIVED_PROPERTY:
		value = function(elementIndex);
		break;
	case GLOBAL_PARAMETER:
	case CONSTANT:
		// Nothing to do.
		break;
	}
}

/******************************************************************************
* Returns a human-readable text listing the input variables.
******************************************************************************/
QString PropertyExpressionEvaluator::inputVariableTable() const
{
	QString str(tr("<p>Available input variables:</p><p><b>Properties:</b><ul>"));
	for(const ExpressionVariable& v : _variables) {
		if((v.type == FLOAT_PROPERTY || v.type == INT_PROPERTY || v.type == INT64_PROPERTY || v.type == ELEMENT_INDEX || v.type == DERIVED_PROPERTY) && v.isRegistered && v.variableClass == 0) {
			if(v.description.isEmpty())
				str.append(QStringLiteral("<li>%1</li>").arg(QString::fromStdString(v.mangledName)));
			else
				str.append(QStringLiteral("<li>%1 (<i style=\"color: #555;\">%2</i>)</li>").arg(QString::fromStdString(v.mangledName)).arg(v.description));
		}
	}
	str.append(QStringLiteral("</ul></p><p><b>Global values:</b><ul>"));
	for(const ExpressionVariable& v : _variables) {
		if(v.type == GLOBAL_PARAMETER && v.isRegistered) {
			if(v.description.isEmpty())
				str.append(QStringLiteral("<li>%1</li>").arg(QString::fromStdString(v.mangledName)));
			else
				str.append(QStringLiteral("<li>%1 (<i style=\"color: #555;\">%2</i>)</li>").arg(QString::fromStdString(v.mangledName)).arg(v.description));
		}
	}
	str.append(QStringLiteral("</ul></p><p><b>Constants:</b><ul>"));
	for(const ExpressionVariable& v : _variables) {
		if(v.type == CONSTANT && v.isRegistered) {
			if(v.description.isEmpty())
				str.append(QStringLiteral("<li>%1</li>").arg(QString::fromStdString(v.mangledName)));
			else
				str.append(QStringLiteral("<li>%1 (<i style=\"color: #555;\">%2</i>)</li>").arg(QString::fromStdString(v.mangledName)).arg(v.description));
		}
	}
	str.append(QStringLiteral("</ul></p>"));
	return str;
}

}	// End of namespace
}	// End of namespace
