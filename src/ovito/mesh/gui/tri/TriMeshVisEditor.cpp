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

#include <ovito/gui/GUI.h>
#include <ovito/gui/properties/ColorParameterUI.h>
#include <ovito/gui/properties/FloatParameterUI.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include <ovito/mesh/tri/TriMeshVis.h>
#include "TriMeshVisEditor.h"

namespace Ovito { namespace Mesh { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(TriMeshVisEditor);
SET_OVITO_OBJECT_EDITOR(TriMeshVis, TriMeshVisEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void TriMeshVisEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Triangle mesh display"), rolloutParams, "visual_elements.triangle_mesh.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	ColorParameterUI* colorUI = new ColorParameterUI(this, PROPERTY_FIELD(TriMeshVis::color));
	layout->addWidget(colorUI->label(), 0, 0);
	layout->addWidget(colorUI->colorPicker(), 0, 1);

	FloatParameterUI* transparencyUI = new FloatParameterUI(this, PROPERTY_FIELD(TriMeshVis::transparencyController));
	layout->addWidget(new QLabel(tr("Transparency:")), 1, 0);
	layout->addLayout(transparencyUI->createFieldLayout(), 1, 1);

	BooleanParameterUI* highlightEdgesUI = new BooleanParameterUI(this, PROPERTY_FIELD(TriMeshVis::highlightEdges));
	layout->addWidget(highlightEdgesUI->checkBox(), 2, 0, 1, 2);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
