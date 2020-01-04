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


#include <ovito/stdmod/StdMod.h>
#include <ovito/stdmod/modifiers/ColorCodingModifier.h>
#include <ovito/core/viewport/overlays/ViewportOverlay.h>
#include <ovito/core/rendering/FrameBuffer.h>

namespace Ovito { namespace StdMod {

/**
 * \brief A viewport overlay that displays the color legend of a ColorCodingModifier.
 */
class OVITO_STDMOD_EXPORT ColorLegendOverlay : public ViewportOverlay
{
	Q_OBJECT
	OVITO_CLASS(ColorLegendOverlay)
	Q_CLASSINFO("DisplayName", "Color legend");

public:

	/// \brief Constructor.
	Q_INVOKABLE ColorLegendOverlay(DataSet* dataset);

	/// This method asks the overlay to paint its contents over the rendered image.
	virtual void render(const Viewport* viewport, TimePoint time, FrameBuffer* frameBuffer, const ViewProjectionParameters& projParams, const RenderSettings* renderSettings, AsyncOperation& operation) override {
		QPainter painter(&frameBuffer->image());
		renderImplementation(painter, projParams, renderSettings);
	}

	/// This method asks the overlay to paint its contents over the given interactive viewport.
	virtual void renderInteractive(const Viewport* viewport, TimePoint time, QPainter& painter, const ViewProjectionParameters& projParams, const RenderSettings* renderSettings, AsyncOperation& operation) override {
		renderImplementation(painter, projParams, renderSettings);
	}

	/// Moves the position of the overlay in the viewport by the given amount,
	/// which is specified as a fraction of the viewport render size.
	virtual void moveLayerInViewport(const Vector2& delta) override {
		setOffsetX(offsetX() + delta.x());
		setOffsetY(offsetY() + delta.y());
	}

public:

	Q_PROPERTY(Ovito::StdMod::ColorCodingModifier* modifier READ modifier WRITE setModifier);

private:

	/// This method paints the overlay contents onto the given canvas.
	void renderImplementation(QPainter& painter, const ViewProjectionParameters& projParams, const RenderSettings* renderSettings);

	/// The corner of the viewport where the color legend is displayed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, alignment, setAlignment, PROPERTY_FIELD_MEMORIZE);

	/// The orientation (horizontal/vertical) of the color legend.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, orientation, setOrientation, PROPERTY_FIELD_MEMORIZE);

	/// Controls the overall size of the color legend.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, legendSize, setLegendSize, PROPERTY_FIELD_MEMORIZE);

	/// Controls the aspect ration of the color bar.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, aspectRatio, setAspectRatio, PROPERTY_FIELD_MEMORIZE);

	/// Controls the horizontal offset of legend position.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, offsetX, setOffsetX)

	/// Controls the vertical offset of legend position.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, offsetY, setOffsetY);

	/// Controls the label font.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(QFont, font, setFont, PROPERTY_FIELD_MEMORIZE);

	/// Controls the label font size.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, fontSize, setFontSize, PROPERTY_FIELD_MEMORIZE);

	/// The title label.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, title, setTitle);

	/// User-defined text for the first numeric label.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, label1, setLabel1);

	/// User-defined text for the second numeric label.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, label2, setLabel2);

	/// The ColorCodingModifier for which to display the legend.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(ColorCodingModifier, modifier, setModifier, PROPERTY_FIELD_NO_SUB_ANIM);

	/// Controls the formatting of the value labels in the color legend.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, valueFormatString, setValueFormatString);

	/// Controls the text color.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Color, textColor, setTextColor, PROPERTY_FIELD_MEMORIZE);

	/// The text outline color.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Color, outlineColor, setOutlineColor, PROPERTY_FIELD_MEMORIZE);

	/// Controls the outlining of the font.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, outlineEnabled, setOutlineEnabled, PROPERTY_FIELD_MEMORIZE)
};

}	// End of namespace
}	// End of namespace
