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
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include "AnimationSettingsDialog.h"

namespace Ovito {

/******************************************************************************
* The constructor of the animation settings dialog.
******************************************************************************/
AnimationSettingsDialog::AnimationSettingsDialog(AnimationSettings* animSettings, QWidget* parent) :
		QDialog(parent), _animSettings(animSettings), UndoableTransaction(animSettings->dataset()->undoStack(), tr("Change animation settings"))
{
	setWindowTitle(tr("Animation Settings"));

	QVBoxLayout* layout1 = new QVBoxLayout(this);

	QGroupBox* playbackRateBox = new QGroupBox(tr("Playback"));
	layout1->addWidget(playbackRateBox);

	QGridLayout* contentLayout = new QGridLayout(playbackRateBox);
	contentLayout->setHorizontalSpacing(0);
	contentLayout->setVerticalSpacing(2);
	contentLayout->setColumnStretch(1, 1);

	contentLayout->addWidget(new QLabel(tr("Frames per second:"), this), 0, 0);
	fpsBox = new QComboBox(this);
	QLocale locale;
	fpsBox->addItem(locale.toString(0.1), TICKS_PER_SECOND * 10);
	fpsBox->addItem(locale.toString(0.2), TICKS_PER_SECOND * 5);
	fpsBox->addItem(locale.toString(0.5), TICKS_PER_SECOND * 2);
	fpsBox->addItem(locale.toString(1), TICKS_PER_SECOND * 1);
	fpsBox->addItem(locale.toString(2), TICKS_PER_SECOND / 2);
	fpsBox->addItem(locale.toString(4), TICKS_PER_SECOND / 4);
	fpsBox->addItem(locale.toString(5), TICKS_PER_SECOND / 5);
	fpsBox->addItem(locale.toString(8), TICKS_PER_SECOND / 8);
	fpsBox->addItem(locale.toString(10), TICKS_PER_SECOND / 10);
	fpsBox->addItem(locale.toString(12), TICKS_PER_SECOND / 12);
	fpsBox->addItem(locale.toString(15), TICKS_PER_SECOND / 15);
	fpsBox->addItem(locale.toString(16), TICKS_PER_SECOND / 16);
	fpsBox->addItem(locale.toString(20), TICKS_PER_SECOND / 20);
	fpsBox->addItem(locale.toString(24), TICKS_PER_SECOND / 24);
	fpsBox->addItem(locale.toString(25), TICKS_PER_SECOND / 25);
	fpsBox->addItem(locale.toString(30), TICKS_PER_SECOND / 30);
	fpsBox->addItem(locale.toString(32), TICKS_PER_SECOND / 32);
	fpsBox->addItem(locale.toString(40), TICKS_PER_SECOND / 40);
	fpsBox->addItem(locale.toString(50), TICKS_PER_SECOND / 50);
	fpsBox->addItem(locale.toString(60), TICKS_PER_SECOND / 60);
	contentLayout->addWidget(fpsBox, 0, 1, 1, 2);
	connect(fpsBox, (void (QComboBox::*)(int))&QComboBox::activated, this, &AnimationSettingsDialog::onFramesPerSecondChanged);

	contentLayout->addWidget(new QLabel(tr("Playback speed in viewports:"), this), 1, 0);
	playbackSpeedBox = new QComboBox(this);
	playbackSpeedBox->addItem(tr("x 1/40"), -40);
	playbackSpeedBox->addItem(tr("x 1/20"), -20);
	playbackSpeedBox->addItem(tr("x 1/10"), -10);
	playbackSpeedBox->addItem(tr("x 1/5"), -5);
	playbackSpeedBox->addItem(tr("x 1/2"), -2);
	playbackSpeedBox->addItem(tr("x 1 (Realtime)"), 1);
	playbackSpeedBox->addItem(tr("x 2"), 2);
	playbackSpeedBox->addItem(tr("x 5"), 5);
	playbackSpeedBox->addItem(tr("x 10"), 10);
	playbackSpeedBox->addItem(tr("x 20"), 20);
	contentLayout->addWidget(playbackSpeedBox, 1, 1, 1, 2);
	connect(playbackSpeedBox, (void (QComboBox::*)(int))&QComboBox::activated, this, &AnimationSettingsDialog::onPlaybackSpeedChanged);

	loopPlaybackBox = new QCheckBox(tr("Loop playback"));
	contentLayout->addWidget(loopPlaybackBox, 2, 0, 1, 3);
	connect(loopPlaybackBox, &QCheckBox::clicked, this, [this](bool checked) {
		_animSettings->setLoopPlayback(checked);
	});

	animIntervalBox = new QGroupBox(tr("Custom animation interval"));
	animIntervalBox->setCheckable(true);
	layout1->addWidget(animIntervalBox);

	contentLayout = new QGridLayout(animIntervalBox);
	contentLayout->setHorizontalSpacing(0);
	contentLayout->setVerticalSpacing(2);
	contentLayout->setColumnStretch(1, 1);

	contentLayout->addWidget(new QLabel(tr("Start frame:"), this), 0, 0);
	QLineEdit* animStartBox = new QLineEdit(this);
	contentLayout->addWidget(animStartBox, 0, 1);
	animStartSpinner = new SpinnerWidget(this);
	animStartSpinner->setTextBox(animStartBox);
	animStartSpinner->setUnit(animSettings->dataset()->unitsManager().timeUnit());
	contentLayout->addWidget(animStartSpinner, 0, 2);
	connect(animStartSpinner, &SpinnerWidget::spinnerValueChanged, this, &AnimationSettingsDialog::onAnimationIntervalChanged);

	contentLayout->addWidget(new QLabel(tr("End frame:"), this), 1, 0);
	QLineEdit* animEndBox = new QLineEdit(this);
	contentLayout->addWidget(animEndBox, 1, 1);
	animEndSpinner = new SpinnerWidget(this);
	animEndSpinner->setTextBox(animEndBox);
	animEndSpinner->setUnit(animSettings->dataset()->unitsManager().timeUnit());
	contentLayout->addWidget(animEndSpinner, 1, 2);
	connect(animEndSpinner, &SpinnerWidget::spinnerValueChanged, this, &AnimationSettingsDialog::onAnimationIntervalChanged);

	connect(animIntervalBox, &QGroupBox::clicked, this, [this](bool checked) {
		_animSettings->setAutoAdjustInterval(!checked);
		updateUI();
	});

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, Qt::Horizontal, this);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &AnimationSettingsDialog::onOk);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &AnimationSettingsDialog::reject);

	// Implement Help button.
	connect(buttonBox, &QDialogButtonBox::helpRequested, []() {
		MainWindow::openHelpTopic(QStringLiteral("animation.animation_settings_dialog.html"));
	});

	layout1->addWidget(buttonBox);
    updateUI();
}

/******************************************************************************
* Event handler for the Ok button.
******************************************************************************/
void AnimationSettingsDialog::onOk()
{
	commit();
	accept();
}

/******************************************************************************
* Updates the values shown in the dialog.
******************************************************************************/
void AnimationSettingsDialog::updateUI()
{
	fpsBox->setCurrentIndex(fpsBox->findData(_animSettings->ticksPerFrame()));
	playbackSpeedBox->setCurrentIndex(playbackSpeedBox->findData(_animSettings->playbackSpeed()));
	animStartSpinner->setIntValue(_animSettings->animationInterval().start());
	animEndSpinner->setIntValue(_animSettings->animationInterval().end());
	loopPlaybackBox->setChecked(_animSettings->loopPlayback());
	animIntervalBox->setChecked(!_animSettings->autoAdjustInterval());
	animStartSpinner->setEnabled(!_animSettings->autoAdjustInterval());
	animEndSpinner->setEnabled(!_animSettings->autoAdjustInterval());
}

/******************************************************************************
* Is called when the user has selected a new value for the frames per seconds.
******************************************************************************/
void AnimationSettingsDialog::onFramesPerSecondChanged(int index)
{
	qint64 newTicksPerFrame = fpsBox->itemData(index).toInt();
	OVITO_ASSERT(newTicksPerFrame != 0);

	int currentFrame = _animSettings->currentFrame();

	// Change the animation speed.
	qint64 oldTicksPerFrame = _animSettings->ticksPerFrame();
	_animSettings->setTicksPerFrame((int)newTicksPerFrame);

	// Rescale animation interval and animation keys.
	TimeInterval oldInterval = _animSettings->animationInterval();
	TimeInterval newInterval;
	newInterval.setStart((TimePoint)((qint64)oldInterval.start() * newTicksPerFrame / oldTicksPerFrame));
	newInterval.setEnd((TimePoint)((qint64)oldInterval.end() * newTicksPerFrame / oldTicksPerFrame));
	_animSettings->setAnimationInterval(newInterval);

	_animSettings->dataset()->rescaleTime(oldInterval, newInterval);

	// Update animation time to remain at the current frame.
	_animSettings->setCurrentFrame(currentFrame);

	// Update dialog controls to reflect new values.
	updateUI();
}

/******************************************************************************
* Is called when the user has selected a new value for the playback speed.
******************************************************************************/
void AnimationSettingsDialog::onPlaybackSpeedChanged(int index)
{
	int newPlaybackSpeed = playbackSpeedBox->itemData(index).toInt();
	OVITO_ASSERT(newPlaybackSpeed != 0);

	// Change the animation speed.
	_animSettings->setPlaybackSpeed(newPlaybackSpeed);

	// Update dialog controls to reflect new values.
	updateUI();
}

/******************************************************************************
* Is called when the user changes the start/end values of the animation interval.
******************************************************************************/
void AnimationSettingsDialog::onAnimationIntervalChanged()
{
	TimeInterval interval(animStartSpinner->intValue(), animEndSpinner->intValue());
	if(interval.end() < interval.start())
		interval.setEnd(interval.start());

	_animSettings->setAnimationInterval(interval);
	if(_animSettings->time() < interval.start())
		_animSettings->setTime(interval.start());
	else if(_animSettings->time() > interval.end())
		_animSettings->setTime(interval.end());

	// Update dialog controls to reflect new values.
	updateUI();
}

}	// End of namespace
