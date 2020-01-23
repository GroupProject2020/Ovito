////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
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
#include <ovito/particles/export/lammps/LAMMPSDataExporter.h>
#include <ovito/particles/import/lammps/LAMMPSDataImporter.h>
#include <ovito/gui/desktop/properties/VariantComboBoxParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerParameterUI.h>
#include "LAMMPSDataExporterEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(LAMMPSDataExporterEditor);
SET_OVITO_OBJECT_EDITOR(LAMMPSDataExporter, LAMMPSDataExporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void LAMMPSDataExporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("LAMMPS Data File"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1,1);
	layout->setColumnStretch(4,1);
	layout->setColumnMinimumWidth(2,10);
	layout->addWidget(new QLabel(tr("LAMMPS atom style:")), 0, 0);

	VariantComboBoxParameterUI* atomStyleUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(LAMMPSDataExporter::atomStyle));
	atomStyleUI->comboBox()->addItem("angle", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Angle));
	atomStyleUI->comboBox()->addItem("atomic", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Atomic));
	atomStyleUI->comboBox()->addItem("bond", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Bond));
	atomStyleUI->comboBox()->addItem("charge", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Charge));
	atomStyleUI->comboBox()->addItem("dipole", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Dipole));
	atomStyleUI->comboBox()->addItem("full", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Full));
	atomStyleUI->comboBox()->addItem("molecular", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Molecular));
	atomStyleUI->comboBox()->addItem("sphere", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Sphere));
	layout->addWidget(atomStyleUI->comboBox(), 0, 1);

	IntegerParameterUI* precisionUI = new IntegerParameterUI(this, PROPERTY_FIELD(FileExporter::floatOutputPrecision));
	layout->addWidget(precisionUI->label(), 0, 3);
	layout->addLayout(precisionUI->createFieldLayout(), 0, 4);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
