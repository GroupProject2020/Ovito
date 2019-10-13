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
 * \brief Abstract base class for drawing bitmaps in the viewports.
 */
class OVITO_CORE_EXPORT ImagePrimitive : public PrimitiveBase
{
public:

	/// \brief Sets the mage to be rendered.
	virtual void setImage(const QImage& image) { _image = image; }

	/// \brief Returns the image stored in the buffer.
	const QImage& image() const { return _image; }

	/// \brief Renders the image in a rectangle given in pixel coordinates.
	virtual void renderWindow(SceneRenderer* renderer, const Point2& pos, const Vector2& size) = 0;

	/// \brief Renders the image in a rectangle given in viewport coordinates.
	virtual void renderViewport(SceneRenderer* renderer, const Point2& pos, const Vector2& size) = 0;

	/// \brief Renders the primitive using the given renderer.
	virtual void render(SceneRenderer* renderer) override {}

private:

	/// The image to be rendered.
	QImage _image;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


