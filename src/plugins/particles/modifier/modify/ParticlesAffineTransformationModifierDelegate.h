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


#include <plugins/particles/Particles.h>
#include <plugins/stdmod/modifiers/AffineTransformationModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

/**
 * \brief Delegate for the AffineTransformationModifier that operates on particles.
 */
class ParticlesAffineTransformationModifierDelegate : public AffineTransformationModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public AffineTransformationModifierDelegate::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using AffineTransformationModifierDelegate::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier delegate can operate on the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("particles"); }
	};	

	Q_OBJECT
	OVITO_CLASS_META(ParticlesAffineTransformationModifierDelegate, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Particles");

public:

	/// Constructor.
	Q_INVOKABLE ParticlesAffineTransformationModifierDelegate(DataSet* dataset) : AffineTransformationModifierDelegate(dataset) {}

	/// Applies the modifier operation to the data in a pipeline flow state.
	virtual PipelineStatus apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs) override;
};

/**
 * \brief Delegate for the AffineTransformationModifier that operates on vector particle properties.
 */
class VectorParticlePropertiesAffineTransformationModifierDelegate : public AffineTransformationModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public AffineTransformationModifierDelegate::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using AffineTransformationModifierDelegate::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier delegate can operate on the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("vector_properties"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(VectorParticlePropertiesAffineTransformationModifierDelegate, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Vector particle properties");

public:

	/// Constructor.
	Q_INVOKABLE VectorParticlePropertiesAffineTransformationModifierDelegate(DataSet* dataset) : AffineTransformationModifierDelegate(dataset) {}

	/// Applies the modifier operation to the data in a pipeline flow state.
	virtual PipelineStatus apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs) override;

private:

	/// Decides if the given particle property is one that should be transformed.
	static bool isTransformableProperty(const PropertyObject* property);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
