///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2018) Alexander Stukowski
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
#include <core/rendering/RenderSettings.h>
#include <core/utilities/units/UnitsManager.h>
#include "CoordinateTripodOverlay.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(CoordinateTripodOverlay);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, alignment);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, tripodSize);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, lineWidth);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, font);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, fontSize);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, offsetX);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, offsetY);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis1Enabled);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis2Enabled);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis3Enabled);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis4Enabled);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis1Label);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis2Label);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis3Label);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis4Label);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis1Dir);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis2Dir);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis3Dir);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis4Dir);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis1Color);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis2Color);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis3Color);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, axis4Color);
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, alignment, "Position");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, tripodSize, "Size factor");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, lineWidth, "Line width");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, font, "Font");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, fontSize, "Label size");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, offsetX, "Offset X");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, offsetY, "Offset Y");
SET_PROPERTY_FIELD_UNITS(CoordinateTripodOverlay, offsetX, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS(CoordinateTripodOverlay, offsetY, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CoordinateTripodOverlay, tripodSize, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CoordinateTripodOverlay, lineWidth, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CoordinateTripodOverlay, fontSize, FloatParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
CoordinateTripodOverlay::CoordinateTripodOverlay(DataSet* dataset) : ViewportOverlay(dataset),
		_alignment(Qt::AlignLeft | Qt::AlignBottom),
		_tripodSize(0.075), _lineWidth(0.06), _offsetX(0), _offsetY(0),
		_fontSize(0.4),
		_axis1Enabled(true), _axis2Enabled(true), _axis3Enabled(true), _axis4Enabled(false),
		_axis1Label("x"), _axis2Label("y"), _axis3Label("z"), _axis4Label("w"),
		_axis1Dir(1,0,0), _axis2Dir(0,1,0), _axis3Dir(0,0,1), _axis4Dir(sqrt(0.5),sqrt(0.5),0),
		_axis1Color(1,0,0), _axis2Color(0,0.8,0), _axis3Color(0.2,0.2,1), _axis4Color(1,0,1)
{
}

/******************************************************************************
* This method paints the overlay contents onto the given canvas.
******************************************************************************/
void CoordinateTripodOverlay::renderImplementation(QPainter& painter, const ViewProjectionParameters& projParams, const RenderSettings* renderSettings)
{
	FloatType tripodSize = this->tripodSize() * renderSettings->outputImageHeight();
	if(tripodSize <= 0) return;

	FloatType lineWidth = this->lineWidth() * tripodSize;
	if(lineWidth <= 0) return;

	FloatType arrowSize = FloatType(0.17);

	QPointF origin(offsetX() * renderSettings->outputImageWidth(), -offsetY() * renderSettings->outputImageHeight());
	FloatType margin = tripodSize + lineWidth;

	if(alignment() & Qt::AlignLeft) origin.rx() += margin;
	else if(alignment() & Qt::AlignRight) origin.rx() += renderSettings->outputImageWidth() - margin;
	else if(alignment() & Qt::AlignHCenter) origin.rx() += FloatType(0.5) * renderSettings->outputImageWidth();

	if(alignment() & Qt::AlignTop) origin.ry() += margin;
	else if(alignment() & Qt::AlignBottom) origin.ry() += renderSettings->outputImageHeight() - margin;
	else if(alignment() & Qt::AlignVCenter) origin.ry() += FloatType(0.5) * renderSettings->outputImageHeight();

	// Project axes to screen.
	Vector3 axisDirs[4] = {
			projParams.viewMatrix * axis1Dir(),
			projParams.viewMatrix * axis2Dir(),
			projParams.viewMatrix * axis3Dir(),
			projParams.viewMatrix * axis4Dir()
	};

	// Get axis colors.
	QColor axisColors[4] = {
			axis1Color(),
			axis2Color(),
			axis3Color(),
			axis4Color()
	};

	// Order axes back to front.
	std::vector<int> orderedAxes;
	if(axis1Enabled()) orderedAxes.push_back(0);
	if(axis2Enabled()) orderedAxes.push_back(1);
	if(axis3Enabled()) orderedAxes.push_back(2);
	if(axis4Enabled()) orderedAxes.push_back(3);
	std::sort(orderedAxes.begin(), orderedAxes.end(), [&axisDirs](int a, int b) {
		return axisDirs[a].z() < axisDirs[b].z();
	});

	const QString labels[4] = {
			axis1Label(),
			axis2Label(),
			axis3Label(),
			axis4Label()
	};
	QFont font = this->font();
	qreal fontSize = tripodSize * std::max(0.0, (double)this->fontSize());
	if(fontSize != 0) {
		font.setPointSizeF(fontSize);
		painter.setFont(font);
	}

	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	for(int axis : orderedAxes) {
		QBrush brush(axisColors[axis]);
		QPen pen(axisColors[axis]);
		pen.setWidthF(lineWidth);
		pen.setJoinStyle(Qt::MiterJoin);
		pen.setCapStyle(Qt::FlatCap);
		painter.setPen(pen);
		painter.setBrush(brush);
		Vector3 dir = tripodSize * axisDirs[axis];
		Vector2 dir2(dir.x(), dir.y());
		if(dir2.squaredLength() > FLOATTYPE_EPSILON) {
			painter.drawLine(origin, origin + QPointF(dir2.x(), -dir2.y()));
			Vector2 ndir = dir2;
			if(ndir.length() > arrowSize * tripodSize)
				ndir.resize(arrowSize * tripodSize);
			QPointF head[3];
			head[1] = origin + QPointF(dir2.x(), -dir2.y());
			head[0] = head[1] + QPointF(0.5 *  ndir.y() - ndir.x(), -(0.5 * -ndir.x() - ndir.y()));
			head[2] = head[1] + QPointF(0.5 * -ndir.y() - ndir.x(), -(0.5 *  ndir.x() - ndir.y()));
			painter.drawConvexPolygon(head, 3);
		}

		if(fontSize != 0) {
			QRectF textRect = painter.boundingRect(QRectF(0,0,0,0), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextDontClip, labels[axis]);
			textRect.translate(origin + QPointF(dir.x(), -dir.y()));
			if(std::abs(dir.x()) > FLOATTYPE_EPSILON || std::abs(dir.y()) > FLOATTYPE_EPSILON) {
				FloatType offset1 = dir.x() != 0 ? textRect.width() / std::abs(dir.x()) : FLOATTYPE_MAX;
				FloatType offset2 = dir.y() != 0 ? textRect.height() / std::abs(dir.y()) : FLOATTYPE_MAX;
				textRect.translate(0.5 * std::min(offset1, offset2) * QPointF(dir.x(), -dir.y()));
				Vector3 ndir(dir.x(), dir.y(), 0);
				ndir.resize(lineWidth);
				textRect.translate(ndir.x(), -ndir.y());
			}
			painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextDontClip, labels[axis]);
		}
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
