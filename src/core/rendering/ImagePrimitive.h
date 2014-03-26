///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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

/**
 * \file ImagePrimitive.h
 * \brief Contains the definition of the Ovito::ImagePrimitive class.
 */

#ifndef __OVITO_IMAGE_PRIMITIVE_H
#define __OVITO_IMAGE_PRIMITIVE_H

#include <core/Core.h>

namespace Ovito {

/**
 * \brief Abstract base class for drawing bitmaps.
 */
class OVITO_CORE_EXPORT ImagePrimitive
{
public:

	/// \brief Virtual base constructor.
	virtual ~ImagePrimitive() {}

	/// \brief Sets the text to be rendered.
	virtual void setImage(const QImage& image) { _image = image; }

	/// \brief Returns the image stored in the buffer.
	const QImage& image() const { return _image; }

	/// \brief Returns true if the buffer is filled and can be rendered with the given renderer.
	virtual bool isValid(SceneRenderer* renderer) = 0;

	/// \brief Renders the image in a rectangle given in pixel coordinates.
	virtual void renderWindow(SceneRenderer* renderer, const Point2& pos, const Vector2& size) = 0;

	/// \brief Renders the image in a rectangle given in viewport coordinates.
	virtual void renderViewport(SceneRenderer* renderer, const Point2& pos, const Vector2& size) = 0;

private:

	/// The image to be rendered.
	QImage _image;
};

};

#endif // __OVITO_IMAGE_PRIMITIVE_H