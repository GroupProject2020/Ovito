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


#include <core/Core.h>
#include <core/dataset/pipeline/modifiers/GenericPropertyModifier.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief This modifier inverts the selection status of each element.
 */
class OVITO_CORE_EXPORT InvertSelectionModifier : public GenericPropertyModifier
{
	Q_OBJECT
	OVITO_CLASS(InvertSelectionModifier)
	Q_CLASSINFO("DisplayName", "Invert selection");
	Q_CLASSINFO("ModifierCategory", "Selection");
	
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE InvertSelectionModifier(DataSet* dataset);

	/// Modifies the input data in an immediate, preliminary way.
	virtual PipelineFlowState evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
