///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2013) Alexander Stukowski
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

#include <core/Core.h>
#include <core/viewport/Viewport.h>
#include <core/gui/widgets/SpinnerWidget.h>
#include <core/utilities/units/UnitsManager.h>
#include "AdjustCameraDialog.h"

namespace Ovito {

/******************************************************************************
* The constructor of the viewport settings dialog.
******************************************************************************/
AdjustCameraDialog::AdjustCameraDialog(Viewport* viewport, QWidget* parent) :
	QDialog(parent), _viewport(viewport)
{
	setWindowTitle(tr("Adjust Camera"));
	
	_oldViewType = viewport->viewType();
	_oldCameraPos = viewport->cameraPosition();
	_oldCameraDir = viewport->cameraDirection();
	_oldFOV = viewport->fieldOfView();

	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	QGridLayout* gridLayout = new QGridLayout();
	gridLayout->setColumnStretch(1,1);
	gridLayout->setColumnStretch(2,1);
	gridLayout->setColumnStretch(3,1);

	_camPerspective = new QCheckBox(tr("Perspective projection"));
	connect(_camPerspective, SIGNAL(clicked(bool)), this, SLOT(onAdjustCamera()));
	connect(_camPerspective, SIGNAL(clicked(bool)), this, SLOT(updateGUI()));
	mainLayout->addWidget(_camPerspective);

	QHBoxLayout* fieldLayout;
	QLineEdit* textBox;

	gridLayout->addWidget(new QLabel(tr("Camera position:")), 0, 0);

	_camPosXSpinner = new SpinnerWidget();
	_camPosYSpinner = new SpinnerWidget();
	_camPosZSpinner = new SpinnerWidget();
	_camPosXSpinner->setUnit(UnitsManager::instance().worldUnit());
	_camPosYSpinner->setUnit(UnitsManager::instance().worldUnit());
	_camPosZSpinner->setUnit(UnitsManager::instance().worldUnit());

	fieldLayout = new QHBoxLayout();
	fieldLayout->setContentsMargins(0,0,0,0);
	fieldLayout->setSpacing(0);
	textBox = new QLineEdit();
	_camPosXSpinner->setTextBox(textBox);
	fieldLayout->addWidget(textBox);
	fieldLayout->addWidget(_camPosXSpinner);
	gridLayout->addLayout(fieldLayout, 0, 1);
	connect(_camPosXSpinner, SIGNAL(spinnerValueChanged()), this, SLOT(onAdjustCamera()));

	fieldLayout = new QHBoxLayout();
	fieldLayout->setContentsMargins(0,0,0,0);
	fieldLayout->setSpacing(0);
	textBox = new QLineEdit();
	_camPosYSpinner->setTextBox(textBox);
	fieldLayout->addWidget(textBox);
	fieldLayout->addWidget(_camPosYSpinner);
	gridLayout->addLayout(fieldLayout, 0, 2);
	connect(_camPosYSpinner, SIGNAL(spinnerValueChanged()), this, SLOT(onAdjustCamera()));

	fieldLayout = new QHBoxLayout();
	fieldLayout->setContentsMargins(0,0,0,0);
	fieldLayout->setSpacing(0);
	textBox = new QLineEdit();
	_camPosZSpinner->setTextBox(textBox);
	fieldLayout->addWidget(textBox);
	fieldLayout->addWidget(_camPosZSpinner);
	gridLayout->addLayout(fieldLayout, 0, 3);
	connect(_camPosZSpinner, SIGNAL(spinnerValueChanged()), this, SLOT(onAdjustCamera()));

	gridLayout->addWidget(new QLabel(tr("Camera direction:")), 1, 0);

	_camDirXSpinner = new SpinnerWidget();
	_camDirYSpinner = new SpinnerWidget();
	_camDirZSpinner = new SpinnerWidget();
	_camDirXSpinner->setUnit(UnitsManager::instance().worldUnit());
	_camDirYSpinner->setUnit(UnitsManager::instance().worldUnit());
	_camDirZSpinner->setUnit(UnitsManager::instance().worldUnit());

	fieldLayout = new QHBoxLayout();
	fieldLayout->setContentsMargins(0,0,0,0);
	fieldLayout->setSpacing(0);
	textBox = new QLineEdit();
	_camDirXSpinner->setTextBox(textBox);
	fieldLayout->addWidget(textBox);
	fieldLayout->addWidget(_camDirXSpinner);
	gridLayout->addLayout(fieldLayout, 1, 1);
	connect(_camDirXSpinner, SIGNAL(spinnerValueChanged()), this, SLOT(onAdjustCamera()));

	fieldLayout = new QHBoxLayout();
	fieldLayout->setContentsMargins(0,0,0,0);
	fieldLayout->setSpacing(0);
	textBox = new QLineEdit();
	_camDirYSpinner->setTextBox(textBox);
	fieldLayout->addWidget(textBox);
	fieldLayout->addWidget(_camDirYSpinner);
	gridLayout->addLayout(fieldLayout, 1, 2);
	connect(_camDirYSpinner, SIGNAL(spinnerValueChanged()), this, SLOT(onAdjustCamera()));

	fieldLayout = new QHBoxLayout();
	fieldLayout->setContentsMargins(0,0,0,0);
	fieldLayout->setSpacing(0);
	textBox = new QLineEdit();
	_camDirZSpinner->setTextBox(textBox);
	fieldLayout->addWidget(textBox);
	fieldLayout->addWidget(_camDirZSpinner);
	gridLayout->addLayout(fieldLayout, 1, 3);
	connect(_camDirZSpinner, SIGNAL(spinnerValueChanged()), this, SLOT(onAdjustCamera()));

	_camFOVLabel = new QLabel(tr("Field of view:"));
	gridLayout->addWidget(_camFOVLabel, 2, 0);
	_camFOVSpinner = new SpinnerWidget();
	_camFOVSpinner->setMinValue(1e-4f);

	fieldLayout = new QHBoxLayout();
	fieldLayout->setContentsMargins(0,0,0,0);
	fieldLayout->setSpacing(0);
	textBox = new QLineEdit();
	_camFOVSpinner->setTextBox(textBox);
	fieldLayout->addWidget(textBox);
	fieldLayout->addWidget(_camFOVSpinner);
	gridLayout->addLayout(fieldLayout, 2, 1);
	connect(_camFOVSpinner, SIGNAL(spinnerValueChanged()), this, SLOT(onAdjustCamera()));

	mainLayout->addLayout(gridLayout);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(onCancel()));
	mainLayout->addWidget(buttonBox);

	updateGUI();
}

/******************************************************************************
* Updates the values displayed in the dialog.
******************************************************************************/
void AdjustCameraDialog::updateGUI()
{
	_camPerspective->setChecked(_viewport->isPerspectiveProjection());
	_camPosXSpinner->setFloatValue(_viewport->cameraPosition().x());
	_camPosYSpinner->setFloatValue(_viewport->cameraPosition().y());
	_camPosZSpinner->setFloatValue(_viewport->cameraPosition().z());
	_camDirXSpinner->setFloatValue(_viewport->cameraDirection().x());
	_camDirYSpinner->setFloatValue(_viewport->cameraDirection().y());
	_camDirZSpinner->setFloatValue(_viewport->cameraDirection().z());
	if(_viewport->isPerspectiveProjection()) {
		_camFOVSpinner->setUnit(UnitsManager::instance().angleUnit());
		_camFOVLabel->setText(tr("View angle:"));
		_camFOVSpinner->setMaxValue(FLOATTYPE_PI - 1e-2);
	}
	else {
		_camFOVSpinner->setUnit(UnitsManager::instance().worldUnit());
		_camFOVLabel->setText(tr("Field of view:"));
		_camFOVSpinner->setMaxValue(FLOATTYPE_MAX);
	}
	_camFOVSpinner->setFloatValue(_viewport->fieldOfView());
}

/******************************************************************************
* Is called when the user has changed the camera settings.
******************************************************************************/
void AdjustCameraDialog::onAdjustCamera()
{
	if(_camPerspective->isChecked()) {
		if(!_viewport->isPerspectiveProjection())
			_camFOVSpinner->setFloatValue(FLOATTYPE_PI/4.0f);
		_viewport->setViewType(Viewport::VIEW_PERSPECTIVE);
	}
	else {
		if(_viewport->isPerspectiveProjection()) {
			_camFOVSpinner->setMaxValue(FLOATTYPE_MAX);
			_camFOVSpinner->setFloatValue(200.0f);
		}
		_viewport->setViewType(Viewport::VIEW_ORTHO);
	}

	_viewport->setCameraPosition(Point3(_camPosXSpinner->floatValue(), _camPosYSpinner->floatValue(), _camPosZSpinner->floatValue()));
	_viewport->setCameraDirection(Vector3(_camDirXSpinner->floatValue(), _camDirYSpinner->floatValue(), _camDirZSpinner->floatValue()));
	_viewport->setFieldOfView(_camFOVSpinner->floatValue());
}

/******************************************************************************
* Event handler for the Cancel button.
******************************************************************************/
void AdjustCameraDialog::onCancel()
{
	_viewport->setViewType(_oldViewType);
	_viewport->setCameraPosition(_oldCameraPos);
	_viewport->setCameraDirection(_oldCameraDir);
	_viewport->setFieldOfView(_oldFOV);

	reject();
}

};