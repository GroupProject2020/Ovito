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

#include <ovito/gui/desktop/GUI.h>
#include "ViewportSettingsPage.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(ViewportSettingsPage);

/******************************************************************************
* Creates the widget that contains the plugin specific setting controls.
******************************************************************************/
void ViewportSettingsPage::insertSettingsDialogPage(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget)
{
	// Retrieve current settings.
	_settings.assign(ViewportSettings::getSettings());

	QWidget* page = new QWidget();
	tabWidget->addTab(page, tr("Viewports"));
	QGridLayout* layout1 = new QGridLayout(page);

	QGroupBox* upDirectionGroupBox = new QGroupBox(tr("Camera"), page);
	layout1->addWidget(upDirectionGroupBox, 0, 0);
	QGridLayout* layout2 = new QGridLayout(upDirectionGroupBox);

	QLabel* label1 = new QLabel(tr("<html><p>Coordinate system orientation:</p></html>"));
	label1->setWordWrap(true);
	layout2->addWidget(label1, 0, 0, 1, 4);

	_upDirectionGroup = new QButtonGroup(page);
	QRadioButton* verticalAxisX = new QRadioButton(QString(), upDirectionGroupBox);
	QRadioButton* verticalAxisY = new QRadioButton(QString(), upDirectionGroupBox);
	QRadioButton* verticalAxisZ = new QRadioButton(tr("(default)"), upDirectionGroupBox);
	_upDirectionGroup->addButton(verticalAxisX, ViewportSettings::X_AXIS);
	_upDirectionGroup->addButton(verticalAxisY, ViewportSettings::Y_AXIS);
	_upDirectionGroup->addButton(verticalAxisZ, ViewportSettings::Z_AXIS);
	verticalAxisX->setIcon(QIcon(":/gui/mainwin/settings/vertical_axis_x.png"));
	verticalAxisX->setIconSize(verticalAxisX->icon().availableSizes().front());
	verticalAxisX->setToolTip(tr("X-axis"));
	verticalAxisY->setIcon(QIcon(":/gui/mainwin/settings/vertical_axis_y.png"));
	verticalAxisY->setIconSize(verticalAxisY->icon().availableSizes().front());
	verticalAxisY->setToolTip(tr("Y-axis"));
	verticalAxisZ->setIcon(QIcon(":/gui/mainwin/settings/vertical_axis_z.png"));
	verticalAxisZ->setIconSize(verticalAxisZ->icon().availableSizes().front());
	verticalAxisZ->setToolTip(tr("Z-axis"));
	layout2->addWidget(verticalAxisX, 1, 0, 1, 1);
	layout2->addWidget(verticalAxisY, 1, 1, 1, 1);
	layout2->addWidget(verticalAxisZ, 1, 2, 1, 1);
	_upDirectionGroup->button(_settings.upDirection())->setChecked(true);
	layout2->setColumnStretch(3, 1);

	_constrainCameraRotationBox = new QCheckBox(tr("Restrict camera rotation to keep major axis pointing upward"));
	_constrainCameraRotationBox->setChecked(_settings.constrainCameraRotation());
	layout2->addWidget(_constrainCameraRotationBox, 2, 0, 1, 3);

	QGroupBox* colorsGroupBox = new QGroupBox(tr("Color scheme"), page);
	layout1->addWidget(colorsGroupBox, 1, 0);
	layout2 = new QGridLayout(colorsGroupBox);

	_colorScheme = new QButtonGroup(page);
	QRadioButton* darkColorScheme = new QRadioButton(tr("Dark"), colorsGroupBox);
	QRadioButton* lightColorScheme = new QRadioButton(tr("Light"), colorsGroupBox);
	layout2->addWidget(darkColorScheme, 0, 0, 1, 1);
	layout2->addWidget(lightColorScheme, 0, 1, 1, 1);
	_colorScheme->addButton(darkColorScheme, 0);
	_colorScheme->addButton(lightColorScheme, 1);
	if(_settings.viewportColor(ViewportSettings::COLOR_VIEWPORT_BKG) == Color(0,0,0))
		darkColorScheme->setChecked(true);
	else
		lightColorScheme->setChecked(true);

	layout1->setRowStretch(2, 1);
}

/******************************************************************************
* Lets the page save all changed settings.
******************************************************************************/
bool ViewportSettingsPage::saveValues(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget)
{
	// Update settings.
	_settings.setUpDirection((ViewportSettings::UpDirection)_upDirectionGroup->checkedId());
	_settings.setConstrainCameraRotation(_constrainCameraRotationBox->isChecked());
	if(_colorScheme->checkedId() == 1) {
		// Light color scheme.
		_settings.setViewportColor(ViewportSettings::COLOR_VIEWPORT_BKG, Color(1.0f, 1.0f, 1.0f));
		_settings.setViewportColor(ViewportSettings::COLOR_GRID, Color(0.6f, 0.6f, 0.6f));
		_settings.setViewportColor(ViewportSettings::COLOR_GRID_INTENS, Color(0.5f, 0.5f, 0.5f));
		_settings.setViewportColor(ViewportSettings::COLOR_GRID_AXIS, Color(0.4f, 0.4f, 0.4f));
		_settings.setViewportColor(ViewportSettings::COLOR_VIEWPORT_CAPTION, Color(0.0f, 0.0f, 0.0f));
		_settings.setViewportColor(ViewportSettings::COLOR_SELECTION, Color(0.0f, 0.0f, 0.0f));
		_settings.setViewportColor(ViewportSettings::COLOR_UNSELECTED, Color(0.5f, 0.5f, 1.0f));
		_settings.setViewportColor(ViewportSettings::COLOR_ACTIVE_VIEWPORT_BORDER, Color(1.0f, 1.0f, 0.0f));
		_settings.setViewportColor(ViewportSettings::COLOR_ANIMATION_MODE, Color(1.0f, 0.0f, 0.0f));
		_settings.setViewportColor(ViewportSettings::COLOR_CAMERAS, Color(0.5f, 0.5f, 1.0f));
	}
	else {
		// Dark color scheme.
		_settings.restoreDefaultViewportColors();
	}

	// Store current settings.
	ViewportSettings::setSettings(_settings);

	return true;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
