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


#include <ovito/core/Core.h>
#include "PrimitiveBase.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/**
 * \brief Abstract base class rendering of text primitives.
 */
class OVITO_CORE_EXPORT TextPrimitive : public PrimitiveBase
{
public:

	/// \brief Default constructor.
	TextPrimitive() : _color(1,1,1,1), _backgroundColor(0,0,0,0) {}

	/// \brief Sets the text to be rendered.
	virtual void setText(const QString& text) { _text = text; }

	/// \brief Returns the number of vertices stored in the buffer.
	const QString& text() const { return _text; }

	/// \brief Sets the text color.
	virtual void setColor(const ColorA& color) { _color = color; }

	/// \brief Returns the text color.
	const ColorA& color() const { return _color; }

	/// \brief Sets the text background color.
	virtual void setBackgroundColor(const ColorA& color) { _backgroundColor = color; }

	/// \brief Returns the text background color.
	const ColorA& backgroundColor() const { return _backgroundColor; }

	/// Sets the text font.
	virtual void setFont(const QFont& font) { _font = font; }

	/// Returns the text font.
	const QFont& font() const { return _font; }

	/// \brief Renders the text string at the given 2D window (pixel) coordinates.
	virtual void renderWindow(SceneRenderer* renderer, const Point2& pos, int alignment = Qt::AlignLeft | Qt::AlignTop) = 0;

	/// \brief Renders the text string at the given 2D normalized viewport coordinates ([-1,+1] range).
	virtual void renderViewport(SceneRenderer* renderer, const Point2& pos, int alignment = Qt::AlignLeft | Qt::AlignTop) = 0;

	/// \brief Renders the primitive using the given renderer.
	virtual void render(SceneRenderer* renderer) override {}

private:

	/// The text to be rendered.
	QString _text;

	/// The text color.
	ColorA _color;

	/// The text background color.
	ColorA _backgroundColor;

	/// The text font.
	QFont _font;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


