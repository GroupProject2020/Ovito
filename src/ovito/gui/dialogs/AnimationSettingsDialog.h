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

#pragma once


#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/gui/widgets/general/SpinnerWidget.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * This dialog box lets the user manage the animation settings.
 */
class OVITO_GUI_EXPORT AnimationSettingsDialog : public QDialog, private UndoableTransaction
{
	Q_OBJECT

public:

	/// Constructor.
	AnimationSettingsDialog(AnimationSettings* animSettings, QWidget* parentWindow = nullptr);

private Q_SLOTS:

	/// Event handler for the Ok button.
	void onOk();

	/// Is called when the user has selected a new value for the frames per seconds.
	void onFramesPerSecondChanged(int index);

	/// Is called when the user has selected a new value for the playback speed.
	void onPlaybackSpeedChanged(int index);

	/// Is called when the user changes the start/end values of the animation interval.
	void onAnimationIntervalChanged();

private:

	/// Updates the values shown in the dialog.
	void updateUI();

	/// The animation settings being edited.
	OORef<AnimationSettings> _animSettings;

	QComboBox* fpsBox;
	SpinnerWidget* animStartSpinner;
	SpinnerWidget* animEndSpinner;
	QComboBox* playbackSpeedBox;
	QCheckBox* loopPlaybackBox;
	QGroupBox* animIntervalBox;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


