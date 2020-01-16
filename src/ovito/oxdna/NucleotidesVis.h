////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/particles/objects/ParticlesVis.h>

namespace Ovito { namespace Particles {

/**
 * \brief A visualization element for rendering DNA nucleotides.
 */
class OVITO_OXDNA_EXPORT NucleotidesVis : public ParticlesVis
{
	Q_OBJECT
	OVITO_CLASS(NucleotidesVis)
	Q_CLASSINFO("DisplayName", "Nucleotides");

public:

	/// Constructor.
	Q_INVOKABLE NucleotidesVis(DataSet* dataset);

	/// Renders the visual element.
	virtual void render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode) override;

	/// Computes the bounding box of the visual element.
	virtual Box3 boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// Determines the effective rendering colors for the backbone sites of the nucleotides.
	void backboneColors(std::vector<ColorA>& output, const ParticlesObject* particles, bool highlightSelection) const;

	/// Determines the effective rendering colors for the base sites of the nucleotides.
	void nucleobaseColors(std::vector<ColorA>& output, const ParticlesObject* particles, bool highlightSelection) const;
};

}	// End of namespace
}	// End of namespace
