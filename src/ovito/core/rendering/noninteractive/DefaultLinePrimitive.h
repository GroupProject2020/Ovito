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
#include <ovito/core/rendering/LinePrimitive.h>

namespace Ovito {

/**
 * \brief Buffer object that stores line geometry to be rendered by a non-interactive renderer.
 */
class OVITO_CORE_EXPORT DefaultLinePrimitive : public LinePrimitive
{
public:

	/// Constructor.
	using LinePrimitive::LinePrimitive;

	/// \brief Allocates a geometry buffer with the given number of vertices.
	virtual void setVertexCount(int vertexCount, FloatType lineWidth) override {
		OVITO_ASSERT(vertexCount >= 0);
		_positionsBuffer.resize(vertexCount);
		_colorsBuffer.resize(vertexCount);
	}

	/// \brief Returns the number of vertices stored in the buffer.
	virtual int vertexCount() const override { return _positionsBuffer.size(); }

	/// \brief Sets the coordinates of the vertices.
	virtual void setVertexPositions(const Point3* coordinates) override {
		std::copy(coordinates, coordinates + _positionsBuffer.size(), _positionsBuffer.begin());
	}

	/// \brief Sets the colors of the vertices.
	virtual void setVertexColors(const ColorA* colors) override {
		std::copy(colors, colors + _colorsBuffer.size(), _colorsBuffer.begin());
	}

	/// \brief Sets the color of all vertices to the given value.
	virtual void setLineColor(const ColorA color) override {
		std::fill(_colorsBuffer.begin(), _colorsBuffer.end(), color);
	}

	/// \brief Returns true if the geometry buffer is filled and can be rendered with the given renderer.
	virtual bool isValid(SceneRenderer* renderer) override;

	/// \brief Renders the geometry.
	virtual void render(SceneRenderer* renderer) override;

	/// Returns a reference to the internal buffer that stores the vertex positions.
	const std::vector<Point3>& positions() const { return _positionsBuffer; }

		/// Returns a reference to the internal buffer that stores the vertex colors.
	const std::vector<ColorA>& colors() const { return _colorsBuffer; }

private:

	/// The buffer that stores the vertex positions.
	std::vector<Point3> _positionsBuffer;

	/// The buffer that stores the vertex colors.
	std::vector<ColorA> _colorsBuffer;

};

}	// End of namespace


