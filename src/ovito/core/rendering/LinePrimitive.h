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
 * \brief Abstract base class for line drawing primitives.
 */
class OVITO_CORE_EXPORT LinePrimitive : public PrimitiveBase
{
public:

	/// \brief Allocates a geometry buffer with the given number of vertices.
	virtual void setVertexCount(int vertexCount, FloatType lineWidth = 0) = 0;

	/// \brief Returns the number of vertices stored in the buffer.
	virtual int vertexCount() const = 0;

	/// \brief Sets the coordinates of the vertices.
	virtual void setVertexPositions(const Point3* coordinates) = 0;

	/// \brief Sets the colors of the vertices.
	virtual void setVertexColors(const ColorA* colors) = 0;

	/// \brief Sets the color of all vertices to the given value.
	virtual void setLineColor(const ColorA color) = 0;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


