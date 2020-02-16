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
#include <ovito/particles/util/ParticleExpressionEvaluator.h>
#include <ovito/particles/util/ParticleOrderingFingerprint.h>
#include <ovito/stdmod/modifiers/ComputePropertyModifier.h>

namespace Ovito { namespace Particles {

using namespace Ovito::StdMod;

/**
 * \brief Delegate plugin for the ComputePropertyModifier that operates on particles.
 */
class OVITO_PARTICLES_EXPORT ParticlesComputePropertyModifierDelegate : public ComputePropertyModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public ComputePropertyModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using ComputePropertyModifierDelegate::OOMetaClass::OOMetaClass;

		/// Indicates which data objects in the given input data collection the modifier delegate is able to operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const override;

		/// Indicates which class of data objects the modifier delegate is able to operate on.
		virtual const DataObject::OOMetaClass& getApplicableObjectClass() const override { return ParticlesObject::OOClass(); }

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("particles"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(ParticlesComputePropertyModifierDelegate, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Particles");

public:

	/// Constructor.
	Q_INVOKABLE ParticlesComputePropertyModifierDelegate(DataSet* dataset);

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

	/// \brief Sets the number of vector components of the property to compute.
	/// \param componentCount The number of vector components.
	/// \undoable
	virtual void setComponentCount(int componentCount) override;

	/// Creates a computation engine that will compute the property values.
	virtual std::shared_ptr<ComputePropertyModifierDelegate::PropertyComputeEngine> createEngine(
				TimePoint time,
				const PipelineFlowState& input,
				const PropertyContainer* container,
				PropertyPtr outputProperty,
				ConstPropertyPtr selectionProperty,
				QStringList expressions) override;

private:

	/// Asynchronous compute engine that does the actual work in a separate thread.
	class ComputeEngine : public ComputePropertyModifierDelegate::PropertyComputeEngine
	{
	public:

		/// Constructor.
		ComputeEngine(
				const TimeInterval& validityInterval,
				TimePoint time,
				PropertyPtr outputProperty,
				const PropertyContainer* container,
				ConstPropertyPtr selectionProperty,
				QStringList expressions,
				int frameNumber,
				const PipelineFlowState& input,
				ConstPropertyPtr positions,
				QStringList neighborExpressions,
				FloatType cutoff);

		/// Returns the list of available input variables for the expressions managed by the delegate.
		virtual QStringList delegateInputVariableNames() const override;

		/// Determines whether any of the math expressions is explicitly time-dependent.
		virtual bool isTimeDependent() override;

		/// Returns a human-readable text listing the input variables.
		virtual QString inputVariableTable() const override;

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Returns the property storage that contains the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Indicates whether contributions from particle neighbors are taken into account.
		bool neighborMode() const { return _neighborMode; }

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	private:

		const FloatType _cutoff;
		QStringList _neighborExpressions;
		bool _neighborMode;
		ConstPropertyPtr _positions;
		std::unique_ptr<ParticleExpressionEvaluator> _neighborEvaluator;
		ParticleOrderingFingerprint _inputFingerprint;
	};

	/// The math expressions for calculating the neighbor-terms of the property function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QStringList, neighborExpressions, setNeighborExpressions);

	/// Controls the cutoff radius for the neighbor lists.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, cutoff, setCutoff, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether multi-line input fields are shown in the UI for the expressions.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useMultilineFields, setUseMultilineFields);
};

}	// End of namespace
}	// End of namespace
