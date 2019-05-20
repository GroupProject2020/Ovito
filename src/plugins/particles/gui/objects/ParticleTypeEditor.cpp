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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/objects/ParticleType.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <gui/properties/ColorParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/StringParameterUI.h>
#include <gui/dialogs/HistoryFileDialog.h>
#include <gui/utilities/concurrent/ProgressDialog.h>
#include <gui/mainwin/MainWindow.h>
#include "ParticleTypeEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(ParticleTypeEditor);
SET_OVITO_OBJECT_EDITOR(ParticleType, ParticleTypeEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ParticleTypeEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Atom Type"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout1 = new QGridLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
#ifndef Q_OS_MACX
	layout1->setSpacing(0);
#endif
	layout1->setColumnStretch(1, 1);
	
	// Text box for the name of particle type.
	StringParameterUI* namePUI = new StringParameterUI(this, PROPERTY_FIELD(ParticleType::name));
	layout1->addWidget(new QLabel(tr("Name:")), 0, 0);
	layout1->addWidget(namePUI->textBox(), 0, 1);
	
	// Display color parameter.
	ColorParameterUI* colorPUI = new ColorParameterUI(this, PROPERTY_FIELD(ParticleType::color));
	layout1->addWidget(colorPUI->label(), 1, 0);
	layout1->addWidget(colorPUI->colorPicker(), 1, 1);
   
	// Display radius parameter.
	FloatParameterUI* radiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(ParticleType::radius));
	layout1->addWidget(radiusPUI->label(), 2, 0);
	layout1->addLayout(radiusPUI->createFieldLayout(), 2, 1);

	// Numeric ID.
	IntegerParameterUI* idPUI = new IntegerParameterUI(this, PROPERTY_FIELD(ParticleType::numericId));
	idPUI->setEnabled(false);
	layout1->addWidget(new QLabel(tr("Numeric ID:")), 3, 0);
	layout1->addWidget(idPUI->textBox(), 3, 1);

	// User-defined shape.
	layout1->addWidget(new QLabel(tr("Shape:")), 4, 0);
	QHBoxLayout* sublayout = new QHBoxLayout();
	sublayout->setContentsMargins(0,0,0,0);
	sublayout->setSpacing(4);
	QPushButton* loadShapeBtn = new QPushButton(tr("Load..."));
	loadShapeBtn->setToolTip(tr("Select a mesh geometry file to use as particle shape."));
	loadShapeBtn->setEnabled(false);
	sublayout->addWidget(loadShapeBtn);
	QPushButton* resetShapeBtn = new QPushButton(tr("Reset"));
	resetShapeBtn->setToolTip(tr("Resets the particle shape back to the default one."));
	resetShapeBtn->setEnabled(false);
	sublayout->addWidget(resetShapeBtn);
	layout1->addLayout(sublayout, 4, 1);

	// Update the shape buttons whenever the particle type is being modified.
	connect(this, &PropertiesEditor::contentsChanged, this, [=](RefTarget* editObject) {
		if(ParticleType* ptype = static_object_cast<ParticleType>(editObject)) {
			loadShapeBtn->setEnabled(true);
			resetShapeBtn->setEnabled(ptype->shapeMesh() != nullptr);
		}
		else {
			loadShapeBtn->setEnabled(false);
			resetShapeBtn->setEnabled(false);
		}
	});

	// Implement shape load button.
	connect(loadShapeBtn, &QPushButton::clicked, this, [this]() {
		if(OORef<ParticleType> ptype = static_object_cast<ParticleType>(editObject())) {

			undoableTransaction(tr("Set particle shape"), [&]() {
				HistoryFileDialog fileDialog(QStringLiteral("particle_shape_mesh"), container(), tr("Pick geometry file"),
					QString(), tr("VTK Mesh Files (*.vtk)"));
				fileDialog.setFileMode(QFileDialog::ExistingFile);

				if(fileDialog.exec()) {
					QStringList selectedFiles = fileDialog.selectedFiles();
					if(!selectedFiles.empty()) {
						ProgressDialog progressDialog(container(), ptype->dataset()->taskManager(), tr("Loading mesh file"));
						ptype->loadShapeMesh(selectedFiles.front(), progressDialog.taskManager());
					}
				}
			});
		}
	});

	// Implement shape reset button.
	connect(resetShapeBtn, &QPushButton::clicked, this, [this]() {
		if(ParticleType* ptype = static_object_cast<ParticleType>(editObject())) {
			undoableTransaction(tr("Reset particle shape"), [&]() {
				ptype->setShapeMesh(nullptr);
			});
		}
	});

	// "Set as default" button
	QPushButton* setAsDefaultBtn = new QPushButton(tr("Set as default"));
	setAsDefaultBtn->setToolTip(tr("Set current color and radius as defaults for this particle type."));
	setAsDefaultBtn->setEnabled(false);
	layout1->addWidget(setAsDefaultBtn, 5, 0, 1, 2, Qt::AlignRight);
	connect(setAsDefaultBtn, &QPushButton::clicked, this, [this]() {
		ParticleType* ptype = static_object_cast<ParticleType>(editObject());
		if(!ptype) return;

		ParticleType::setDefaultParticleColor(ParticlesObject::TypeProperty, ptype->name(), ptype->color());
		ParticleType::setDefaultParticleRadius(ParticlesObject::TypeProperty, ptype->name(), ptype->radius());

		mainWindow()->statusBar()->showMessage(tr("Stored current color and radius as defaults for particle type '%1'.").arg(ptype->name()), 4000);
	});
	connect(this, &PropertiesEditor::contentsReplaced, [setAsDefaultBtn,namePUI](RefTarget* newEditObject) {
		setAsDefaultBtn->setEnabled(newEditObject != nullptr);

		// Update the placeholder text of the name input field to reflect the numeric ID of the current particle type.
		if(QLineEdit* lineEdit = qobject_cast<QLineEdit*>(namePUI->textBox())) {
			if(ElementType* ptype = dynamic_object_cast<ElementType>(newEditObject))
				lineEdit->setPlaceholderText(QStringLiteral("[%1]").arg(ElementType::generateDefaultTypeName(ptype->numericId())));
			else
				lineEdit->setPlaceholderText({});
		}
	});
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
