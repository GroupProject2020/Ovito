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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/import/gsd/GSDImporter.h>
#include <ovito/gui/properties/IntegerParameterUI.h>
#include "GSDImporterEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(GSDImporterEditor);
SET_OVITO_OBJECT_EDITOR(GSDImporter, GSDImporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void GSDImporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("GSD reader"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* optionsBox = new QGroupBox(tr("Options"), rollout);
	QGridLayout* sublayout = new QGridLayout(optionsBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(6);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(optionsBox);

	// Rounding resolution
	IntegerParameterUI* resolutionUI = new IntegerParameterUI(this, PROPERTY_FIELD(GSDImporter::roundingResolution));
	sublayout->addWidget(resolutionUI->label(), 1, 0);
	sublayout->addLayout(resolutionUI->createFieldLayout(), 1, 1);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
