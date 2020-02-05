////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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


#include <ovito/stdmod/StdMod.h>
#include <ovito/stdobj/properties/GenericPropertyModifier.h>

namespace Ovito { namespace StdMod {

/**
 * \brief This modifier inverts the selection status of each element.
 */
class OVITO_STDMOD_EXPORT InvertSelectionModifier : public GenericPropertyModifier
{
	Q_OBJECT
	OVITO_CLASS(InvertSelectionModifier)
	Q_CLASSINFO("DisplayName", "Invert selection");
	Q_CLASSINFO("ModifierCategory", "Selection");

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE InvertSelectionModifier(DataSet* dataset);

	/// Modifies the input data synchronously.
	virtual void evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;
};

}	// End of namespace
}	// End of namespace
