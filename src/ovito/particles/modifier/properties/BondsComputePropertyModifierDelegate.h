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
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/util/ParticleExpressionEvaluator.h>
#include <ovito/particles/util/ParticleOrderingFingerprint.h>
#include <ovito/stdmod/modifiers/ComputePropertyModifier.h>

namespace Ovito { namespace Particles {

using namespace Ovito::StdMod;

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

		/// Indicates which data objects in the given input data collection the modifier delegate is able to operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const override;

		/// Indicates which class of data objects the modifier delegate is able to operate on.
		virtual const DataObject::OOMetaClass& getApplicableObjectClass() const override { return BondsObject::OOClass(); }

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("bonds"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(BondsComputePropertyModifierDelegate, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Bonds");

public:

	/// Constructor.
	Q_INVOKABLE BondsComputePropertyModifierDelegate(DataSet* dataset) : ComputePropertyModifierDelegate(dataset) {}

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

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_topology.reset();
			PropertyComputeEngine::cleanup();
		}

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

}	// End of namespace
}	// End of namespace
