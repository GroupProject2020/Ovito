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

#include <ovito/gui/desktop/GUI.h>
#include "ColorPickerWidget.h"

namespace Ovito {

/******************************************************************************
* Constructs the control.
******************************************************************************/
ColorPickerWidget::ColorPickerWidget(QWidget* parent)
	: QAbstractButton(parent), _color(1,1,1)
{
	connect(this, &ColorPickerWidget::clicked, this, &ColorPickerWidget::activateColorPicker);
}

/******************************************************************************
* Sets the current value of the color picker.
******************************************************************************/
void ColorPickerWidget::setColor(const Color& newVal, bool emitChangeSignal)
{
	if(newVal == _color) return;

	// Update control.
	_color = newVal;
	update();

	// Send change message
	if(emitChangeSignal)
		colorChanged();
}

/******************************************************************************
* Paints the widget.
******************************************************************************/
void ColorPickerWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	QBrush brush{(QColor)color()};
	if(isEnabled()) {
		qDrawShadePanel(&painter, rect(), palette(), isDown(), 1, &brush);
	}
	else {
		painter.fillRect(rect(), brush);
	}
}

/******************************************************************************
* Returns the preferred size of the widget.
******************************************************************************/
QSize ColorPickerWidget::sizeHint() const
{
    int w = 16;
	int h = fontMetrics().xHeight();

#if !defined(Q_OS_MAC)
	QStyleOptionButton opt;
	opt.initFrom(this);
	opt.features = QStyleOptionButton::Flat;
	return style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(w, h), this).expandedTo(QApplication::globalStrut()).expandedTo(QSize(0,22));
#else
	QStyleOptionFrame opt;
	opt.initFrom(this);
	opt.features = QStyleOptionFrame::Flat;
	return style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h), this).expandedTo(QApplication::globalStrut()).expandedTo(QSize(0,22));
#endif
}

/******************************************************************************
* Is called when the user has clicked on the color picker control.
******************************************************************************/
void ColorPickerWidget::activateColorPicker()
{
	QColor newColor = QColorDialog::getColor((QColor)_color, window());
	if(newColor.isValid()) {
		setColor(Color(newColor), true);
	}
}

}	// End of namespace
