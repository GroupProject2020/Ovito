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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/SlipSurfaceVis.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanGroupBoxParameterUI.h>
#include "SlipSurfaceVisEditor.h"

namespace Ovito { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(SlipSurfaceVisEditor);
SET_OVITO_OBJECT_EDITOR(SlipSurfaceVis, SlipSurfaceVisEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SlipSurfaceVisEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(QString(), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* surfaceGroupBox = new QGroupBox(tr("Surface display"));
	QGridLayout* sublayout = new QGridLayout(surfaceGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(surfaceGroupBox);

	FloatParameterUI* surfaceTransparencyUI = new FloatParameterUI(this, PROPERTY_FIELD(SlipSurfaceVis::surfaceTransparencyController));
	sublayout->addWidget(new QLabel(tr("Transparency:")), 0, 0);
	sublayout->addLayout(surfaceTransparencyUI->createFieldLayout(), 0, 1);

	BooleanParameterUI* smoothShadingUI = new BooleanParameterUI(this, PROPERTY_FIELD(SlipSurfaceVis::smoothShading));
	sublayout->addWidget(smoothShadingUI->checkBox(), 1, 0, 1, 2);

	BooleanParameterUI* highlightEdgesUI = new BooleanParameterUI(this, PROPERTY_FIELD(SlipSurfaceVis::highlightEdges));
	sublayout->addWidget(highlightEdgesUI->checkBox(), 2, 0, 1, 2);
}

}	// End of namespace
}	// End of namespace
