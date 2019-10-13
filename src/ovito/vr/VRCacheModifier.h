////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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


#include <ovito/core/Core.h>
#include <ovito/core/dataset/pipeline/Modifier.h>

namespace VRPlugin {

using namespace Ovito;

/**
 * \brief A modifier that caches the results of the data pipeline.
 */
class VRCacheModifier : public Modifier
{
	Q_OBJECT
	OVITO_CLASS(VRCacheModifier)

	Q_CLASSINFO("DisplayName", "VR Display Cache");
	Q_CLASSINFO("ModifierCategory", "VR");

public:

	/// \brief Default constructor.
	Q_INVOKABLE VRCacheModifier(DataSet* dataset) : Modifier(dataset) {}

	/// \brief This modifies the input object in a specific way.
	virtual PipelineStatus modifyObject(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// Asks the modifier whether it can be applied to the given input data.
	virtual bool isApplicableTo(const DataCollection& input) override { return true; }

private:

	/// The cached state.
	PipelineFlowState _cache;
};

}	// End of namespace
