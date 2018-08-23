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
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/particles/util/ParticleExpressionEvaluator.h>
#include <plugins/particles/util/ParticleOrderingFingerprint.h>
#include <plugins/stdmod/modifiers/ComputePropertyModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

/**
 * \brief Delegate plugin for the ComputePropertyModifier that operates on bonds.
 */
class OVITO_PARTICLES_EXPORT BondsComputePropertyModifierDelegate : public ComputePropertyModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public ComputePropertyModifierDelegate::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using ComputePropertyModifierDelegate::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier delegate can operate on the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override {
			return input.findObjectOfType<BondProperty>() != nullptr;
		}

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("bonds"); }
	};	

	Q_OBJECT
	OVITO_CLASS_META(BondsComputePropertyModifierDelegate, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Bonds");

public:

	/// Constructor.
	Q_INVOKABLE BondsComputePropertyModifierDelegate(DataSet* dataset);

	/// \brief Returns the class of properties this delegate computes.
	virtual const PropertyClass& propertyClass() const override { return BondProperty::OOClass(); }

	/// Creates a computation engine that will compute the property values.
	virtual std::shared_ptr<ComputePropertyModifierDelegate::PropertyComputeEngine> createEngine(
				TimePoint time, 
				const PipelineFlowState& input,
				PropertyPtr outputProperty, 
				ConstPropertyPtr selectionProperty,
				QStringList expressions, 
				bool initializeOutputProperty) override;

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
				ConstPropertyPtr selectionProperty,
				QStringList expressions, 
				int frameNumber, 
				const PipelineFlowState& input);
				
		/// Computes the modifier's results.
		virtual void perform() override;

		/// Returns a human-readable text listing the input variables.
		virtual QString inputVariableTable() const override;

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

	private:

		ParticleOrderingFingerprint _inputFingerprint;
		ConstPropertyPtr _topology;
	};
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
