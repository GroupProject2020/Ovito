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
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/stdmod/modifiers/SpatialBinningModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief Delegate plugin for the SpatialBinningModifier that operates on particles.
 */
class ParticlesSpatialBinningModifierDelegate : public SpatialBinningModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public SpatialBinningModifierDelegate::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using SpatialBinningModifierDelegate::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier delegate can operate on the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override {
			return input.findObject<ParticleProperty>() != nullptr;
		}

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("particles"); }
	};	

	Q_OBJECT
	OVITO_CLASS_META(ParticlesSpatialBinningModifierDelegate, OOMetaClass);

	Q_CLASSINFO("DisplayName", "Particles");

public:

	/// Constructor.
	Q_INVOKABLE ParticlesSpatialBinningModifierDelegate(DataSet* dataset);

	/// \brief Returns the class of data elements this delegate operates on.
	virtual const PropertyClass& propertyClass() const override { return ParticleProperty::OOClass(); }

	/// Creates a computation engine that will perform the actual binning of elements.
	virtual std::shared_ptr<SpatialBinningModifierDelegate::SpatialBinningEngine> createEngine(
				TimePoint time, 
				const PipelineFlowState& input,
				const SimulationCell& cell,
				ConstPropertyPtr sourceProperty,
				size_t sourceComponent, 
				ConstPropertyPtr selectionProperty,
				PropertyPtr binData,
				const Vector3I& binCount,
				const Vector3I& binDir,
				int reductionOperation,
                bool computeFirstDerivative) override;

private:

	/// Asynchronous compute engine that does the actual work in a separate thread.
	class ComputeEngine : public SpatialBinningModifierDelegate::SpatialBinningEngine
	{
	public:

		/// Constructor.
		ComputeEngine(
				const TimeInterval& validityInterval, 
				const SimulationCell& cell,
				ConstPropertyPtr sourceProperty,
				size_t sourceComponent, 
				ConstPropertyPtr selection, 
				ConstPropertyPtr positions,
				PropertyPtr binData,
				const Vector3I& binCount,
				const Vector3I& binDir,
				int reductionOperation,
				bool computeFirstDerivative) :
			SpatialBinningEngine(validityInterval, cell, std::move(sourceProperty), sourceComponent, std::move(selection), std::move(binData), binCount, binDir, reductionOperation, computeFirstDerivative), 
			_positions(std::move(positions)) {}
				
		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_positions.reset();
			SpatialBinningEngine::cleanup();
		}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Returns the property storage that contains the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

	private:

		ConstPropertyPtr _positions;
	};
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
