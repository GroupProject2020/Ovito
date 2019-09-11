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


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/stdmod/modifiers/AssignColorModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

/**
 * \brief Function for the AssignColorModifier that operates on particles.
 */
class ParticlesAssignColorModifierDelegate : public AssignColorModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class ParticlesAssignColorModifierDelegateClass : public AssignColorModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using AssignColorModifierDelegate::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier delegate can operate on the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override {
			return input.containsObject<ParticlesObject>();
		}

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("particles"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(ParticlesAssignColorModifierDelegate, ParticlesAssignColorModifierDelegateClass)

	Q_CLASSINFO("DisplayName", "Particles");

public:

	/// Constructor.
	Q_INVOKABLE ParticlesAssignColorModifierDelegate(DataSet* dataset) : AssignColorModifierDelegate(dataset) {}

	/// \brief Returns the class of properties that can serve as input for the color coding.
	virtual const PropertyContainerClass& containerClass() const override { return ParticlesObject::OOClass(); }

protected:

	/// \brief returns the ID of the standard property that will receive the assigned colors.
	virtual int outputColorPropertyId() const override { return ParticlesObject::ColorProperty; }
};

/**
 * \brief Function for the AssignColorModifier that operates on particle vectors.
 */
class ParticleVectorsAssignColorModifierDelegate : public AssignColorModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class ParticleVectorsAssignColorModifierDelegateClass : public AssignColorModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using AssignColorModifierDelegate::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier delegate can operate on the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("vectors"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(ParticleVectorsAssignColorModifierDelegate, ParticleVectorsAssignColorModifierDelegateClass)

	Q_CLASSINFO("DisplayName", "Particle vectors");

public:

	/// Constructor.
	Q_INVOKABLE ParticleVectorsAssignColorModifierDelegate(DataSet* dataset) : AssignColorModifierDelegate(dataset) {}

	/// \brief Returns the class of properties that can serve as input for the color coding.
	virtual const PropertyContainerClass& containerClass() const override { return ParticlesObject::OOClass(); }

protected:

	/// \brief returns the ID of the standard property that will receive the assigned colors.
	virtual int outputColorPropertyId() const override { return ParticlesObject::VectorColorProperty; }
};

/**
 * \brief Function for the AssignColorModifier that operates on bonds.
 */
class BondsAssignColorModifierDelegate : public AssignColorModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class BondsAssignColorModifierDelegateClass : public AssignColorModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using AssignColorModifierDelegate::OOMetaClass::OOMetaClass;

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
	OVITO_CLASS_META(BondsAssignColorModifierDelegate, BondsAssignColorModifierDelegateClass)

	Q_CLASSINFO("DisplayName", "Bonds");

public:

	/// Constructor.
	Q_INVOKABLE BondsAssignColorModifierDelegate(DataSet* dataset) : AssignColorModifierDelegate(dataset) {}

	/// \brief Returns the class of properties that can serve as input for the color coding.
	virtual const PropertyContainerClass& containerClass() const override { return BondsObject::OOClass(); }

protected:

	/// \brief returns the ID of the standard property that will receive the computed colors.
	virtual int outputColorPropertyId() const override { return BondsObject::ColorProperty; }
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
