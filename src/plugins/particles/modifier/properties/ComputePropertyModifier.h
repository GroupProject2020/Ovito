///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include <plugins/particles/util/ParticleExpressionEvaluator.h>
#include <plugins/stdobj/simcell/SimulationCell.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>
#include <core/dataset/pipeline/AsynchronousModifierApplication.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)

/**
 * \brief Computes the values of a particle property from a user-defined math expression.
 */
class OVITO_PARTICLES_EXPORT ComputePropertyModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class OOMetaClass : public AsynchronousModifier::OOMetaClass 
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(ComputePropertyModifier, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Compute property");
	Q_CLASSINFO("ModifierCategory", "Modification");
	
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE ComputePropertyModifier(DataSet* dataset);

	/// \brief This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(ModifierApplication* modApp) override;

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

	/// \brief Sets the math expression that is used to compute the neighbor-terms of the property function.
	/// \param index The property component for which the expression should be set.
	/// \param expression The math formula.
	/// \undoable
	void setNeighborExpression(const QString& expression, int index = 0) {
		if(index < 0 || index >= neighborExpressions().size())
			throwException("Property component index is out of range.");
		QStringList copy = _neighborExpressions;
		copy[index] = expression;
		setNeighborExpressions(std::move(copy));
	}

	/// \brief Returns the math expression that is used to compute the neighbor-terms of the property function.
	/// \param index The property component for which the expression should be returned.
	/// \return The math formula.
	/// \undoable
	const QString& neighborExpression(int index = 0) const {
		if(index < 0 || index >= neighborExpressions().size())
			throwException("Property component index is out of range.");
		return neighborExpressions()[index];
	}

	/// \brief Returns the list of available input variables.
	const QStringList& inputVariableNames() const { return _inputVariableNames; }

	/// \brief Returns a human-readable text listing the input variables.
	const QString& inputVariableTable() const { return _inputVariableTable; }

	/// \brief Stores the given information about the available input variables in the modifier.
	void setVariablesInfo(QStringList variableNames, QString variableTable) {
		if(variableNames != _inputVariableNames || variableTable != _inputVariableTable) {
			_inputVariableNames = std::move(variableNames);
			_inputVariableTable = std::move(variableTable);
			notifyDependents(ReferenceEvent::ObjectStatusChanged);
		}
	}

protected:

	/// \brief Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

protected:

	/// Holds the modifier's results.
	class PropertyComputeResults : public ComputeEngineResults
	{
	public:

		/// Constructor.
		PropertyComputeResults(const TimeInterval& validityInterval, PropertyPtr outputProperty) : ComputeEngineResults(validityInterval),
			_outputProperty(std::move(outputProperty)) {}

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
		
		/// Returns the property storage that will receive the computed values.
		const PropertyPtr& outputProperty() const { return _outputProperty; }
		
	private:

		const PropertyPtr _outputProperty;
	};

	/// Asynchronous compute engine that does the actual work in a background thread.
	class PropertyComputeEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		PropertyComputeEngine(const TimeInterval& validityInterval, 
				TimePoint time,
				PropertyPtr outputProperty, 
				ConstPropertyPtr positions, 
				ConstPropertyPtr selectionProperty,
				const SimulationCell& simCell, 
				FloatType cutoff,
				QStringList expressions, 
				QStringList neighborExpressions,
				std::vector<ConstPropertyPtr> inputProperties,
				int frameNumber, 
				QVariantMap attributes);
				
		/// Computes the modifier's results.
		virtual void perform() override;

		/// Returns the property storage that contains the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Returns the property storage that contains the input particle selection.
		const ConstPropertyPtr& selection() const { return _selection; }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

		/// Returns the list of available input variables.
		const QStringList& inputVariableNames() const { return _inputVariableNames; }

		/// Returns a human-readable text listing the input variables.
		const QString& inputVariableTable() const { return _inputVariableTable; }

		/// Indicates whether contributions from particle neighbors are taken into account.
		bool neighborMode() const { return _cutoff != 0; }

		/// Returns the property storage that will receive the computed values.
		const PropertyPtr& outputProperty() const { return _results->outputProperty(); }

	private:

		const FloatType _cutoff;
		const SimulationCell _simCell;
		const int _frameNumber;
		const QStringList _expressions;
		const QVariantMap _attributes;
		const QStringList _neighborExpressions;
		const ConstPropertyPtr _positions;
		const ConstPropertyPtr _selection;
		const std::vector<ConstPropertyPtr> _inputProperties;
		QStringList _inputVariableNames;
		QString _inputVariableTable;
		ParticleExpressionEvaluator _evaluator;
		ParticleExpressionEvaluator _neighborEvaluator;
		std::shared_ptr<PropertyComputeResults> _results;
	};

	/// The math expressions for calculating the property values. One for every vector component.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QStringList, expressions, setExpressions);

	/// Specifies the output property that will receive the computed per-particles values.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, outputProperty, setOutputProperty);

	/// Controls whether the math expression is evaluated and output only for selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedParticles, setOnlySelectedParticles);

	/// Controls whether the contributions from neighbor terms are included in the computation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, neighborModeEnabled, setNeighborModeEnabled);

	/// The math expressions for calculating the neighbor-terms of the property function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QStringList, neighborExpressions, setNeighborExpressions);

	/// Controls the cutoff radius for the neighbor lists.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, cutoff, setCutoff, PROPERTY_FIELD_MEMORIZE);

	/// The list of input variables during the last evaluation.
	QStringList _inputVariableNames;

	/// Human-readable text listing the input variables during the last evaluation.
	QString _inputVariableTable;
};


/**
 * Used by the ComputePropertyModifier to store working data.
 */
class OVITO_PARTICLES_EXPORT ComputePropertyModifierApplication : public AsynchronousModifierApplication
{
	OVITO_CLASS(ComputePropertyModifierApplication)
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ComputePropertyModifierApplication(DataSet* dataset) : AsynchronousModifierApplication(dataset) {}

private:

	/// The cached visualization elements that are attached to the output particle property.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD_FLAGS(DataVis, cachedVisElements, setCachedVisElements, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_CHANGE_MESSAGE | PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_SUB_ANIM);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


