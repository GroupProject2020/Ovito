////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
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
#include <ovito/core/rendering/MarkerPrimitive.h>
#include "OpenGLBuffer.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief This class is responsible for rendering marker primitives using OpenGL.
 */
class OpenGLMarkerPrimitive : public MarkerPrimitive, public std::enable_shared_from_this<OpenGLMarkerPrimitive>
{
public:

	/// Constructor.
	OpenGLMarkerPrimitive(OpenGLSceneRenderer* renderer, MarkerShape shape);

	/// \brief Allocates a geometry buffer with the given number of markers.
	virtual void setCount(int markerCount) override;

	/// \brief Returns the number of markers stored in the buffer.
	virtual int markerCount() const override { return _markerCount; }

	/// \brief Sets the coordinates of the markers.
	virtual void setMarkerPositions(const Point3* positions) override;

	/// \brief Sets the color of all markers to the given value.
	virtual void setMarkerColor(const ColorA color) override;

	/// \brief Returns true if the geometry buffer is filled and can be rendered with the given renderer.
	virtual bool isValid(SceneRenderer* renderer) override;

	/// \brief Renders the geometry.
	virtual void render(SceneRenderer* renderer) override;

private:

	/// The number of markers stored in the class.
	int _markerCount;

	/// The internal OpenGL vertex buffer that stores the marker positions.
	OpenGLBuffer<Point_3<float>> _positionBuffer;

	/// The internal OpenGL vertex buffer that stores the marker colors.
	OpenGLBuffer<ColorAT<float>> _colorBuffer;

	/// The GL context group under which the GL vertex buffers have been created.
	QPointer<QOpenGLContextGroup> _contextGroup;

	/// The OpenGL shader program that is used to render the markers.
	QOpenGLShaderProgram* _shader;

	/// The OpenGL shader program that is used to render the markers in picking mode.
	QOpenGLShaderProgram* _pickingShader;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


