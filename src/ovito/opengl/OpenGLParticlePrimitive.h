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
#include "OpenGLBuffer.h"
#include "OpenGLTexture.h"

namespace Ovito {

/**
 * \brief This class is responsible for rendering particle primitives using OpenGL.
 */
class OpenGLParticlePrimitive : public ParticlePrimitive, public std::enable_shared_from_this<OpenGLParticlePrimitive>
{
public:

	/// Constructor.
	OpenGLParticlePrimitive(OpenGLSceneRenderer* renderer,
			ShadingMode shadingMode, RenderingQuality renderingQuality, ParticleShape shape, bool translucentParticles);

	/// \brief Allocates a geometry buffer with the given number of particles.
	virtual void setSize(int particleCount) override;

	/// \brief Returns the number of particles stored in the buffer.
	virtual int particleCount() const override { return _particleCount; }

	/// \brief Sets the coordinates of the particles.
	virtual void setParticlePositions(const Point3* positions) override;

	/// \brief Sets the radii of the particles.
	virtual void setParticleRadii(const FloatType* radii) override;

	/// \brief Sets the radius of all particles to the given value.
	virtual void setParticleRadius(FloatType radius) override;

	//Begin of modification
	/// \brief Sets the transparency of the particles.
	virtual void setParticleTransparencies(const FloatType* transparencies) override;

	/// \brief Sets the transparency of all particles to the given value.
	virtual void setParticleTransparency(FloatType transparency) override;
	//End of modification

	/// \brief Sets the colors of the particles.
	virtual void setParticleColors(const ColorA* colors) override;

	/// \brief Sets the colors of the particles.
	virtual void setParticleColors(const Color* colors) override;

	/// \brief Sets the color of all particles to the given value.
	virtual void setParticleColor(const ColorA color) override;

	/// \brief Sets the aspherical shapes of the particles.
	virtual void setParticleShapes(const Vector3* shapes) override;

	/// \brief Sets the orientation of aspherical particles.
	virtual void setParticleOrientations(const Quaternion* orientations) override;

	/// \brief Resets the aspherical shape of the particles.
	virtual void clearParticleShapes() override;

	/// \brief Resets the orientation of particles.
	virtual void clearParticleOrientations() override;

	/// \brief Returns true if the geometry buffer is filled and can be rendered with the given renderer.
	virtual bool isValid(SceneRenderer* renderer) override;

	/// \brief Renders the geometry.
	virtual void render(SceneRenderer* renderer) override;

	/// \brief Changes the shading mode for particles.
	virtual bool setShadingMode(ShadingMode mode) override { return (mode == shadingMode()); }

	/// \brief Changes the rendering quality of particles.
	virtual bool setRenderingQuality(RenderingQuality level) override { return (level == renderingQuality()); }

	/// \brief Changes the display shape of particles.
	virtual bool setParticleShape(ParticleShape shape) override { return (shape == particleShape()); }

protected:

	/// Creates the texture used for billboard rendering of particles.
	void initializeBillboardTexture(OpenGLSceneRenderer* renderer);

	/// Activates a texture for billboard rendering of particles.
	void activateBillboardTexture(OpenGLSceneRenderer* renderer);

	/// Deactivates the texture used for billboard rendering of spherical particles.
	void deactivateBillboardTexture(OpenGLSceneRenderer* renderer);

	/// Renders the particles using OpenGL point sprites.
	void renderPointSprites(OpenGLSceneRenderer* renderer);

	/// Renders a box for each particle using triangle strips.
	void renderBoxes(OpenGLSceneRenderer* renderer);

	/// Renders the particles using quads.
	void renderImposters(OpenGLSceneRenderer* renderer);

	/// Returns an array of particle indices, sorted back-to-front, which is used to render translucent particles.
	std::vector<GLuint> determineRenderingOrder(OpenGLSceneRenderer* renderer);

private:

	/// The available techniques for rendering particles.
	enum RenderingTechnique {
		POINT_SPRITES,	///< Use OpenGL point sprites to render imposter quads with a texture map.
		IMPOSTER_QUADS,	///< Render explicit quad geometry made of two triangles.
		BOX_GEOMETRY	///< Render a box for each particle (possibly using a raytracing fragment shader to make it look spherical).
	};

	/// The maximum size (in bytes) of a single VBO buffer.
	int _maxVBOSize = 4 * 1024 * 1024;

	/// The maximum number of render elements per VBO buffer.
	int _chunkSize = 0;

	/// The number of particles stored in the class.
	int _particleCount = -1;

	/// The internal OpenGL vertex buffer that stores the particle positions.
	std::vector<OpenGLBuffer<Point_3<float>>> _positionsBuffers;

	//Begin of modification
	std::vector<OpenGLBuffer<float>> _transparenciesBuffers;
	//End of modification

	/// The internal OpenGL vertex buffer that stores the particle radii.
	std::vector<OpenGLBuffer<float>> _radiiBuffers;

	/// The internal OpenGL vertex buffer that stores the particle colors.
	std::vector<OpenGLBuffer<ColorAT<float>>> _colorsBuffers;

	/// The internal OpenGL vertex buffer that stores the shape of aspherical particles.
	std::vector<OpenGLBuffer<Vector_3<float>>> _shapeBuffers;

	/// The internal OpenGL vertex buffer that stores the orientation of aspherical particles.
	std::vector<OpenGLBuffer<QuaternionT<float>>> _orientationBuffers;

	/// The GL context group under which the GL vertex buffers have been created.
	QPointer<QOpenGLContextGroup> _contextGroup;

	/// The OpenGL texture that is used for billboard rendering of particles.
	OpenGLTexture _billboardTexture;

#ifndef Q_OS_WASM
	/// This array contains the start indices of primitives and is passed to glMultiDrawArrays().
	std::vector<GLint> _primitiveStartIndices;

	/// This array contains the vertex counts of primitives and is passed to glMultiDrawArrays().
	std::vector<GLsizei> _primitiveVertexCounts;
#else
	/// Vertex indices passed to glDrawElements() using GL_TRIANGLES primitives.
	std::vector<GLuint> _trianglePrimitiveVertexIndices;
#endif

	/// The OpenGL shader program that is used to render the particles.
	QOpenGLShaderProgram* _shader = nullptr;

	/// The OpenGL shader program that is used to render the particles in picking mode.
	QOpenGLShaderProgram* _pickingShader = nullptr;

	/// The technique used to render particles. This depends on settings such as rendering quality, shading etc.
	RenderingTechnique _renderingTechnique;

	/// Indicates that an OpenGL geometry shader is being used.
	bool _usingGeometryShader;

	/// A copy of the particle coordinates. This is only required to render translucent
	/// particles in the correct order from back to front.
	std::vector<Point3> _particleCoordinates;
};

}	// End of namespace
