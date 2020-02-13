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
#include <ovito/particles/export/vasp/POSCARExporter.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include "POSCARExporterEditor.h"

namespace Ovito { namespace Particles {

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

}	// End of namespace
}	// End of namespace
