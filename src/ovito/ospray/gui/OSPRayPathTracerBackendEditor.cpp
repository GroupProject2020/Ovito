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

#include <ovito/gui/GUI.h>
#include <ovito/gui/properties/IntegerParameterUI.h>
#include <ovito/ospray/renderer/OSPRayBackend.h>
#include "OSPRayPathTracerBackendEditor.h"

namespace Ovito { namespace OSPRay { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(OSPRayPathTracerBackendEditor);
SET_OVITO_OBJECT_EDITOR(OSPRayPathTracerBackend, OSPRayPathTracerBackendEditor);

/******************************************************************************
* Creates the UI controls for the editor.
******************************************************************************/
void OSPRayPathTracerBackendEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Path tracer settings"), rolloutParams, "rendering.ospray_renderer.html");

	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	// Roulette depth.
	IntegerParameterUI* rouletteDepthUI = new IntegerParameterUI(this, PROPERTY_FIELD(OSPRayPathTracerBackend::rouletteDepth));
	layout->addWidget(rouletteDepthUI->label(), 0, 0);
	layout->addLayout(rouletteDepthUI->createFieldLayout(), 0, 1);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
