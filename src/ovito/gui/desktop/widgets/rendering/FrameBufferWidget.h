////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/core/rendering/FrameBuffer.h>

namespace Ovito {

/**
 * This widget displays the contents of a FrameBuffer.
 */
class OVITO_GUI_EXPORT FrameBufferWidget : public QWidget
{
public:

	/// Constructor.
	FrameBufferWidget(QWidget* parent = nullptr) : QWidget(parent) {}

	/// Return the FrameBuffer that is currently shown in the widget (can be NULL).
	const std::shared_ptr<FrameBuffer>& frameBuffer() const { return _frameBuffer; }

	/// Sets the FrameBuffer that is currently shown in the widget.
	void setFrameBuffer(const std::shared_ptr<FrameBuffer>& frameBuffer);

	/// Returns the preferred size of the widget.
	virtual QSize sizeHint() const override;

	/// Returns the current zoom factor.
	qreal zoomFactor() const { return _zoomFactor; }

	/// Zooms in or out if the image.
	void setZoomFactor(qreal zoom);

protected:

	/// This is called by the system to paint the viewport area.
	virtual void paintEvent(QPaintEvent* event) override;

private Q_SLOTS:

	/// This handles contentChanged() signals from the frame buffer.
	void onFrameBufferContentChanged(QRect changedRegion) {
		// Repaint only portion of the widget.
		update(changedRegion);
	}

	/// This handles contentReset() signals from the frame buffer.
	void onFrameBufferContentReset();

private:

	/// The FrameBuffer that is shown in the widget.
	std::shared_ptr<FrameBuffer> _frameBuffer;

	/// The current zoom factor.
	qreal _zoomFactor = 1;

private:

	Q_OBJECT
};

}	// End of namespace


