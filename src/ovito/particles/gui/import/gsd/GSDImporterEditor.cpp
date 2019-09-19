///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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
