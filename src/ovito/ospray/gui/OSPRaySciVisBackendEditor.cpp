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
#include <ovito/gui/properties/BooleanParameterUI.h>
#include <ovito/gui/properties/BooleanGroupBoxParameterUI.h>
#include <ovito/gui/properties/IntegerParameterUI.h>
#include <ovito/gui/properties/FloatParameterUI.h>
#include <ovito/ospray/renderer/OSPRayBackend.h>
#include "OSPRaySciVisBackendEditor.h"

namespace Ovito { namespace OSPRay { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(OSPRaySciVisBackendEditor);
SET_OVITO_OBJECT_EDITOR(OSPRaySciVisBackend, OSPRaySciVisBackendEditor);

/******************************************************************************
* Creates the UI controls for the editor.
******************************************************************************/
void OSPRaySciVisBackendEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("SciVis settings"), rolloutParams, "rendering.ospray_renderer.html");

	QVBoxLayout* mainLayout = new QVBoxLayout(rollout);
	mainLayout->setContentsMargins(4,4,4,4);

	// Shadows.
	BooleanParameterUI* enableShadowsUI = new BooleanParameterUI(this, PROPERTY_FIELD(OSPRaySciVisBackend::shadowsEnabled));
	mainLayout->addWidget(enableShadowsUI->checkBox());

	// Ambient occlusion.
	BooleanGroupBoxParameterUI* enableAmbientOcclusionUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(OSPRaySciVisBackend::ambientOcclusionEnabled));
	QGroupBox* aoGroupBox = enableAmbientOcclusionUI->groupBox();
	mainLayout->addWidget(aoGroupBox);

	QGridLayout* layout = new QGridLayout(enableAmbientOcclusionUI->childContainer());
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	// Ambient occlusion samples.
	IntegerParameterUI* aoSamplesUI = new IntegerParameterUI(this, PROPERTY_FIELD(OSPRaySciVisBackend::ambientOcclusionSamples));
	aoSamplesUI->label()->setText(tr("Sample count:"));
	layout->addWidget(aoSamplesUI->label(), 0, 0);
	layout->addLayout(aoSamplesUI->createFieldLayout(), 0, 1);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
