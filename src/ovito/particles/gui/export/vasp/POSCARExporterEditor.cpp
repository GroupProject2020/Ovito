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
#include <ovito/particles/export/vasp/POSCARExporter.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include "POSCARExporterEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(POSCARExporterEditor);
SET_OVITO_OBJECT_EDITOR(POSCARExporter, POSCARExporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void POSCARExporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("POSCAR format"), rolloutParams);

    // Create the rollout contents.
	QHBoxLayout* layout = new QHBoxLayout(rollout);
	layout->setContentsMargins(6,6,6,6);
	layout->setSpacing(4);

	BooleanParameterUI* reducedCoordsUI = new BooleanParameterUI(this, PROPERTY_FIELD(POSCARExporter::writeReducedCoordinates));
	layout->addWidget(reducedCoordsUI->checkBox());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
