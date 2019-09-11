///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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


#include <ovito/stdmod/StdMod.h>
#include <ovito/core/dataset/pipeline/DelegatingModifier.h>

namespace Ovito { namespace StdMod {

/**
 * \brief Base class for AffineTransformationModifier delegates that operate on different kinds of data.
 */
class OVITO_STDMOD_EXPORT AffineTransformationModifierDelegate : public ModifierDelegate
{
	Q_OBJECT
	OVITO_CLASS(AffineTransformationModifierDelegate)

protected:

	/// Abstract class constructor.
	using ModifierDelegate::ModifierDelegate;
};

/**
 * \brief Delegate for the AffineTransformationModifier that operates on simulation cells.
 */
class OVITO_STDMOD_EXPORT SimulationCellAffineTransformationModifierDelegate : public AffineTransformationModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public AffineTransformationModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using AffineTransformationModifierDelegate::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier delegate can operate on the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("cell"); }
	};

	OVITO_CLASS_META(SimulationCellAffineTransformationModifierDelegate, OOMetaClass)
	Q_CLASSINFO("DisplayName", "Simulation cell");
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE SimulationCellAffineTransformationModifierDelegate(DataSet* dataset) : AffineTransformationModifierDelegate(dataset) {}

	/// Applies the modifier operation to the data in a pipeline flow state.
	virtual PipelineStatus apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs) override;
};

/**
 * \brief This modifier applies an arbitrary affine transformation to the
 *        particles, the simulation box and other entities.
 *
 * The affine transformation is specified as a 3x4 matrix.
 */
class OVITO_STDMOD_EXPORT AffineTransformationModifier : public MultiDelegatingModifier
{
public:

	/// Give this modifier class its own metaclass.
	class OOMetaClass : public MultiDelegatingModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using MultiDelegatingModifier::OOMetaClass::OOMetaClass;

		/// Return the metaclass of delegates for this modifier type.
		virtual const ModifierDelegate::OOMetaClass& delegateMetaclass() const override { return AffineTransformationModifierDelegate::OOClass(); }
	};

	OVITO_CLASS_META(AffineTransformationModifier, OOMetaClass)
	Q_CLASSINFO("DisplayName", "Affine transformation");
	Q_CLASSINFO("ModifierCategory", "Modification");
	Q_OBJECT

public:

	/// \brief Constructor.
	Q_INVOKABLE AffineTransformationModifier(DataSet* dataset);

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// Modifies the input data in an immediate, preliminary way.
	virtual void evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

protected:

	/// This property fields stores the transformation matrix (used in 'relative' mode).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(AffineTransformation, transformationTM, setTransformationTM);

	/// This property fields stores the simulation cell geometry (used in 'absolute' mode).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(AffineTransformation, targetCell, setTargetCell);

	/// This controls whether the transformation is applied only to the selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, selectionOnly, setSelectionOnly);

	/// This controls whether a relative transformation is applied to the simulation box or
	/// the absolute cell geometry has been specified.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, relativeMode, setRelativeMode);
};

}	// End of namespace
}	// End of namespace
