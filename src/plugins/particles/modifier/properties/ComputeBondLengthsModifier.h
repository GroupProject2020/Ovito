///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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
#include <core/dataset/pipeline/Modifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)

/**
 * \brief This modifier computes the length of bonds and stores them in a new bond property.
 */
class OVITO_PARTICLES_EXPORT ComputeBondLengthsModifier : public Modifier
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE ComputeBondLengthsModifier(DataSet* dataset);

	/// Modifies the input data in an immediate, preliminary way.
	virtual PipelineFlowState evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

public:
	
	/// Give this modifier class its own metaclass.
	class OOMetaClass : public Modifier::OOMetaClass 
	{
	public:

		/// Inherit constructor from base metaclass.
		using Modifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
	};

private:

	Q_OBJECT
	OVITO_CLASS

	Q_CLASSINFO("DisplayName", "Compute bond lengths");
	Q_CLASSINFO("ModifierCategory", "Modification");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


