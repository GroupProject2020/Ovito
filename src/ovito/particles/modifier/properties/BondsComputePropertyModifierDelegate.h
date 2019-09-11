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


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/util/ParticleExpressionEvaluator.h>
#include <ovito/particles/util/ParticleOrderingFingerprint.h>
#include <ovito/stdmod/modifiers/ComputePropertyModifier.h>

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
		virtual bool isApplicableTo(const DataCollection& input) const override {
			if(const ParticlesObject* particles = input.getObject<ParticlesObject>())
				return particles->bonds() != nullptr;
			return false;
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

	/// \brief Returns the class of property containers this delegate operates on.
	virtual const PropertyContainerClass& containerClass() const override { return BondsObject::OOClass(); }

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
				const PipelineFlowState& input);

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Returns a human-readable text listing the input variables.
		virtual QString inputVariableTable() const override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	private:

		ParticleOrderingFingerprint _inputFingerprint;
		ConstPropertyPtr _topology;
	};
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
