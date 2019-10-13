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

#include <ovito/core/Core.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/utilities/units/UnitsManager.h>
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
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, tripodStyle);
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, alignment, "Position");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, tripodSize, "Size factor");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, lineWidth, "Line width");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, font, "Font");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, fontSize, "Label size");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, offsetX, "Offset X");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, offsetY, "Offset Y");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, tripodStyle, "Style");
SET_PROPERTY_FIELD_UNITS(CoordinateTripodOverlay, offsetX, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS(CoordinateTripodOverlay, offsetY, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CoordinateTripodOverlay, tripodSize, FloatParameterUnit, 1e-4);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CoordinateTripodOverlay, lineWidth, FloatParameterUnit, 1e-4);
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
		_axis1Color(1,0,0), _axis2Color(0,0.8,0), _axis3Color(0.2,0.2,1), _axis4Color(1,0,1),
		_tripodStyle(FlatArrows)
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

	// Render axis arrows.
	FloatType lastZ = -1;
	for(int axis : orderedAxes) {

		if(tripodStyle() == SolidArrows && lastZ < 0 && axisDirs[axis].z() >= 0) {
			paintSolidJoint(painter, origin, projParams.viewMatrix, lineWidth);
		}
		lastZ = axisDirs[axis].z();

		QBrush brush(axisColors[axis]);
		QPen pen(axisColors[axis]);
		pen.setWidthF(lineWidth);
		pen.setJoinStyle(Qt::MiterJoin);
		pen.setCapStyle(Qt::RoundCap);
		painter.setPen(pen);
		painter.setBrush(brush);
		Vector3 dir3d = tripodSize * axisDirs[axis];
		dir3d.y() = -dir3d.y();
		Vector2 dir2d(dir3d.x(), dir3d.y());
		FloatType labelMargin = lineWidth;

		// Render axis arrow.
		if(tripodStyle() == FlatArrows) {
			labelMargin += paintFlatArrow(painter, dir2d, arrowSize, lineWidth, tripodSize, origin);
		}
		else if(tripodStyle() == SolidArrows) {
			labelMargin += paintSolidArrow(painter, dir2d, dir3d, arrowSize, lineWidth, tripodSize, origin);
		}

		// Render axis label.
		if(fontSize != 0 && !labels[axis].isEmpty()) {
			QRectF textRect = painter.boundingRect(QRectF(0,0,0,0), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextDontClip, labels[axis]);
			textRect.translate(origin + QPointF(dir2d.x(), dir2d.y()));
			if(dir2d.isZero() && orderedAxes.size() >= 2) {
				// When looking on the axis head-on, determine the displacement of the label such that it moves away
				// from the other axes.
				Vector3 averageAxisDir = Vector3::Zero();
				for(int otherAxis : orderedAxes)
					if(otherAxis != axis) averageAxisDir += axisDirs[otherAxis];
				if(!averageAxisDir.isZero())
					dir2d = Vector2(-averageAxisDir.x(), averageAxisDir.y());
			}
			// Position the label at the end of the axis arrow and a bit beyond.
			if(!dir2d.isZero()) {
				FloatType offset1 = dir2d.x() != 0 ? textRect.width() / std::abs(dir2d.x()) : FLOATTYPE_MAX;
				FloatType offset2 = dir2d.y() != 0 ? textRect.height() / std::abs(dir2d.y()) : FLOATTYPE_MAX;
				textRect.translate(0.5 * std::min(offset1, offset2) * QPointF(dir2d.x(), dir2d.y()));
				Vector2 dir2d_normalized = dir2d;
				dir2d_normalized.resize(labelMargin);
				textRect.translate(dir2d_normalized.x(), dir2d_normalized.y());
			}
			painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextDontClip, labels[axis]);
		}
	}

	if(tripodStyle() == SolidArrows && lastZ < 0) {
		paintSolidJoint(painter, origin, projParams.viewMatrix, lineWidth);
	}
}

/******************************************************************************
* Paints a single arrow in flat style.
******************************************************************************/
FloatType CoordinateTripodOverlay::paintFlatArrow(QPainter& painter, const Vector2& dir2d, FloatType arrowSize, FloatType lineWidth, FloatType tripodSize, QPointF origin)
{
	if(!dir2d.isZero()) {
		painter.drawLine(origin, origin + QPointF(dir2d.x(), dir2d.y()));
		Vector2 dir2d_normalized = dir2d;
		if(dir2d_normalized.length() > arrowSize * tripodSize)
			dir2d_normalized.resize(arrowSize * tripodSize);
		QPointF head[3];
		head[1] = origin + QPointF(dir2d.x(), dir2d.y());
		head[0] = head[1] + QPointF(0.5 * -dir2d_normalized.y() - dir2d_normalized.x(), -(0.5 * -dir2d_normalized.x() + dir2d_normalized.y()));
		head[2] = head[1] + QPointF(0.5 *  dir2d_normalized.y() - dir2d_normalized.x(), -(0.5 *  dir2d_normalized.x() + dir2d_normalized.y()));
		painter.drawConvexPolygon(head, 3);
		return 0;
	}
	else {
		// Draw a circle instead of an arrow when looking head onto the axis.
		double arrowHeadSize = (lineWidth + tripodSize * arrowSize) * 0.5;
		QPen pen = painter.pen();
		painter.setPen(Qt::NoPen);
		painter.drawEllipse(origin, arrowHeadSize, arrowHeadSize);
		painter.setPen(pen);
		return arrowHeadSize * 0.5;
	}
}

/******************************************************************************
* Paints a single arrow in solid style.
******************************************************************************/
FloatType CoordinateTripodOverlay::paintSolidArrow(QPainter& painter, const Vector2& dir2d, const Vector3& dir3d, FloatType arrowSize, FloatType lineWidth, FloatType tripodSize, QPointF origin)
{
	if(!dir2d.isZero()) {
		QPainterPath cylPth;
		QPainterPath capPth;
		FloatType len = dir2d.length();
		FloatType offset = len / tripodSize * lineWidth;
		cylPth.moveTo(offset, lineWidth);
		cylPth.lineTo(len, lineWidth);
		if(std::abs(dir3d.z()) > FLOATTYPE_EPSILON) {
			qreal d = -dir3d.z() / tripodSize * lineWidth;
			cylPth.arcTo(QRectF(len - d, -lineWidth, d*2, lineWidth*2), 270.0, 180.0);
			if(dir3d.z() > 0) {
				capPth.addEllipse(QRectF(len - d, -lineWidth, d*2, lineWidth*2));
			}
		}
		else {
			cylPth.lineTo(len, -lineWidth);
		}
		cylPth.lineTo(offset, -lineWidth);
		if(std::abs(dir3d.z()) > FLOATTYPE_EPSILON) {
			qreal d = -dir3d.z() / tripodSize * lineWidth;
			cylPth.arcTo(QRectF(offset - d, -lineWidth, d*2, lineWidth*2), 90.0, -180.0);
		}
		else {
			cylPth.closeSubpath();
		}
		QTransform parentTransform = painter.transform();
		QTransform transform;
		transform.translate(origin.x(), origin.y());
		transform.rotateRadians(std::atan2(dir2d.y(), dir2d.x()));
		painter.setWorldTransform(transform, true);
		QPen pen = painter.pen();
		painter.setPen(QPen(Qt::black, 0.3));
		painter.drawPath(capPth);
		QBrush brush = painter.brush();
		QLinearGradient gradient(0, -lineWidth, 0, lineWidth);
		gradient.setColorAt(0.0, brush.color().darker());
		gradient.setColorAt(0.2, brush.color());
		gradient.setColorAt(0.4, (brush.color().lightness() != 0) ? brush.color().lighter() : QColor(200, 200, 200));
		gradient.setColorAt(0.7, brush.color());
		gradient.setColorAt(1.0, brush.color().darker());
		painter.setBrush(gradient);
		painter.drawPath(cylPth);
		painter.setPen(pen);
		painter.setBrush(brush);
		painter.setWorldTransform(parentTransform);
	}
	else {
		double arrowHeadSize = (lineWidth + tripodSize * arrowSize) * 0.5;
		return arrowHeadSize * 0.5;
	}
	return 0;
}

/******************************************************************************
* Paints the tripod's joint in solid style.
******************************************************************************/
void CoordinateTripodOverlay::paintSolidJoint(QPainter& painter, QPointF origin, const AffineTransformation& viewTM, FloatType lineWidth)
{
	const FloatType scaling = lineWidth;
	const Vector3 dirs[3] = {
		viewTM.column(0),
		viewTM.column(1),
		viewTM.column(2)
	};

	QPen pen = painter.pen();
	QBrush brush = painter.brush();
	painter.setPen(QPen(Qt::black, 0.2));

	QPointF vertices[4];
	for(int side = 0; side < 3; side++) {
		qreal lightness = (std::abs(dirs[side].z()) + 0.5) / 1.6;
		painter.setBrush(QColor::fromHslF(0.0, 0.0, lightness));
		FloatType flip = (dirs[side].z() < 0) ? -1 : 1;
		vertices[0] = origin;
		vertices[0].rx() += (flip * dirs[side].x() + dirs[(side+1)%3].x() + dirs[(side+2)%3].x()) * scaling;
		vertices[0].ry() -= (flip * dirs[side].y() + dirs[(side+1)%3].y() + dirs[(side+2)%3].y()) * scaling;
		vertices[1] = origin;
		vertices[1].rx() += (flip * dirs[side].x() - dirs[(side+1)%3].x() + dirs[(side+2)%3].x()) * scaling;
		vertices[1].ry() -= (flip * dirs[side].y() - dirs[(side+1)%3].y() + dirs[(side+2)%3].y()) * scaling;
		vertices[2] = origin;
		vertices[2].rx() += (flip * dirs[side].x() - dirs[(side+1)%3].x() - dirs[(side+2)%3].x()) * scaling;
		vertices[2].ry() -= (flip * dirs[side].y() - dirs[(side+1)%3].y() - dirs[(side+2)%3].y()) * scaling;
		vertices[3] = origin;
		vertices[3].rx() += (flip * dirs[side].x() + dirs[(side+1)%3].x() - dirs[(side+2)%3].x()) * scaling;
		vertices[3].ry() -= (flip * dirs[side].y() + dirs[(side+1)%3].y() - dirs[(side+2)%3].y()) * scaling;
		painter.drawPolygon(vertices, 4);
	}
	painter.setPen(pen);
	painter.setBrush(brush);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
