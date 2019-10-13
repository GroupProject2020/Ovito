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
#include "DefaultTextPrimitive.h"
#include "NonInteractiveSceneRenderer.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/******************************************************************************
* Returns true if the buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool DefaultTextPrimitive::isValid(SceneRenderer* renderer)
{
	// This buffer type works only in conjunction with a non-interactive renderer.
	return (qobject_cast<NonInteractiveSceneRenderer*>(renderer) != nullptr);
}

/******************************************************************************
* Renders the text string at the given location given in normalized
* viewport coordinates ([-1,+1] range).
******************************************************************************/
void DefaultTextPrimitive::renderViewport(SceneRenderer* renderer, const Point2& pos, int alignment)
{
	QSize imageSize = renderer->outputSize();
	Point2 windowPos((pos.x() + 1.0f) * imageSize.width() / 2, (-pos.y() + 1.0f) * imageSize.height() / 2);
	renderWindow(renderer, windowPos, alignment);
}

/******************************************************************************
* Renders the text string at the given 2D window (pixel) coordinates.
******************************************************************************/
void DefaultTextPrimitive::renderWindow(SceneRenderer* renderer, const Point2& pos, int alignment)
{
	NonInteractiveSceneRenderer* niRenderer = dynamic_object_cast<NonInteractiveSceneRenderer>(renderer);
	if(text().isEmpty() || !niRenderer || renderer->isPicking())
		return;

	niRenderer->renderText(*this, pos, alignment);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
