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
#include <ovito/core/oo/OvitoObject.h>
#include "PrimitiveBase.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/**
 * \brief Abstract base class for rendering arrow glyphs and cylinders.
 */
class OVITO_CORE_EXPORT ArrowPrimitive : public PrimitiveBase
{
public:

	enum ShadingMode {
		NormalShading,
		FlatShading,
	};
	Q_ENUMS(ShadingMode);

	enum RenderingQuality {
		LowQuality,
		MediumQuality,
		HighQuality
	};
	Q_ENUMS(RenderingQuality);

	enum Shape {
		CylinderShape,
		ArrowShape,
	};
	Q_ENUMS(Shape);

public:

	/// Constructor.
	ArrowPrimitive(Shape shape, ShadingMode shadingMode, RenderingQuality renderingQuality, bool translucentElements) :
		_shape(shape), _shadingMode(shadingMode), _renderingQuality(renderingQuality), _translucentElements(translucentElements) {}

	/// \brief Allocates a geometry buffer with the given number of elements.
	virtual void startSetElements(int elementCount) = 0;

	/// \brief Returns the number of elements stored in the buffer.
	virtual int elementCount() const = 0;

	/// \brief Sets the properties of a single element.
	virtual void setElement(int index, const Point3& pos, const Vector3& dir, const ColorA& color, FloatType width) = 0;

	/// \brief Finalizes the geometry buffer after all elements have been set.
	virtual void endSetElements() = 0;

	/// \brief Returns the shading mode for elements.
	ShadingMode shadingMode() const { return _shadingMode; }

	/// \brief Changes the shading mode for elements.
	/// \return false if the shading mode cannot be changed after the buffer has been created; true otherwise.
	virtual bool setShadingMode(ShadingMode mode) { _shadingMode = mode; return true; }

	/// \brief Returns the rendering quality of elements.
	RenderingQuality renderingQuality() const { return _renderingQuality; }

	/// \brief Changes the rendering quality of elements.
	/// \return false if the quality level cannot be changed after the buffer has been created; true otherwise.
	virtual bool setRenderingQuality(RenderingQuality level) { _renderingQuality = level; return true; }

	/// \brief Returns the selected element shape.
	Shape shape() const { return _shape; }

	/// \brief Returns whether elements are displayed as semi-transparent if their alpha color value is smaller than one.
	bool translucentElements() const { return _translucentElements; }

private:

	/// Controls the shading.
	ShadingMode _shadingMode;

	/// Controls the rendering quality.
	RenderingQuality _renderingQuality;

	/// The shape of the elements.
	Shape _shape;

	/// Indicates whether some of the elements may be semi-transparent.
	/// If false, the alpha color value is ignored.
	bool _translucentElements;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::ArrowPrimitive::ShadingMode);
Q_DECLARE_METATYPE(Ovito::ArrowPrimitive::RenderingQuality);
Q_DECLARE_METATYPE(Ovito::ArrowPrimitive::Shape);
Q_DECLARE_TYPEINFO(Ovito::ArrowPrimitive::ShadingMode, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Ovito::ArrowPrimitive::RenderingQuality, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Ovito::ArrowPrimitive::Shape, Q_PRIMITIVE_TYPE);


