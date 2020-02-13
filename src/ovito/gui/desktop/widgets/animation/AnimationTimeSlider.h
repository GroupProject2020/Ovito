////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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


#include <ovito/gui/desktop/GUI.h>

namespace Ovito {

/**
 * This slider component controls the current scene time.
 */
class AnimationTimeSlider : public QFrame
{
	Q_OBJECT

public:

	/// Constructor.
	AnimationTimeSlider(MainWindow* mainWindow, QWidget* parentWindow = nullptr);

	/// Computes the x position within the widget corresponding to the given animation time.
	int timeToPos(TimePoint time);

	/// Converts a distance in pixels to a time difference.
	TimePoint distanceToTimeDifference(int distance);

	/// Computes the current position of the slider thumb.
	QRect thumbRectangle();

	/// Computes the width of the thumb.
	int thumbWidth() const;

	/// Computes the time ticks to draw.
	std::tuple<TimePoint,TimePoint,TimePoint> tickRange(int tickWidth);

	/// Computes the maximum width of a frame tick label.
	int maxTickLabelWidth();

	/// Returns the recommended size of the widget.
	virtual QSize sizeHint() const override;

	/// Returns the minimum size of the widget.
	virtual QSize minimumSizeHint() const override { return sizeHint(); }

protected:

	/// Handles paint events.
	virtual void paintEvent(QPaintEvent* event) override;

	/// Handles mouse down events.
	virtual void mousePressEvent(QMouseEvent* event) override;

	/// Handles mouse up events.
	virtual void mouseReleaseEvent(QMouseEvent* event) override;

	/// Handles mouse move events.
	virtual void mouseMoveEvent(QMouseEvent* event) override;

	/// Is called when the widgets looses the input focus.
	virtual void focusOutEvent(QFocusEvent* event) override;

protected Q_SLOTS:

	/// This is called when new animation settings have been loaded.
	void onAnimationSettingsReplaced(AnimationSettings* newAnimationSettings);

	/// Is called whenever the Auto Key mode is activated or deactivated.
	void onAutoKeyModeChanged(bool active);

private:

	/// The dragging start position.
	int _dragPos = -1;

	/// The default palette used to the draw the time slide background.
	QPalette _normalPalette;

	/// The color palette used to the draw the time slide background when Auto Key mode is active.
	QPalette _autoKeyModePalette;

	/// The palette used to the draw the slider.
	QPalette _sliderPalette;

	/// The main window.
	MainWindow* _mainWindow;

	/// The current animation settings object.
	AnimationSettings* _animSettings = nullptr;

	QMetaObject::Connection _autoKeyModeChangedConnection;
	QMetaObject::Connection _animIntervalChangedConnection;
	QMetaObject::Connection _timeFormatChangedConnection;
	QMetaObject::Connection _timeChangedConnection;
};

}	// End of namespace


