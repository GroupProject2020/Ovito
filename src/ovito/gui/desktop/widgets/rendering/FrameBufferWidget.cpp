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

#include <ovito/gui/desktop/GUI.h>
#include "FrameBufferWidget.h"

namespace Ovito {

/******************************************************************************
* Sets the FrameBuffer that is currently shown in the widget.
******************************************************************************/
void FrameBufferWidget::setFrameBuffer(const std::shared_ptr<FrameBuffer>& newFrameBuffer)
{
	if(newFrameBuffer == frameBuffer()) {
		onFrameBufferContentReset();
		return;
	}

	if(frameBuffer()) {
		disconnect(_frameBuffer.get(), &FrameBuffer::contentChanged, this, &FrameBufferWidget::onFrameBufferContentChanged);
		disconnect(_frameBuffer.get(), &FrameBuffer::contentReset, this, &FrameBufferWidget::onFrameBufferContentReset);
	}

	_frameBuffer = newFrameBuffer;

	onFrameBufferContentReset();

	connect(_frameBuffer.get(), &FrameBuffer::contentChanged, this, &FrameBufferWidget::onFrameBufferContentChanged);
	connect(_frameBuffer.get(), &FrameBuffer::contentReset, this, &FrameBufferWidget::onFrameBufferContentReset);
}

/******************************************************************************
* Computes the preferred size of the widget.
******************************************************************************/
QSize FrameBufferWidget::sizeHint() const
{
	if(_frameBuffer) {
		return _frameBuffer->size() * _zoomFactor;
	}
	return QWidget::sizeHint();
}

/******************************************************************************
* This is called by the system to paint the widgets area.
******************************************************************************/
void FrameBufferWidget::paintEvent(QPaintEvent* event)
{
	if(frameBuffer()) {
		QPainter painter(this);
		QSize imgSize = frameBuffer()->image().size();
		painter.drawImage(QRect(QPoint(0, 0), imgSize * _zoomFactor), frameBuffer()->image());
	}
}

/******************************************************************************
* Zooms in or out if the image.
******************************************************************************/
void FrameBufferWidget::setZoomFactor(qreal zoom)
{
	_zoomFactor = qBound(1e-1, zoom, 1e1);
	update();
}

/******************************************************************************
* This handles contentReset() signals from the frame buffer.
******************************************************************************/
void FrameBufferWidget::onFrameBufferContentReset()
{
	// Reset zoom factor.
	_zoomFactor = 1;

	// Resize widget.
	if(_frameBuffer) {
		resize(_frameBuffer->size());
		for(QWidget* w = parentWidget(); w != nullptr; w = w->parentWidget()) {
			// Size hint of scroll area has changed. Update its geometry.
			if(qobject_cast<QScrollArea*>(w)) {
				w->updateGeometry();
				break;
			}
		}
	}

	// Repaint the widget.
	update();
}

}	// End of namespace
