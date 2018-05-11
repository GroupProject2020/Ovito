///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/export/lammps/LAMMPSDumpExporter.h>
#include <gui/properties/IntegerParameterUI.h>
#include "LAMMPSDumpExporterEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(LAMMPSDumpExporterEditor);
SET_OVITO_OBJECT_EDITOR(LAMMPSDumpExporter, LAMMPSDumpExporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void LAMMPSDumpExporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("LAMMPS Dump File"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(2,1);

	IntegerParameterUI* precisionUI = new IntegerParameterUI(this, PROPERTY_FIELD(FileExporter::floatOutputPrecision));
	layout->addWidget(precisionUI->label(), 0, 0);
	layout->addLayout(precisionUI->createFieldLayout(), 0, 1);

	FileColumnParticleExporterEditor::createUI(rolloutParams.before(rollout));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
