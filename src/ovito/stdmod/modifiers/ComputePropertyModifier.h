////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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


#include <ovito/stdmod/StdMod.h>
#include <ovito/stdobj/simcell/SimulationCell.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/stdobj/properties/PropertyReference.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/stdobj/properties/PropertyExpressionEvaluator.h>
#include <ovito/core/dataset/pipeline/AsynchronousDelegatingModifier.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifierApplication.h>

namespace Ovito { namespace StdMod {

/**
 * \brief Base class for modifier delegates used by the ComputePropertyModifier class.
 */
class OVITO_STDMOD_EXPORT ComputePropertyModifierDelegate : public AsynchronousModifierDelegate
{
	Q_OBJECT
	OVITO_CLASS(ComputePropertyModifierDelegate)

protected:

	/// Constructor.
	using AsynchronousModifierDelegate::AsynchronousModifierDelegate;

	/// Asynchronous compute engine that does the actual work in a separate thread.
	class OVITO_STDMOD_EXPORT PropertyComputeEngine : public AsynchronousModifier::ComputeEngine
	{
	public:

		/// Constructor.
		PropertyComputeEngine(const TimeInterval& validityInterval,
				TimePoint time,
				const PipelineFlowState& input,
				const PropertyContainer* container,
				PropertyPtr outputProperty,
				ConstPropertyPtr selectionProperty,
				QStringList expressions,
				int frameNumber,
				std::unique_ptr<PropertyExpressionEvaluator> evaluator);

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Returns the data accessor to the selection flag array.
		const ConstPropertyAccessAndRef<int>& selectionArray() const { return _selectionArray; }

		/// Returns the list of available input variables.
		virtual QStringList inputVariableNames() const;

		/// Returns the list of available input variables for the expressions managed by the delegate.
		virtual QStringList delegateInputVariableNames() const { return {}; }

		/// Returns a human-readable text listing the input variables.
		virtual QString inputVariableTable() const {
			if(_evaluator) return _evaluator->inputVariableTable();
			return {};
		}

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		/// Returns the property storage that will receive the computed values.
		const PropertyPtr& outputProperty() const { return _outputProperty; }

		/// Returns the data accessor to the output property array that will receive the computed values.
		PropertyAccess<void, true>& outputArray() { return _outputArray; }

		/// Determines whether any of the math expressions is explicitly time-dependent.
		virtual bool isTimeDependent() { return _evaluator->isTimeDependent(); }

	protected:

		/// Releases data that is no longer needed.
		void releaseWorkingData() {
			_selectionArray.reset();
			_expressions.clear();
			_evaluator.reset();
			_outputArray.reset();
		}

		const int _frameNumber;
		QStringList _expressions;
		ConstPropertyAccessAndRef<int> _selectionArray;
		std::unique_ptr<PropertyExpressionEvaluator> _evaluator;
		const PropertyPtr _outputProperty;
		PropertyAccess<void, true> _outputArray;
	};

public:

	/// Returns the type of input property container that this delegate can process.
	PropertyContainerClassPtr inputContainerClass() const {
		return static_class_cast<PropertyContainer>(&getOOMetaClass().getApplicableObjectClass());
	}

	/// Returns the reference to the selected input property container for this delegate.
	PropertyContainerReference inputContainerRef() const {
		return PropertyContainerReference(inputContainerClass(), inputDataObject().dataPath(), inputDataObject().dataTitle());
	}

	/// \brief Sets the number of vector components of the property to compute.
	/// \param componentCount The number of vector components.
	/// \undoable
	virtual void setComponentCount(int componentCount) {}

	/// Creates a computation engine that will compute the property values.
	virtual std::shared_ptr<PropertyComputeEngine> createEngine(
				TimePoint time,
				const PipelineFlowState& input,
				const PropertyContainer* container,
				PropertyPtr outputProperty,
				ConstPropertyPtr selectionProperty,
				QStringList expressions) = 0;
};

/**
 * \brief Computes the values of a property from a user-defined math expression.
 */
class OVITO_STDMOD_EXPORT ComputePropertyModifier : public AsynchronousDelegatingModifier
{
	/// Give this modifier class its own metaclass.
	class ComputePropertyModifierClass : public AsynchronousDelegatingModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using AsynchronousDelegatingModifier::OOMetaClass::OOMetaClass;

		/// Return the metaclass of delegates for this modifier type.
		virtual const AsynchronousModifierDelegate::OOMetaClass& delegateMetaclass() const override { return ComputePropertyModifierDelegate::OOClass(); }
	};

	Q_OBJECT
	OVITO_CLASS_META(ComputePropertyModifier, ComputePropertyModifierClass)

	Q_CLASSINFO("DisplayName", "Compute property");
	Q_CLASSINFO("ModifierCategory", "Modification");

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE ComputePropertyModifier(DataSet* dataset);

	/// \brief Returns the current delegate of this ComputePropertyModifier.
	ComputePropertyModifierDelegate* delegate() const { return static_object_cast<ComputePropertyModifierDelegate>(AsynchronousDelegatingModifier::delegate()); }

	/// \brief Sets the math expression that is used to calculate the values of one of the new property's components.
	/// \param index The property component for which the expression should be set.
	/// \param expression The math formula.
	/// \undoable
	void setExpression(const QString& expression, int index = 0) {
		if(index < 0 || index >= expressions().size())
			throwException("Property component index is out of range.");
		QStringList copy = _expressions;
		copy[index] = expression;
		setExpressions(std::move(copy));
	}

	/// \brief Returns the math expression that is used to calculate the values of one of the new property's components.
	/// \param index The property component for which the expression should be returned.
	/// \return The math formula used to calculates the channel's values.
	/// \undoable
	const QString& expression(int index = 0) const {
		if(index < 0 || index >= expressions().size())
			throwException("Property component index is out of range.");
		return expressions()[index];
	}

	/// \brief Returns the number of vector components of the property to create.
	/// \return The number of vector components.
	/// \sa setPropertyComponentCount()
	int propertyComponentCount() const { return expressions().size(); }

	/// \brief Sets the number of vector components of the property to create.
	/// \param newComponentCount The number of vector components.
	/// \undoable
	void setPropertyComponentCount(int newComponentCount);

	/// This method indicates whether cached computation results of the modifier should be discarded whenever
	/// a parameter of the modifier changes.
	virtual bool discardResultsOnModifierChange(const PropertyFieldEvent& event) const override {
		if(event.field() == &PROPERTY_FIELD(useMultilineFields)) return false;
		return AsynchronousDelegatingModifier::discardResultsOnModifierChange(event);
	}

protected:

	/// \brief Is called when the value of a reference field of this RefMaker changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input) override;

protected:

	/// The math expressions for calculating the property values. One for every vector component.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QStringList, expressions, setExpressions);

	/// Specifies the output property that will receive the computed per-particles values.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(PropertyReference, outputProperty, setOutputProperty);

	/// Controls whether the math expression is evaluated and output only for selected elements.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedElements, setOnlySelectedElements);

	/// Controls whether multi-line input fields are shown in the UI for the expressions.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useMultilineFields, setUseMultilineFields);
};

/**
 * Used by the ComputePropertyModifier to store working data.
 */
class OVITO_STDMOD_EXPORT ComputePropertyModifierApplication : public AsynchronousModifierApplication
{
	OVITO_CLASS(ComputePropertyModifierApplication)
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ComputePropertyModifierApplication(DataSet* dataset) : AsynchronousModifierApplication(dataset) {}

private:

	/// The cached visualization elements that are attached to the output property.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD_FLAGS(DataVis, cachedVisElements, setCachedVisElements, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_CHANGE_MESSAGE | PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_SUB_ANIM);

	/// The list of input variables during the last evaluation.
	DECLARE_RUNTIME_PROPERTY_FIELD_FLAGS(QStringList, inputVariableNames, setInputVariableNames, PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// The list of input variables for the expressions managed by the delegate during the last evaluation.
	DECLARE_RUNTIME_PROPERTY_FIELD_FLAGS(QStringList, delegateInputVariableNames, setDelegateInputVariableNames, PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// Human-readable text listing the input variables during the last evaluation.
	DECLARE_RUNTIME_PROPERTY_FIELD_FLAGS(QString, inputVariableTable, setInputVariableTable, PROPERTY_FIELD_NO_CHANGE_MESSAGE);
};

}	// End of namespace
}	// End of namespace
