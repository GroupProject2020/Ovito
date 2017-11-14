///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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

#include <gui/GUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/BooleanGroupBoxParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <plugins/ospray/renderer/OSPRayBackend.h>
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
	QWidget* rollout = createRollout(tr("SciVis settings"), rolloutParams);

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
