////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/grid/objects/VoxelGridVis.h>
#include "VoxelGridVisEditor.h"

namespace Ovito { namespace Grid { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(VoxelGridVisEditor);
SET_OVITO_OBJECT_EDITOR(VoxelGridVis, VoxelGridVisEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void VoxelGridVisEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Grid display"), rolloutParams, "visual_elements.voxel_grid.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	FloatParameterUI* transparencyUI = new FloatParameterUI(this, PROPERTY_FIELD(VoxelGridVis::transparencyController));
	layout->addWidget(transparencyUI->label(), 1, 0);
	layout->addLayout(transparencyUI->createFieldLayout(), 1, 1);

	BooleanParameterUI* highlightLinesUI = new BooleanParameterUI(this, PROPERTY_FIELD(VoxelGridVis::highlightGridLines));
	layout->addWidget(highlightLinesUI->checkBox(), 2, 0, 1, 2);

	BooleanParameterUI* interpolateColorsUI = new BooleanParameterUI(this, PROPERTY_FIELD(VoxelGridVis::interpolateColors));
	layout->addWidget(interpolateColorsUI->checkBox(), 3, 0, 1, 2);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
