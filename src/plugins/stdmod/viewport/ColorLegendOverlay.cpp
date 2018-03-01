///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2017) Alexander Stukowski
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

#include <plugins/stdmod/StdMod.h>
#include <core/viewport/Viewport.h>
#include <core/rendering/RenderSettings.h>
#include <core/app/Application.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/scene/RootSceneNode.h>
#include <core/dataset/scene/PipelineSceneNode.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "ColorLegendOverlay.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ColorLegendOverlay);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, alignment);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, orientation);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, legendSize);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, font);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, fontSize);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, offsetX);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, offsetY);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, aspectRatio);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, textColor);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, outlineColor);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, outlineEnabled);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, title);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, label1);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, label2);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, valueFormatString);
DEFINE_REFERENCE_FIELD(ColorLegendOverlay, modifier);	
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, alignment, "Position");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, orientation, "Orientation");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, legendSize, "Size factor");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, font, "Font");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, fontSize, "Font size");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, offsetX, "Offset X");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, offsetY, "Offset Y");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, aspectRatio, "Aspect ratio");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, textColor, "Font color");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, outlineColor, "Outline color");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, outlineEnabled, "Enable outline");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, title, "Title");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, label1, "Label 1");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, label2, "Label 2");
SET_PROPERTY_FIELD_UNITS(ColorLegendOverlay, offsetX, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS(ColorLegendOverlay, offsetY, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ColorLegendOverlay, legendSize, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ColorLegendOverlay, aspectRatio, FloatParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ColorLegendOverlay, fontSize, FloatParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
ColorLegendOverlay::ColorLegendOverlay(DataSet* dataset) : ViewportOverlay(dataset),
	_alignment(Qt::AlignHCenter | Qt::AlignBottom), 
	_orientation(Qt::Horizontal),
	_legendSize(0.3), 
	_offsetX(0), 
	_offsetY(0),
	_fontSize(0.1), 
	_valueFormatString("%g"), 
	_aspectRatio(8.0),
	_textColor(0,0,0),
	_outlineColor(1,1,1),
	_outlineEnabled(false)
{
	if(Application::instance()->guiMode()) {
		// Find a ColorCodingModifiers in the scene that we can connect to.
		dataset->sceneRoot()->visitObjectNodes([this](PipelineSceneNode* node) {
			PipelineObject* obj = node->dataProvider();
			while(obj) {
				if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(obj)) {
					if(ColorCodingModifier* mod = dynamic_object_cast<ColorCodingModifier>(modApp->modifier())) {
						setModifier(mod);
						if(mod->isEnabled())
							return false;	// Stop search.
					}
					obj = modApp->input();
				}
				else break;
			}
			return true;
		});
	}
}

/******************************************************************************
* This method asks the overlay to paint its contents over the given viewport.
******************************************************************************/
void ColorLegendOverlay::render(Viewport* viewport, TimePoint time, QPainter& painter, 
						const ViewProjectionParameters& projParams, RenderSettings* renderSettings,
						bool interactiveViewport, TaskManager& taskManager)
{
	// Check whether a Color Coding modifier has been wired to this color legend:
	if(!modifier()) {
		if(Application::instance()->consoleMode()) {
			throwException(tr("You are trying to render a Viewport with a ColorLegendOverlay whose 'modifier' property has "
							  "not been linked to a ColorCodingModifier. Did you forget to assign it?"));
		}
		else {
			// Ignore invalid configuration in GUI mode by not rendering the legend.
			return;
		}
	}

	FloatType legendSize = this->legendSize() * renderSettings->outputImageHeight();
	if(legendSize <= 0) return;

	FloatType colorBarWidth = legendSize;
	FloatType colorBarHeight = colorBarWidth / std::max(FloatType(0.01), aspectRatio());
	bool vertical = (orientation() == Qt::Vertical);
	if(vertical)
		std::swap(colorBarWidth, colorBarHeight);

	QPointF origin(offsetX() * renderSettings->outputImageWidth(), -offsetY() * renderSettings->outputImageHeight());
	FloatType hmargin = FloatType(0.01) * renderSettings->outputImageWidth();
	FloatType vmargin = FloatType(0.01) * renderSettings->outputImageHeight();

	if(alignment() & Qt::AlignLeft) origin.rx() += hmargin;
	else if(alignment() & Qt::AlignRight) origin.rx() += renderSettings->outputImageWidth() - hmargin - colorBarWidth;
	else if(alignment() & Qt::AlignHCenter) origin.rx() += FloatType(0.5) * renderSettings->outputImageWidth() - FloatType(0.5) * colorBarWidth;

	if(alignment() & Qt::AlignTop) origin.ry() += vmargin;
	else if(alignment() & Qt::AlignBottom) origin.ry() += renderSettings->outputImageHeight() - vmargin - colorBarHeight;
	else if(alignment() & Qt::AlignVCenter) origin.ry() += FloatType(0.5) * renderSettings->outputImageHeight() - FloatType(0.5) * colorBarHeight;

	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

	// Create the color scale image.
	int imageSize = 256;
	QImage image(vertical ? 1 : imageSize, vertical ? imageSize : 1, QImage::Format_RGB32);
	for(int i = 0; i < imageSize; i++) {
		FloatType t = (FloatType)i / (FloatType)(imageSize - 1);
		Color color = modifier()->colorGradient()->valueToColor(vertical ? (FloatType(1) - t) : t);
		image.setPixel(vertical ? 0 : i, vertical ? i : 0, QColor(color).rgb());
	}
	painter.drawImage(QRectF(origin, QSizeF(colorBarWidth, colorBarHeight)), image);

	qreal fontSize = legendSize * std::max(FloatType(0), this->fontSize());
	if(fontSize == 0) return;
	QFont font = this->font();

	// Always render the outline pen 3 pixels wide, irrespective of frame buffer resolution.
	qreal outlineWidth = 3.0 / painter.combinedTransform().m11();
	painter.setPen(QPen(QBrush(outlineColor()), outlineWidth));

	// Get modifier's parameters.
	FloatType startValue = modifier()->startValue();
	FloatType endValue = modifier()->endValue();

	QByteArray format = valueFormatString().toUtf8();
	if(format.contains("%s")) format.clear();

	QString titleLabel, topLabel, bottomLabel;
	if(label1().isEmpty())
		topLabel.sprintf(format.constData(), endValue);
	else
		topLabel = label1();
	if(label2().isEmpty())
		bottomLabel.sprintf(format.constData(), startValue);
	else
		bottomLabel = label2();
	if(title().isEmpty())
		titleLabel = modifier()->sourceProperty().nameWithComponent();
	else
		titleLabel = title();

	font.setPointSizeF(fontSize);
	painter.setFont(font);

	qreal textMargin = 0.2 * legendSize / std::max(FloatType(0.01), aspectRatio());

	bool drawOutline = outlineEnabled();

	// Create text at QPainterPaths so that we can easily draw an outline around the text
	QPainterPath titlePath = QPainterPath();
	titlePath.addText(origin, font, titleLabel);

	// QPainterPath::addText uses the baseline as point where text is drawn. Compensate for this.
	titlePath.translate(0, -QFontMetrics(font).descent());

	QRectF titleBounds = titlePath.boundingRect();

	// Move the text path to the correct place based on colorbar direction and position
	if(!vertical || (alignment() & Qt::AlignHCenter)) {
		// Q: Why factor 0.5 ?
		titlePath.translate(0.5 * colorBarWidth - titleBounds.width()/2.0, -0.5 * textMargin);
	}
	else {
		if(alignment() & Qt::AlignLeft)
			titlePath.translate(0, -textMargin);
		else if(alignment() & Qt::AlignRight)
			titlePath.translate(-titleBounds.width(), -textMargin);
	}

	if(drawOutline) painter.drawPath(titlePath);
	painter.fillPath(titlePath, (QColor)textColor());

	font.setPointSizeF(fontSize * 0.8);
	painter.setFont(font);

	QPainterPath topPath = QPainterPath();
	QPainterPath bottomPath = QPainterPath();

	topPath.addText(origin, font, topLabel);
	bottomPath.addText(origin, font, bottomLabel);

	QRectF bottomBounds = bottomPath.boundingRect();
	QRectF topBounds = topPath.boundingRect();

	if(!vertical) {
		bottomPath.translate(-textMargin - bottomBounds.width(), 0.5*colorBarHeight + bottomBounds.height()/2.0);
		topPath.translate(colorBarWidth + textMargin, 0.5*colorBarHeight + topBounds.height()/2.0);
	}
	else {
		topPath.translate(0, topBounds.height());
		if(alignment() & Qt::AlignLeft) {
			topPath.translate(colorBarWidth + textMargin, 0);
			bottomPath.translate(colorBarWidth + textMargin, colorBarHeight);
		}
		else if(alignment() & Qt::AlignRight) {
			topPath.translate(-textMargin -topBounds.width(), 0);
			bottomPath.translate(-textMargin - bottomBounds.width(), colorBarHeight);
		}
		else if(alignment() & Qt::AlignHCenter) { // Q: Same as Qt:AlignLeft case on purpose?
			topPath.translate(colorBarWidth + textMargin, 0);
			bottomPath.translate(colorBarWidth + textMargin, colorBarHeight);
		}
	}

	if(drawOutline) painter.drawPath(topPath);
	painter.fillPath(topPath, (QColor)textColor());

	if(drawOutline) painter.drawPath(bottomPath);
	painter.fillPath(bottomPath, (QColor)textColor());
}

}	// End of namespace
}	// End of namespace
