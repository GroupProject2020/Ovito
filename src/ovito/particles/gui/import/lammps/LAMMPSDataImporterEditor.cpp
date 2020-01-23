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
#include <ovito/particles/import/lammps/LAMMPSDataImporter.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include "LAMMPSDataImporterEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(LAMMPSDataImporterEditor);
SET_OVITO_OBJECT_EDITOR(LAMMPSDataImporter, LAMMPSDataImporterEditor);

/******************************************************************************
* This method is called by the FileSource each time a new source
* file has been selected by the user.
******************************************************************************/
bool LAMMPSDataImporterEditor::inspectNewFile(FileImporter* importer, const QUrl& sourceFile, QWidget* parent)
{
	LAMMPSDataImporter* dataImporter = static_object_cast<LAMMPSDataImporter>(importer);

	// Inspect the data file and try to detect the LAMMPS atom style.
	Future<LAMMPSDataImporter::LAMMPSAtomStyle> inspectFuture = dataImporter->inspectFileHeader(FileSourceImporter::Frame(sourceFile));
	if(!importer->dataset()->taskManager().waitForFuture(inspectFuture))
		return false;
	LAMMPSDataImporter::LAMMPSAtomStyle detectedAtomStyle = inspectFuture.result();

	// Show dialog to ask user for the right LAMMPS atom style if it could not be detected.
	if(detectedAtomStyle == LAMMPSDataImporter::AtomStyle_Unknown) {

		QMap<QString, LAMMPSDataImporter::LAMMPSAtomStyle> styleList = {
				{ QStringLiteral("angle"), LAMMPSDataImporter::AtomStyle_Angle },
				{ QStringLiteral("atomic"), LAMMPSDataImporter::AtomStyle_Atomic },
				{ QStringLiteral("body"), LAMMPSDataImporter::AtomStyle_Body },
				{ QStringLiteral("bond"), LAMMPSDataImporter::AtomStyle_Bond },
				{ QStringLiteral("charge"), LAMMPSDataImporter::AtomStyle_Charge },
				{ QStringLiteral("dipole"), LAMMPSDataImporter::AtomStyle_Dipole },
				{ QStringLiteral("dpd"), LAMMPSDataImporter::AtomStyle_DPD },
				{ QStringLiteral("edpd"), LAMMPSDataImporter::AtomStyle_EDPD },
				{ QStringLiteral("mdpd"), LAMMPSDataImporter::AtomStyle_MDPD },
				{ QStringLiteral("electron"), LAMMPSDataImporter::AtomStyle_Electron },
				{ QStringLiteral("ellipsoid"), LAMMPSDataImporter::AtomStyle_Ellipsoid },
				{ QStringLiteral("full"), LAMMPSDataImporter::AtomStyle_Full },
				{ QStringLiteral("line"), LAMMPSDataImporter::AtomStyle_Line },
				{ QStringLiteral("meso"), LAMMPSDataImporter::AtomStyle_Meso },
				{ QStringLiteral("molecular"), LAMMPSDataImporter::AtomStyle_Molecular },
				{ QStringLiteral("peri"), LAMMPSDataImporter::AtomStyle_Peri },
				{ QStringLiteral("smd"), LAMMPSDataImporter::AtomStyle_SMD },
				{ QStringLiteral("sphere"), LAMMPSDataImporter::AtomStyle_Sphere },
				{ QStringLiteral("template"), LAMMPSDataImporter::AtomStyle_Template },
				{ QStringLiteral("tri"), LAMMPSDataImporter::AtomStyle_Tri },
				{ QStringLiteral("wavepacket"), LAMMPSDataImporter::AtomStyle_Wavepacket }
		};
		QStringList itemList = styleList.keys();

		QSettings settings;
		settings.beginGroup(LAMMPSDataImporter::OOClass().plugin()->pluginId());
		settings.beginGroup(LAMMPSDataImporter::OOClass().name());

		int currentIndex = -1;
		for(int i = 0; i < itemList.size(); i++)
			if(dataImporter->atomStyle() == styleList[itemList[i]])
				currentIndex = i;
		if(currentIndex == -1)
			currentIndex = itemList.indexOf(settings.value("DefaultAtomStyle").toString());
		if(currentIndex == -1)
			currentIndex = itemList.indexOf("atomic");

		bool ok;
		QString selectedItem = QInputDialog::getItem(parent, tr("LAMMPS data file"), tr("Please select the LAMMPS atom style used by the data file:"), itemList, currentIndex, false, &ok);
		if(!ok) return false;

		settings.setValue("DefaultAtomStyle", selectedItem);
		dataImporter->setAtomStyle(styleList[selectedItem]);
	}
	else {
		dataImporter->setAtomStyle(detectedAtomStyle);
	}

	return true;
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void LAMMPSDataImporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("LAMMPS data reader"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* optionsBox = new QGroupBox(tr("Options"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(optionsBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(optionsBox);

	// Sort particles
	BooleanParameterUI* sortParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(ParticleImporter::sortParticles));
	sublayout->addWidget(sortParticlesUI->checkBox());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
