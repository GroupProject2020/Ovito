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
#include <gui/properties/BooleanParameterUI.h>
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
	QWidget* rollout = createRollout(tr("Particle Type"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);

	QGroupBox* nameBox = new QGroupBox(tr("Particle type"), rollout);
	QGridLayout* gridLayout = new QGridLayout(nameBox);
	gridLayout->setContentsMargins(4,4,4,4);
	gridLayout->setColumnStretch(1, 1);
	layout1->addWidget(nameBox);

	// Name.
	StringParameterUI* namePUI = new StringParameterUI(this, PROPERTY_FIELD(ParticleType::name));
	gridLayout->addWidget(new QLabel(tr("Name:")), 0, 0);
	gridLayout->addWidget(namePUI->textBox(), 0, 1);

	// Numeric ID.
	gridLayout->addWidget(new QLabel(tr("Numeric ID:")), 1, 0);
	QLabel* numericIdLabel = new QLabel();
	gridLayout->addWidget(numericIdLabel, 1, 1);
	connect(this, &PropertiesEditor::contentsReplaced, [numericIdLabel](RefTarget* newEditObject) {
		if(ElementType* ptype = static_object_cast<ElementType>(newEditObject))
			numericIdLabel->setText(QString::number(ptype->numericId()));
		else
			numericIdLabel->setText({});
	});

	QGroupBox* appearanceBox = new QGroupBox(tr("Appearance"), rollout);
	gridLayout = new QGridLayout(appearanceBox);
	gridLayout->setContentsMargins(4,4,4,4);
	gridLayout->setColumnStretch(1, 1);
	layout1->addWidget(appearanceBox);

	// Display color parameter.
	ColorParameterUI* colorPUI = new ColorParameterUI(this, PROPERTY_FIELD(ParticleType::color));
	gridLayout->addWidget(colorPUI->label(), 0, 0);
	gridLayout->addWidget(colorPUI->colorPicker(), 0, 1);

	// Display radius parameter.
	FloatParameterUI* radiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(ParticleType::radius));
	gridLayout->addWidget(radiusPUI->label(), 1, 0);
	gridLayout->addLayout(radiusPUI->createFieldLayout(), 1, 1);

	// "Save as defaults" button
	QPushButton* setAsDefaultBtn = new QPushButton(tr("Save as defaults"));
	setAsDefaultBtn->setToolTip(tr("Saves the current color and radius values as defaults for this particle type."));
	setAsDefaultBtn->setEnabled(false);
	gridLayout->addWidget(setAsDefaultBtn, 2, 0, 1, 2, Qt::AlignRight);
	connect(setAsDefaultBtn, &QPushButton::clicked, this, [this]() {
		ParticleType* ptype = static_object_cast<ParticleType>(editObject());
		if(!ptype) return;

		ParticleType::setDefaultParticleColor(ParticlesObject::TypeProperty, ptype->nameOrNumericId(), ptype->color());
		ParticleType::setDefaultParticleRadius(ParticlesObject::TypeProperty, ptype->nameOrNumericId(), ptype->radius());

		mainWindow()->statusBar()->showMessage(tr("Stored current color and radius as defaults for particle type '%1'.").arg(ptype->nameOrNumericId()), 4000);
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

	QGroupBox* userShapeBox = new QGroupBox(tr("User-defined particle shape"), rollout);
	gridLayout = new QGridLayout(userShapeBox);
	gridLayout->setContentsMargins(4,4,4,4);
	gridLayout->setSpacing(2);
	layout1->addWidget(userShapeBox);

	// User-defined shape.
	QLabel* userShapeLabel = new QLabel();
	gridLayout->addWidget(userShapeLabel, 0, 0, 1, 2);
	QPushButton* loadShapeBtn = new QPushButton(tr("Load shape..."));
	loadShapeBtn->setToolTip(tr("Select a mesh geometry file to use as particle shape."));
	loadShapeBtn->setEnabled(false);
	gridLayout->addWidget(loadShapeBtn, 1, 0);
	QPushButton* resetShapeBtn = new QPushButton(tr("Remove"));
	resetShapeBtn->setToolTip(tr("Reset the particle shape back to the built-in one."));
	resetShapeBtn->setEnabled(false);
	gridLayout->addWidget(resetShapeBtn, 1, 1);
	BooleanParameterUI* highlightEdgesUI = new BooleanParameterUI(this, PROPERTY_FIELD(ParticleType::highlightShapeEdges));
	gridLayout->addWidget(highlightEdgesUI->checkBox(), 2, 0, 1, 1);
	BooleanParameterUI* shapeBackfaceCullingUI = new BooleanParameterUI(this, PROPERTY_FIELD(ParticleType::shapeBackfaceCullingEnabled));
	gridLayout->addWidget(shapeBackfaceCullingUI->checkBox(), 2, 1, 1, 1);

	// Update the shape buttons whenever the particle type is being modified.
	connect(this, &PropertiesEditor::contentsChanged, this, [=](RefTarget* editObject) {
		if(ParticleType* ptype = static_object_cast<ParticleType>(editObject)) {
			loadShapeBtn->setEnabled(true);
			resetShapeBtn->setEnabled(ptype->shapeMesh() != nullptr);
			if(ptype->shapeMesh())
				userShapeLabel->setText(tr("Assigned mesh: %1 faces/%2 vertices").arg(ptype->shapeMesh()->mesh().faceCount()).arg(ptype->shapeMesh()->mesh().vertexCount()));
			else
				userShapeLabel->setText(tr("No user-defined shape assigned"));
			highlightEdgesUI->setEnabled(ptype->shapeMesh() != nullptr);
			shapeBackfaceCullingUI->setEnabled(ptype->shapeMesh() != nullptr);
		}
		else {
			loadShapeBtn->setEnabled(false);
			resetShapeBtn->setEnabled(false);
			userShapeLabel->setText({});
		}
	});

	// Implement shape load button.
	connect(loadShapeBtn, &QPushButton::clicked, this, [this]() {
		if(OORef<ParticleType> ptype = static_object_cast<ParticleType>(editObject())) {

			undoableTransaction(tr("Set particle shape"), [&]() {
				QStringList selectedFiles;
				// Put code in a block: Need to release dialog before loading the input file.
				{
					HistoryFileDialog fileDialog(QStringLiteral("particle_shape_mesh"), container(), tr("Pick geometry file"),
						QString(), tr("Mesh Files (*.obj *.stl *.vtk)"));
					fileDialog.setFileMode(QFileDialog::ExistingFile);

					if(!fileDialog.exec())
						return;
					selectedFiles = fileDialog.selectedFiles();
				}
				if(!selectedFiles.empty()) {
					ProgressDialog progressDialog(container(), ptype->dataset()->taskManager(), tr("Loading mesh file"));
					ptype->loadShapeMesh(selectedFiles.front(), progressDialog.taskManager());
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
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
