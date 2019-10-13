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
 * \brief Abstract base class for particle drawing primitives.
 */
class OVITO_CORE_EXPORT ParticlePrimitive : public PrimitiveBase
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
		HighQuality,
		AutoQuality
	};
	Q_ENUMS(RenderingQuality);

	enum ParticleShape {
		SphericalShape,
		SquareCubicShape,
		BoxShape,
		EllipsoidShape
	};
	Q_ENUMS(ParticleShape);

public:

	/// Constructor.
	ParticlePrimitive(ShadingMode shadingMode, RenderingQuality renderingQuality, ParticleShape shape, bool translucentParticles)
		: _shadingMode(shadingMode), _renderingQuality(renderingQuality), _particleShape(shape), _translucentParticles(translucentParticles) {}

	/// \brief Allocates a geometry buffer with the given number of particles.
	virtual void setSize(int particleCount) = 0;

	/// \brief Returns the number of particles stored in the buffer.
	virtual int particleCount() const = 0;

	/// \brief Sets the coordinates of the particles.
	virtual void setParticlePositions(const Point3* coordinates) = 0;

	/// \brief Sets the radii of the particles.
	virtual void setParticleRadii(const FloatType* radii) = 0;

	/// \brief Sets the radius of all particles to the given value.
	virtual void setParticleRadius(FloatType radius) = 0;

	/// \brief Sets the colors of the particles.
	virtual void setParticleColors(const ColorA* colors) = 0;

	/// \brief Sets the colors of the particles.
	virtual void setParticleColors(const Color* colors) = 0;

	/// \brief Sets the color of all particles to the given value.
	virtual void setParticleColor(const ColorA color) = 0;

	/// \brief Sets the aspherical shape of the particles.
	virtual void setParticleShapes(const Vector3* shapes) = 0;

	/// \brief Sets the aspherical shape of the particles.
	virtual void setParticleOrientations(const Quaternion* orientations) = 0;

	/// \brief Resets the aspherical shape of the particles.
	virtual void clearParticleShapes() = 0;

	/// \brief Resets the orientation of particles.
	virtual void clearParticleOrientations() = 0;

	/// \brief Returns the shading mode for particles.
	ShadingMode shadingMode() const { return _shadingMode; }

	/// \brief Changes the shading mode for particles.
	/// \return false if the shading mode cannot be changed after the buffer has been created; true otherwise.
	virtual bool setShadingMode(ShadingMode mode) { _shadingMode = mode; return true; }

	/// \brief Returns the rendering quality of particles.
	RenderingQuality renderingQuality() const { return _renderingQuality; }

	/// \brief Changes the rendering quality of particles.
	/// \return false if the quality level cannot be changed after the buffer has been created; true otherwise.
	virtual bool setRenderingQuality(RenderingQuality level) { _renderingQuality = level; return true; }

	/// \brief Returns the display shape of particles.
	ParticleShape particleShape() const { return _particleShape; }

	/// \brief Changes the display shape of particles.
	/// \return false if the shape cannot be changed after the buffer has been created; true otherwise.
	virtual bool setParticleShape(ParticleShape shape) { _particleShape = shape; return true; }

	/// \brief Returns whether particles are displayed as semi-transparent if their alpha color value is smaller than one.
	bool translucentParticles() const { return _translucentParticles; }

private:

	/// Controls the shading of particles.
	ShadingMode _shadingMode;

	/// Controls the rendering quality of particles.
	RenderingQuality _renderingQuality;

	/// Controls the shape of particles.
	ParticleShape _particleShape;

	/// Indicates whether some of the particles may be semi-transparent.
	/// If false, the alpha color value is ignored.
	bool _translucentParticles;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::ParticlePrimitive::ShadingMode);
Q_DECLARE_METATYPE(Ovito::ParticlePrimitive::RenderingQuality);
Q_DECLARE_METATYPE(Ovito::ParticlePrimitive::ParticleShape);
Q_DECLARE_TYPEINFO(Ovito::ParticlePrimitive::ShadingMode, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Ovito::ParticlePrimitive::RenderingQuality, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Ovito::ParticlePrimitive::ParticleShape, Q_PRIMITIVE_TYPE);
