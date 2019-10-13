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
#include "DefaultArrowPrimitive.h"
#include "NonInteractiveSceneRenderer.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/******************************************************************************
* Sets the properties of a single element.
******************************************************************************/
void DefaultArrowPrimitive::setElement(int index, const Point3& pos, const Vector3& dir, const ColorA& color, FloatType width)
{
	OVITO_ASSERT(index >= 0 && index < _elements.size());
	ArrowElement& elmnt = _elements[index];

	elmnt.pos = pos;
	elmnt.dir = dir;
	elmnt.color = color;
	elmnt.width = width;
}

/******************************************************************************
* Returns true if the geometry buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool DefaultArrowPrimitive::isValid(SceneRenderer* renderer)
{
	// This buffer type works only in conjunction with a non-interactive renderer.
	return (qobject_cast<NonInteractiveSceneRenderer*>(renderer) != nullptr);
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void DefaultArrowPrimitive::render(SceneRenderer* renderer)
{
	NonInteractiveSceneRenderer* niRenderer = dynamic_object_cast<NonInteractiveSceneRenderer>(renderer);
	if(_elements.empty() || !niRenderer || renderer->isPicking())
		return;

	niRenderer->renderArrows(*this);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
