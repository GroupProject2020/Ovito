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

#include <ovito/core/Core.h>
#include "DefaultImagePrimitive.h"
#include "NonInteractiveSceneRenderer.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/******************************************************************************
* Returns true if the buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool DefaultImagePrimitive::isValid(SceneRenderer* renderer)
{
	// This buffer type works only in conjunction with a non-interactive renderer.
	return (qobject_cast<NonInteractiveSceneRenderer*>(renderer) != nullptr);
}

/******************************************************************************
* Renders the image in a rectangle given in viewport coordinates.
******************************************************************************/
void DefaultImagePrimitive::renderViewport(SceneRenderer* renderer, const Point2& pos, const Vector2& size)
{
	QSize imageSize = renderer->outputSize();
	Point2 windowPos((pos.x() + 1.0f) * imageSize.width() / 2, (-(pos.y() + size.y()) + 1.0) * imageSize.height() / 2);
	Vector2 windowSize(size.x() * imageSize.width() / 2, size.y() * imageSize.height() / 2);
	renderWindow(renderer, windowPos, windowSize);
}

/******************************************************************************
* Renders the image in a rectangle given in window coordinates.
******************************************************************************/
void DefaultImagePrimitive::renderWindow(SceneRenderer* renderer, const Point2& pos, const Vector2& size)
{
	NonInteractiveSceneRenderer* niRenderer = dynamic_object_cast<NonInteractiveSceneRenderer>(renderer);
	if(image().isNull() || !niRenderer || renderer->isPicking())
		return;

	niRenderer->renderImage(*this, pos, size);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
