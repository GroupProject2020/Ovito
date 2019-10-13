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
#include <ovito/core/rendering/ArrowPrimitive.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/**
 * \brief Buffer object that stores a set of arrows to be rendered by a non-interactive renderer.
 */
class OVITO_CORE_EXPORT DefaultArrowPrimitive : public ArrowPrimitive
{
public:

	/// Data structure that stores the data of a single arrow element.
	struct ArrowElement {
		Point3 pos;
		Vector3 dir;
		ColorA color;
		FloatType width;
	};

public:

	/// Constructor.
	using ArrowPrimitive::ArrowPrimitive;

	/// \brief Allocates a geometry buffer with the given number of elements.
	virtual void startSetElements(int elementCount) override {
		OVITO_ASSERT(elementCount >= 0);
		_elements.resize(elementCount);
	}

	/// \brief Returns the number of elements stored in the buffer.
	virtual int elementCount() const override { return _elements.size(); }

	/// \brief Sets the properties of a single line element.
	virtual void setElement(int index, const Point3& pos, const Vector3& dir, const ColorA& color, FloatType width) override;

	/// \brief Finalizes the geometry buffer after all elements have been set.
	virtual void endSetElements() override {}

	/// \brief Returns true if the geometry buffer is filled and can be rendered with the given renderer.
	virtual bool isValid(SceneRenderer* renderer) override;

	/// \brief Renders the geometry.
	virtual void render(SceneRenderer* renderer) override;

	/// Returns a reference to the internal buffer that stores the arrow geometry.
	const std::vector<ArrowElement>& elements() const { return _elements; }

private:

	/// The internal memory buffer for arrow elements.
	std::vector<ArrowElement> _elements;

};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


