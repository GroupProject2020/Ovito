////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/core/rendering/TextPrimitive.h>
#include <ovito/core/rendering/ImagePrimitive.h>
#include "OpenGLSceneRenderer.h"

namespace Ovito {

/**
 * \brief Buffer object that stores a text string to be rendered in the viewports.
 */
class OpenGLTextPrimitive : public TextPrimitive
{
public:

	/// Constructor.
	OpenGLTextPrimitive(OpenGLSceneRenderer* renderer);

	/// \brief Sets the text to be rendered.
	virtual void setText(const QString& text) override {
		if(text != this->text())
			_needImageUpdate = true;
		TextPrimitive::setText(text);
	}

	/// Sets the text font.
	virtual void setFont(const QFont& font) override {
		if(font != this->font())
			_needImageUpdate = true;
		TextPrimitive::setFont(font);
	}

	/// Sets the text color.
	virtual void setColor(const ColorA& color) override {
		if(color != this->color())
			_needImageUpdate = true;
		TextPrimitive::setColor(color);
	}

	/// Sets the text background color.
	virtual void setBackgroundColor(const ColorA& color) override {
		if(color != this->backgroundColor())
			_needImageUpdate = true;
		TextPrimitive::setBackgroundColor(color);
	}

	/// \brief Returns true if the geometry buffer is filled and can be rendered with the given renderer.
	virtual bool isValid(SceneRenderer* renderer) override;

	/// \brief Renders the text string at the given 2D window (pixel) coordinates.
	virtual void renderWindow(SceneRenderer* renderer, const Point2& pos, int alignment = Qt::AlignLeft | Qt::AlignTop) override;

	/// \brief Renders the text string at the given 2D normalized viewport coordinates ([-1,+1] range).
	virtual void renderViewport(SceneRenderer* renderer, const Point2& pos, int alignment = Qt::AlignLeft | Qt::AlignTop) override;

private:

	/// The GL context group under which the GL vertex buffer has been created.
	QOpenGLContextGroup* _contextGroup;

	/// The pre-rendered text.
	std::shared_ptr<ImagePrimitive> _imageBuffer;

	/// The position of the text inside the texture image.
	QPoint _textOffset;

	/// Indicates that the pre-rendered image needs to be updated.
	bool _needImageUpdate;
};

}	// End of namespace
