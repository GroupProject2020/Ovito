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
#include <ovito/core/rendering/ParticlePrimitive.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/**
 * \brief Buffer object that stores a set of particles to be rendered by a non-interactive renderer.
 */
class OVITO_CORE_EXPORT DefaultParticlePrimitive : public ParticlePrimitive
{
public:

	/// Constructor.
	using ParticlePrimitive::ParticlePrimitive;

	/// \brief Allocates a geometry buffer with the given number of particles.
	virtual void setSize(int particleCount) override {
		OVITO_ASSERT(particleCount >= 0);
		_positionsBuffer.resize(particleCount);
		_radiiBuffer.resize(particleCount);
		_colorsBuffer.resize(particleCount);
	}

	/// \brief Returns the number of particles stored in the buffer.
	virtual int particleCount() const override { return _positionsBuffer.size(); }

	/// \brief Sets the coordinates of the particles.
	virtual void setParticlePositions(const Point3* coordinates) override {
		std::copy(coordinates, coordinates + _positionsBuffer.size(), _positionsBuffer.begin());
	}

	/// \brief Sets the radii of the particles.
	virtual void setParticleRadii(const FloatType* radii) override {
		std::copy(radii, radii + _radiiBuffer.size(), _radiiBuffer.begin());
	}

	/// \brief Sets the radius of all particles to the given value.
	virtual void setParticleRadius(FloatType radius) override {
		std::fill(_radiiBuffer.begin(), _radiiBuffer.end(), radius);
	}

	/// \brief Sets the colors of the particles.
	virtual void setParticleColors(const ColorA* colors) override {
		std::copy(colors, colors + _colorsBuffer.size(), _colorsBuffer.begin());
	}

	/// \brief Sets the colors of the particles.
	virtual void setParticleColors(const Color* colors) override {
		std::copy(colors, colors + _colorsBuffer.size(), _colorsBuffer.begin());
	}

	/// \brief Sets the color of all particles to the given value.
	virtual void setParticleColor(const ColorA color) override {
		std::fill(_colorsBuffer.begin(), _colorsBuffer.end(), color);
	}

	/// \brief Sets the aspherical shapes of the particles.
	virtual void setParticleShapes(const Vector3* shapes) override {
		_shapesBuffer.resize(particleCount());
		std::copy(shapes, shapes + _shapesBuffer.size(), _shapesBuffer.begin());
	}

	/// \brief Sets the orientation of aspherical particles.
	virtual void setParticleOrientations(const Quaternion* orientations) override {
		_orientationsBuffer.resize(particleCount());
		std::copy(orientations, orientations + _orientationsBuffer.size(), _orientationsBuffer.begin());
	}

	/// \brief Resets the aspherical shape of the particles.
	virtual void clearParticleShapes() override {
		_shapesBuffer.clear();
	}

	/// \brief Resets the orientation of particles.
	virtual void clearParticleOrientations() override {
		_orientationsBuffer.clear();
	}

	/// \brief Returns true if the geometry buffer is filled and can be rendered with the given renderer.
	virtual bool isValid(SceneRenderer* renderer) override;

	/// \brief Renders the geometry.
	virtual void render(SceneRenderer* renderer) override;

	/// Returns a reference to the internal buffer that stores the particle positions.
	const std::vector<Point3>& positions() const { return _positionsBuffer; }

	/// Returns a reference to the internal buffer that stores the particle radii.
	const std::vector<FloatType>& radii() const { return _radiiBuffer; }

	/// Returns a reference to the internal buffer that stores the particle colors.
	const std::vector<ColorA>& colors() const { return _colorsBuffer; }

	/// Returns a reference to the internal buffer that stores the shapes of aspherical particles.
	const std::vector<Vector3>& shapes() const { return _shapesBuffer; }

	/// Returns a reference to the internal buffer that stores the orientations of aspherical particles.
	const std::vector<Quaternion>& orientations() const { return _orientationsBuffer; }

private:

	/// The internal buffer that stores the particle positions.
	std::vector<Point3> _positionsBuffer;

	/// The internal buffer that stores the particle radii.
	std::vector<FloatType> _radiiBuffer;

	/// The internal buffer that stores the particle colors and alpha values.
	std::vector<ColorA> _colorsBuffer;

	/// The internal buffer that stores the shapes of aspherical particles.
	std::vector<Vector3> _shapesBuffer;

	/// The internal buffer that stores the orientations of aspherical particles.
	std::vector<Quaternion> _orientationsBuffer;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


