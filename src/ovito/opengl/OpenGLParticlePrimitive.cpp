////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include "OpenGLParticlePrimitive.h"
#include "OpenGLSceneRenderer.h"

/// The maximum resolution of the texture used for billboard rendering of particles. Specified as a power of two.
#define BILLBOARD_TEXTURE_LEVELS 	8

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
OpenGLParticlePrimitive::OpenGLParticlePrimitive(OpenGLSceneRenderer* renderer, ShadingMode shadingMode,
		RenderingQuality renderingQuality, ParticleShape shape, bool translucentParticles) :
	ParticlePrimitive(shadingMode, renderingQuality, shape, translucentParticles),
	_contextGroup(QOpenGLContextGroup::currentContextGroup()),
	_usingGeometryShader(renderer->useGeometryShaders())
{
	OVITO_ASSERT(renderer->glcontext()->shareGroup() == _contextGroup);

	// Determine rendering technique to use.
	if(shadingMode == FlatShading) {
		if(renderer->usePointSprites())
			_renderingTechnique = POINT_SPRITES;
		else
			_renderingTechnique = IMPOSTER_QUADS;
	}
	else {
		if(shape == SphericalShape && renderingQuality < HighQuality) {
			if(renderer->usePointSprites())
				_renderingTechnique = POINT_SPRITES;
			else
				_renderingTechnique = IMPOSTER_QUADS;
		}
		else {
			_renderingTechnique = BOX_GEOMETRY;
		}
	}

	// Load the right OpenGL shaders.
	if(_renderingTechnique == POINT_SPRITES) {
		if(shadingMode == FlatShading) {
			if(shape == SphericalShape || shape == EllipsoidShape) {
				_shader = renderer->loadShaderProgram("particle_pointsprite_spherical_flat",
						":/openglrenderer/glsl/particles/pointsprites/sphere/without_depth.vs",
						":/openglrenderer/glsl/particles/pointsprites/sphere/flat_shading.fs");
				_pickingShader = renderer->loadShaderProgram("particle_pointsprite_spherical_nodepth_picking",
						":/openglrenderer/glsl/particles/pointsprites/sphere/picking/without_depth.vs",
						":/openglrenderer/glsl/particles/pointsprites/sphere/picking/flat_shading.fs");
			}
			else if(shape == SquareCubicShape || shape == BoxShape) {
				_shader = renderer->loadShaderProgram("particle_pointsprite_square_flat",
						":/openglrenderer/glsl/particles/pointsprites/sphere/without_depth.vs",
						":/openglrenderer/glsl/particles/pointsprites/square/flat_shading.fs");
				_pickingShader = renderer->loadShaderProgram("particle_pointsprite_square_flat_picking",
						":/openglrenderer/glsl/particles/pointsprites/sphere/picking/without_depth.vs",
						":/openglrenderer/glsl/particles/pointsprites/square/picking/flat_shading.fs");
			}
		}
		else if(shadingMode == NormalShading) {
			if(shape == SphericalShape) {
				if(renderingQuality == LowQuality) {
					_shader = renderer->loadShaderProgram("particle_pointsprite_spherical_shaded_nodepth",
							":/openglrenderer/glsl/particles/pointsprites/sphere/without_depth.vs",
							":/openglrenderer/glsl/particles/pointsprites/sphere/without_depth.fs");
					_pickingShader = renderer->loadShaderProgram("particle_pointsprite_spherical_nodepth_picking",
							":/openglrenderer/glsl/particles/pointsprites/sphere/picking/without_depth.vs",
							":/openglrenderer/glsl/particles/pointsprites/sphere/picking/flat_shading.fs");
				}
				else if(renderingQuality == MediumQuality) {
					_shader = renderer->loadShaderProgram("particle_pointsprite_spherical_shaded_depth",
							":/openglrenderer/glsl/particles/pointsprites/sphere/with_depth.vs",
							":/openglrenderer/glsl/particles/pointsprites/sphere/with_depth.fs");
					_pickingShader = renderer->loadShaderProgram("particle_pointsprite_spherical_shaded_depth_picking",
							":/openglrenderer/glsl/particles/pointsprites/sphere/picking/with_depth.vs",
							":/openglrenderer/glsl/particles/pointsprites/sphere/picking/with_depth.fs");
				}
			}
		}
	}
	else if(_renderingTechnique == IMPOSTER_QUADS) {
		if(shadingMode == FlatShading) {
			if(shape == SphericalShape || shape == EllipsoidShape) {
				if(_usingGeometryShader) {
					_shader = renderer->loadShaderProgram("particle_geomshader_imposter_spherical_flat",
							":/openglrenderer/glsl/particles/imposter/sphere/without_depth.vs",
							":/openglrenderer/glsl/particles/imposter/sphere/flat_shading.fs",
							":/openglrenderer/glsl/particles/imposter/sphere/without_depth.gs");
					_pickingShader = renderer->loadShaderProgram("particle_geomshader_imposter_spherical_nodepth_picking",
							":/openglrenderer/glsl/particles/imposter/sphere/picking/without_depth.vs",
							":/openglrenderer/glsl/particles/imposter/sphere/picking/flat_shading.fs",
							":/openglrenderer/glsl/particles/imposter/sphere/picking/without_depth.gs");
				}
				else {
					_shader = renderer->loadShaderProgram("particle_imposter_spherical_flat",
							":/openglrenderer/glsl/particles/imposter/sphere/without_depth_tri.vs",
							":/openglrenderer/glsl/particles/imposter/sphere/flat_shading.fs");
					_pickingShader = renderer->loadShaderProgram("particle_imposter_spherical_nodepth_picking",
							":/openglrenderer/glsl/particles/imposter/sphere/picking/without_depth_tri.vs",
							":/openglrenderer/glsl/particles/imposter/sphere/picking/flat_shading.fs");
				}
			}
			else if(shape == SquareCubicShape || shape == BoxShape) {
				if(_usingGeometryShader) {
					_shader = renderer->loadShaderProgram("particle_geomshader_imposter_square_flat",
							":/openglrenderer/glsl/particles/imposter/sphere/without_depth.vs",
							":/openglrenderer/glsl/particles/pointsprites/square/flat_shading.fs",
							":/openglrenderer/glsl/particles/imposter/sphere/without_depth.gs");
					_pickingShader = renderer->loadShaderProgram("particle_geomshader_imposter_square_flat_picking",
							":/openglrenderer/glsl/particles/imposter/sphere/picking/without_depth.vs",
							":/openglrenderer/glsl/particles/pointsprites/square/picking/flat_shading.fs",
							":/openglrenderer/glsl/particles/imposter/sphere/picking/without_depth.gs");
				}
				else {
					_shader = renderer->loadShaderProgram("particle_imposter_square_flat",
							":/openglrenderer/glsl/particles/imposter/sphere/without_depth_tri.vs",
							":/openglrenderer/glsl/particles/pointsprites/square/flat_shading.fs");
					_pickingShader = renderer->loadShaderProgram("particle_imposter_square_flat_picking",
							":/openglrenderer/glsl/particles/imposter/sphere/picking/without_depth_tri.vs",
							":/openglrenderer/glsl/particles/pointsprites/square/picking/flat_shading.fs");
				}
			}
		}
		else if(shadingMode == NormalShading) {
			if(shape == SphericalShape) {
				if(renderingQuality == LowQuality) {
					if(_usingGeometryShader) {
						_shader = renderer->loadShaderProgram("particle_geomshader_imposter_spherical_shaded_nodepth",
								":/openglrenderer/glsl/particles/imposter/sphere/without_depth.vs",
								":/openglrenderer/glsl/particles/imposter/sphere/without_depth.fs",
								":/openglrenderer/glsl/particles/imposter/sphere/without_depth.gs");
						_pickingShader = renderer->loadShaderProgram("particle_geomshader_imposter_spherical_nodepth_picking",
								":/openglrenderer/glsl/particles/imposter/sphere/picking/without_depth.vs",
								":/openglrenderer/glsl/particles/imposter/sphere/picking/flat_shading.fs",
								":/openglrenderer/glsl/particles/imposter/sphere/picking/without_depth.gs");
					}
					else {
						_shader = renderer->loadShaderProgram("particle_imposter_spherical_shaded_nodepth",
								":/openglrenderer/glsl/particles/imposter/sphere/without_depth_tri.vs",
								":/openglrenderer/glsl/particles/imposter/sphere/without_depth.fs");
						_pickingShader = renderer->loadShaderProgram("particle_imposter_spherical_nodepth_picking",
								":/openglrenderer/glsl/particles/imposter/sphere/picking/without_depth_tri.vs",
								":/openglrenderer/glsl/particles/imposter/sphere/picking/flat_shading.fs");
					}
				}
				else if(renderingQuality == MediumQuality) {
					if(_usingGeometryShader) {
						_shader = renderer->loadShaderProgram("particle_geomshader_imposter_spherical_shaded_depth",
								":/openglrenderer/glsl/particles/imposter/sphere/with_depth.vs",
								":/openglrenderer/glsl/particles/imposter/sphere/with_depth.fs",
								":/openglrenderer/glsl/particles/imposter/sphere/with_depth.gs");
						_pickingShader = renderer->loadShaderProgram("particle_geomshader_imposter_spherical_shaded_depth_picking",
								":/openglrenderer/glsl/particles/imposter/sphere/picking/with_depth.vs",
								":/openglrenderer/glsl/particles/imposter/sphere/picking/with_depth.fs",
								":/openglrenderer/glsl/particles/imposter/sphere/picking/with_depth.gs");
					}
					else {
						_shader = renderer->loadShaderProgram("particle_imposter_spherical_shaded_depth",
								":/openglrenderer/glsl/particles/imposter/sphere/with_depth_tri.vs",
								":/openglrenderer/glsl/particles/imposter/sphere/with_depth.fs");
						_pickingShader = renderer->loadShaderProgram("particle_imposter_spherical_shaded_depth_picking",
								":/openglrenderer/glsl/particles/imposter/sphere/picking/with_depth_tri.vs",
								":/openglrenderer/glsl/particles/imposter/sphere/picking/with_depth.fs");
					}
				}
			}
		}
	}
	else if(_renderingTechnique == BOX_GEOMETRY) {
		if(shadingMode == NormalShading) {
			if(_usingGeometryShader) {
				if(shape == SphericalShape && renderingQuality == HighQuality) {
					_shader = renderer->loadShaderProgram("particle_geomshader_sphere",
							":/openglrenderer/glsl/particles/geometry/sphere/sphere.vs",
							":/openglrenderer/glsl/particles/geometry/sphere/sphere.fs",
							":/openglrenderer/glsl/particles/geometry/sphere/sphere.gs");
					_pickingShader = renderer->loadShaderProgram("particle_geomshader_sphere_picking",
							":/openglrenderer/glsl/particles/geometry/sphere/picking/sphere.vs",
							":/openglrenderer/glsl/particles/geometry/sphere/picking/sphere.fs",
							":/openglrenderer/glsl/particles/geometry/sphere/picking/sphere.gs");
				}
				else if(shape == SquareCubicShape) {
					_shader = renderer->loadShaderProgram("particle_geomshader_cube",
							":/openglrenderer/glsl/particles/geometry/cube/cube.vs",
							":/openglrenderer/glsl/particles/geometry/cube/cube.fs",
							":/openglrenderer/glsl/particles/geometry/cube/cube.gs");
					_pickingShader = renderer->loadShaderProgram("particle_geomshader_cube_picking",
							":/openglrenderer/glsl/particles/geometry/cube/picking/cube.vs",
							":/openglrenderer/glsl/particles/geometry/cube/picking/cube.fs",
							":/openglrenderer/glsl/particles/geometry/cube/picking/cube.gs");
				}
				else if(shape == BoxShape) {
					_shader = renderer->loadShaderProgram("particle_geomshader_box",
							":/openglrenderer/glsl/particles/geometry/box/box.vs",
							":/openglrenderer/glsl/particles/geometry/cube/cube.fs",
							":/openglrenderer/glsl/particles/geometry/box/box.gs");
					_pickingShader = renderer->loadShaderProgram("particle_geomshader_box_picking",
							":/openglrenderer/glsl/particles/geometry/box/picking/box.vs",
							":/openglrenderer/glsl/particles/geometry/cube/picking/cube.fs",
							":/openglrenderer/glsl/particles/geometry/box/picking/box.gs");
				}
				else if(shape == EllipsoidShape) {
					_shader = renderer->loadShaderProgram("particle_geomshader_ellipsoid",
							":/openglrenderer/glsl/particles/geometry/ellipsoid/ellipsoid.vs",
							":/openglrenderer/glsl/particles/geometry/ellipsoid/ellipsoid.fs",
							":/openglrenderer/glsl/particles/geometry/ellipsoid/ellipsoid.gs");
					_pickingShader = renderer->loadShaderProgram("particle_geomshader_ellipsoid_picking",
							":/openglrenderer/glsl/particles/geometry/ellipsoid/picking/ellipsoid.vs",
							":/openglrenderer/glsl/particles/geometry/ellipsoid/picking/ellipsoid.fs",
							":/openglrenderer/glsl/particles/geometry/ellipsoid/picking/ellipsoid.gs");
				}
			}
			else {
				if(shape == SphericalShape && renderingQuality == HighQuality) {
					_shader = renderer->loadShaderProgram("particle_tristrip_sphere",
							":/openglrenderer/glsl/particles/geometry/sphere/sphere_tristrip.vs",
							":/openglrenderer/glsl/particles/geometry/sphere/sphere.fs");
					_pickingShader = renderer->loadShaderProgram("particle_tristrip_sphere_picking",
							":/openglrenderer/glsl/particles/geometry/sphere/picking/sphere_tristrip.vs",
							":/openglrenderer/glsl/particles/geometry/sphere/picking/sphere.fs");
				}
				else if(shape == SquareCubicShape) {
					_shader = renderer->loadShaderProgram("particle_tristrip_cube",
							":/openglrenderer/glsl/particles/geometry/cube/cube_tristrip.vs",
							":/openglrenderer/glsl/particles/geometry/cube/cube.fs");
					_pickingShader = renderer->loadShaderProgram("particle_tristrip_cube_picking",
							":/openglrenderer/glsl/particles/geometry/cube/picking/cube_tristrip.vs",
							":/openglrenderer/glsl/particles/geometry/cube/picking/cube.fs");
				}
				else if(shape == BoxShape) {
					_shader = renderer->loadShaderProgram("particle_tristrip_box",
							":/openglrenderer/glsl/particles/geometry/box/box_tristrip.vs",
							":/openglrenderer/glsl/particles/geometry/cube/cube.fs");
					_pickingShader = renderer->loadShaderProgram("particle_tristrip_box_picking",
							":/openglrenderer/glsl/particles/geometry/box/picking/box_tristrip.vs",
							":/openglrenderer/glsl/particles/geometry/cube/picking/cube.fs");
				}
				else if(shape == EllipsoidShape) {
					_shader = renderer->loadShaderProgram("particle_tristrip_ellipsoid",
							":/openglrenderer/glsl/particles/geometry/ellipsoid/ellipsoid_tristrip.vs",
							":/openglrenderer/glsl/particles/geometry/ellipsoid/ellipsoid.fs");
					_pickingShader = renderer->loadShaderProgram("particle_tristrip_ellipsoid_picking",
							":/openglrenderer/glsl/particles/geometry/ellipsoid/picking/ellipsoid_tristrip.vs",
							":/openglrenderer/glsl/particles/geometry/ellipsoid/picking/ellipsoid.fs");
				}
			}
		}
	}
	OVITO_ASSERT(_shader != nullptr);
	OVITO_ASSERT(_pickingShader != nullptr);

	// Prepare texture that is required for imposter rendering of spherical particles.
	if(shape == SphericalShape && shadingMode == NormalShading && (_renderingTechnique == POINT_SPRITES || _renderingTechnique == IMPOSTER_QUADS))
		initializeBillboardTexture(renderer);
}

/******************************************************************************
* Allocates a particle buffer with the given number of particles.
******************************************************************************/
void OpenGLParticlePrimitive::setSize(int particleCount)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);

	_particleCount = particleCount;

	// Determine the required number of vertices that need to be sent to the graphics card per particle.
	int verticesPerParticle;
	if(_renderingTechnique == POINT_SPRITES) {
		verticesPerParticle = 1;
	}
	else if(_renderingTechnique == IMPOSTER_QUADS) {
		if(_usingGeometryShader)
			verticesPerParticle = 1;
		else
			verticesPerParticle = 6;
	}
	else if(_renderingTechnique == BOX_GEOMETRY) {
		if(_usingGeometryShader)
			verticesPerParticle = 1;
		else
			verticesPerParticle = 14;
	}
	else OVITO_ASSERT(false);

	// Determine the VBO chunk size.
	int bytesPerVertex = sizeof(ColorAT<float>);
	_chunkSize = std::min(_maxVBOSize / verticesPerParticle / bytesPerVertex, particleCount);

	// Cannot use chunked VBOs when rendering semi-transparent particles,
	// because they will be rendered in arbitrary order.
	if(translucentParticles())
		_chunkSize = particleCount;

	// Allocate VBOs.
	int numChunks = particleCount ? ((particleCount + _chunkSize - 1) / _chunkSize) : 0;
	_positionsBuffers.resize(numChunks);
	_radiiBuffers.resize(numChunks);
	//Modif
	_transparenciesBuffers.resize(numChunks);
	_colorsBuffers.resize(numChunks);
	if(particleShape() == BoxShape || particleShape() == EllipsoidShape) {
		_shapeBuffers.resize(numChunks);
		_orientationBuffers.resize(numChunks);
	}

	for(int i = 0; i < numChunks; i++) {
		int size = std::min(_chunkSize, particleCount - i * _chunkSize);
		_positionsBuffers[i].create(QOpenGLBuffer::StaticDraw, size, verticesPerParticle);
		_radiiBuffers[i].create(QOpenGLBuffer::StaticDraw, size, verticesPerParticle);
		//Modif
		_transparenciesBuffers[i].create(QOpenGLBuffer::StaticDraw, size, verticesPerParticle);
		_colorsBuffers[i].create(QOpenGLBuffer::StaticDraw, size, verticesPerParticle);
		if(particleShape() == BoxShape || particleShape() == EllipsoidShape) {
			_shapeBuffers[i].create(QOpenGLBuffer::StaticDraw, size, verticesPerParticle);
			_shapeBuffers[i].fillConstant(Vector_3<float>::Zero());
			_orientationBuffers[i].create(QOpenGLBuffer::StaticDraw, size, verticesPerParticle);
			_orientationBuffers[i].fillConstant(QuaternionT<float>(0,0,0,1));
		}
	}
}

/******************************************************************************
* Sets the coordinates of the particles.
******************************************************************************/
void OpenGLParticlePrimitive::setParticlePositions(const Point3* coordinates)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);

	// Make a copy of the particle coordinates. They will be needed when rendering
	// semi-transparent particles in the correct order from back to front.
	if(translucentParticles()) {
		_particleCoordinates.resize(particleCount());
		std::copy(coordinates, coordinates + particleCount(), _particleCoordinates.begin());
	}

	for(auto& buffer : _positionsBuffers) {
		buffer.fill(coordinates);
		coordinates += buffer.elementCount();
	}
}

/******************************************************************************
* Sets the radii of the particles.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleRadii(const FloatType* radii)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	for(auto& buffer : _radiiBuffers) {
		buffer.fill(radii);
		radii += buffer.elementCount();
	}
}

/******************************************************************************
* Sets the radius of all particles to the given value.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleRadius(FloatType radius)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	for(auto& buffer : _radiiBuffers)
		buffer.fillConstant(radius);
}

//Begin of modification
/******************************************************************************
* Sets the transparencies of the particles.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleTransparencies(const FloatType* transparencies)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	for(auto& buffer : _transparenciesBuffers) {
		buffer.fill(transparencies);
		transparencies += buffer.elementCount();
	}
}

/******************************************************************************
* Sets the transparency of all particles to the given value.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleTransparency(FloatType transparency)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	for(auto& buff : _transparenciesBuffers){
		buff.fillConstant(transparency);
    }


}

//End of modification

/******************************************************************************
* Sets the colors of the particles.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleColors(const ColorA* colors)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	for(auto& buffer : _colorsBuffers) {
		buffer.fill(colors);
		colors += buffer.elementCount();
	}
}

/******************************************************************************
* Sets the colors of the particles.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleColors(const Color* colors)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	// Need to convert array from Color to ColorA.
	for(auto& buffer : _colorsBuffers) {
		ColorAT<float>* dest = buffer.map();
		const Color* end_colors = colors + buffer.elementCount();
		for(; colors != end_colors; ++colors) {
			for(int i = 0; i < buffer.verticesPerElement(); i++, ++dest) {
				dest->r() = (float)colors->r();
				dest->g() = (float)colors->g();
				dest->b() = (float)colors->b();
				dest->a() = 1;
			}
		}
		buffer.unmap();
	}
}

/******************************************************************************
* Sets the color of all particles to the given value.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleColor(const ColorA color)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	for(auto& buffer : _colorsBuffers)
		buffer.fillConstant(color);
}

/******************************************************************************
* Sets the aspherical shapes of the particles.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleShapes(const Vector3* shapes)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	if(!_shapeBuffers.empty()) {
		for(auto& buffer : _shapeBuffers) {
			buffer.fill(shapes);
			shapes += buffer.elementCount();
		}
	}
}

/******************************************************************************
* Sets the orientations of aspherical particles.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleOrientations(const Quaternion* orientations)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	if(!_orientationBuffers.empty()) {
		for(auto& buffer : _orientationBuffers) {
			buffer.fill(orientations);
			orientations += buffer.elementCount();
		}
	}
}

/******************************************************************************
* Resets the aspherical shape of the particles.
******************************************************************************/
void OpenGLParticlePrimitive::clearParticleShapes()
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	for(auto& buffer : _shapeBuffers) {
		buffer.fillConstant(Vector_3<float>::Zero());
	}
}

/******************************************************************************
* Resets the orientation of particles.
******************************************************************************/
void OpenGLParticlePrimitive::clearParticleOrientations()
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	for(auto& buffer : _orientationBuffers) {
		buffer.fillConstant(QuaternionT<float>(0,0,0,1));
	}
}

/******************************************************************************
* Returns true if the geometry buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool OpenGLParticlePrimitive::isValid(SceneRenderer* renderer)
{
	OpenGLSceneRenderer* vpRenderer = dynamic_object_cast<OpenGLSceneRenderer>(renderer);
	if(!vpRenderer) return false;
	return (_particleCount >= 0) && (_contextGroup == vpRenderer->glcontext()->shareGroup());
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void OpenGLParticlePrimitive::render(SceneRenderer* renderer)
{
	OVITO_ASSERT(_contextGroup == QOpenGLContextGroup::currentContextGroup());

	OpenGLSceneRenderer* vpRenderer = dynamic_object_cast<OpenGLSceneRenderer>(renderer);

	if(particleCount() <= 0 || !vpRenderer)
		return;
    OVITO_REPORT_OPENGL_ERRORS(vpRenderer);

	// If object is translucent, don't render it during the first rendering pass.
	// Queue primitive so that it gets rendered during the second pass.
	if(!renderer->isPicking() && translucentParticles() && vpRenderer->translucentPass() == false) {
		vpRenderer->registerTranslucentPrimitive(shared_from_this());
		return;
	}

	vpRenderer->rebindVAO();

	if(_renderingTechnique == POINT_SPRITES)
		renderPointSprites(vpRenderer);
	else if(_renderingTechnique == IMPOSTER_QUADS)
		renderImposters(vpRenderer);
	else if(_renderingTechnique == BOX_GEOMETRY)
		renderBoxes(vpRenderer);
}

/******************************************************************************
* Renders the particles using OpenGL point sprites.
******************************************************************************/
void OpenGLParticlePrimitive::renderPointSprites(OpenGLSceneRenderer* renderer)
{
#ifndef Q_OS_WASM
	OVITO_ASSERT(!_positionsBuffers.empty());
	OVITO_ASSERT(_positionsBuffers.front().verticesPerElement() == 1);

	// Let the vertex shader compute the point size.
	OVITO_CHECK_OPENGL(renderer, renderer->glEnable(GL_VERTEX_PROGRAM_POINT_SIZE));

	// Enable point sprites when using the compatibility OpenGL profile.
	// In the core profile, they are already enabled by default.
	if(renderer->glformat().profile() != QSurfaceFormat::CoreProfile) {
		OVITO_CHECK_OPENGL(renderer, renderer->glEnable(GL_POINT_SPRITE));

		// Specify point sprite texture coordinate replacement mode.
		renderer->glTexEnvf(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
	}

	if(particleShape() == SphericalShape && shadingMode() == NormalShading && !renderer->isPicking())
		activateBillboardTexture(renderer);

	// Pick the right OpenGL shader program.
	QOpenGLShaderProgram* shader = renderer->isPicking() ? _pickingShader : _shader;
	if(!shader->bind())
		renderer->throwException(QStringLiteral("Failed to bind OpenGL shader program."));

	// This is how our point sprite's size will be modified based on the distance from the viewer.
	GLint viewportCoords[4];
	renderer->glGetIntegerv(GL_VIEWPORT, viewportCoords);
	float param = renderer->projParams().projectionMatrix(1,1) * viewportCoords[3];

	if(renderer->isCoreProfile() == false) {
		// This is a fallback if GL_VERTEX_PROGRAM_POINT_SIZE is not supported.
		std::array<float,3> distanceAttenuation;
		if(renderer->projParams().isPerspective)
			distanceAttenuation = std::array<float,3>{{ 0.0f, 0.0f, 1.0f / (param * param) }};
		else
			distanceAttenuation = std::array<float,3>{{ 1.0f / param, 0.0f, 0.0f }};
		OVITO_CHECK_OPENGL(renderer, renderer->glPointSize(1));
		OVITO_CHECK_OPENGL(renderer, renderer->glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, distanceAttenuation.data()));
	}

	// Account for possible scaling in the model-view TM.
	float radius_scalingfactor = (float)pow(renderer->modelViewTM().determinant(), FloatType(1.0/3.0));
	shader->setUniformValue("radius_scalingfactor", radius_scalingfactor);
	param *= radius_scalingfactor;

	shader->setUniformValue("basePointSize", param);
	shader->setUniformValue("projection_matrix", (QMatrix4x4)renderer->projParams().projectionMatrix);
	shader->setUniformValue("modelview_matrix", (QMatrix4x4)renderer->modelViewTM());

	//MODIF
//	if(!renderer->isPicking() && translucentParticles()) {
/*		//renderer->glEnable(GL_CULL_FACE);
		renderer->glEnable(GL_BLEND); //initialisation de la transparence
		renderer->glBlendEquation(GL_FUNC_ADD);
		//renderer->glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		//modif
		//renderer->glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		renderer->glBlendFunc(GL_SRC_ALPHA, GL_CONSTANT_ALPHA);// alpha = 1 - color_alpha
		//renderer->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		renderer->glEnable(GL_DEPTH_TEST);
		//renderer->glDepthFunc(GL_LESS);
		renderer->glDepthFunc(GL_NEVER);
		renderer->glDisable(GL_CULL_FACE);
		renderer->glDepthFunc(GL_LESS);

		renderer->glEnable(GL_CULL_FACE);
		renderer->glCullFace(GL_FRONT);
		renderer->glDepthFunc(GL_ALWAYS);
	//}
	//
	//
*/

	renderer->glEnable(GL_BLEND);
	renderer->glBlendEquation(GL_FUNC_SUBTRACT);
	renderer->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLint pickingBaseID = 0;
	if(renderer->isPicking()) {
		pickingBaseID = renderer->registerSubObjectIDs(particleCount());
		renderer->activateVertexIDs(shader, _chunkSize);
	}

	for(size_t chunkIndex = 0; chunkIndex < _positionsBuffers.size(); chunkIndex++) {
		int chunkSize = _positionsBuffers[chunkIndex].elementCount();
		_positionsBuffers[chunkIndex].bindPositions(renderer, shader);
		_radiiBuffers[chunkIndex].bind(renderer, shader, "particle_radius", GL_FLOAT, 0, 1);
		//Modif
		_transparenciesBuffers[chunkIndex].bind(renderer, shader, "particle_transparency", GL_FLOAT, 0, 1);
		if(!renderer->isPicking())
			_colorsBuffers[chunkIndex].bindColors(renderer, shader, 4);
		else {
			shader->setUniformValue("pickingBaseID", pickingBaseID);
			pickingBaseID += _chunkSize;
		}

		//Modif

		// Are we rendering translucent particles? If yes, render them in back to front order to avoid visual artifacts at overlapping particles.
//		if(!renderer->isPicking() && translucentParticles() && !_particleCoordinates.empty()) {
			// Create temporary OpenGL index buffer which can be used with glDrawElements to draw particles in desired order.
			OpenGLBuffer<GLuint> primitiveIndices(QOpenGLBuffer::IndexBuffer);
			primitiveIndices.create(QOpenGLBuffer::StaticDraw, particleCount());
			primitiveIndices.fill(determineRenderingOrder(renderer).data());
			primitiveIndices.oglBuffer().bind();
			OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_POINTS, particleCount(), GL_UNSIGNED_INT, nullptr));
			primitiveIndices.oglBuffer().release();
/*		}
		else {
			// Fully opaque particles can be rendered in unsorted storage order.
			OVITO_CHECK_OPENGL(renderer, renderer->glDrawArrays(GL_POINTS, 0, chunkSize));
		}
*/
		_positionsBuffers[chunkIndex].detachPositions(renderer, shader);
		_radiiBuffers[chunkIndex].detach(renderer, shader, "particle_radius");
		_transparenciesBuffers[chunkIndex].detach(renderer, shader, "particle_transparency");
		if(!renderer->isPicking())
			_colorsBuffers[chunkIndex].detachColors(renderer, shader);
	}
	if(renderer->isPicking())
		renderer->deactivateVertexIDs(shader);

	shader->release();

	OVITO_CHECK_OPENGL(renderer, renderer->glDisable(GL_VERTEX_PROGRAM_POINT_SIZE));
	//MODIF
//	if(!renderer->isPicking() && translucentParticles()) {
		//renderer->glDisable(GL_CULL_FACE);
		renderer->glDisable(GL_BLEND);
		//renderer->glDepthFunc(GL_LEQUAL);
//	}

	// Disable point sprites again.
	if(renderer->glformat().profile() != QSurfaceFormat::CoreProfile)
		OVITO_CHECK_OPENGL(renderer, renderer->glDisable(GL_POINT_SPRITE));

	if(particleShape() == SphericalShape && shadingMode() == NormalShading && !renderer->isPicking())
		deactivateBillboardTexture(renderer);
#endif		
}

/******************************************************************************
* Renders a cube for each particle using triangle strips.
******************************************************************************/
void OpenGLParticlePrimitive::renderBoxes(OpenGLSceneRenderer* renderer)
{
	int verticesPerElement = _positionsBuffers.front().verticesPerElement();
	OVITO_ASSERT(!_usingGeometryShader || verticesPerElement == 1);
	OVITO_ASSERT(_usingGeometryShader || verticesPerElement == 14);

	// Pick the right OpenGL shader program.
	QOpenGLShaderProgram* shader = renderer->isPicking() ? _pickingShader : _shader;
	if(!shader->bind())
		renderer->throwException(QStringLiteral("Failed to bind OpenGL shader program."));

	// Need to render only the front facing sides of the cubes.
	renderer->glCullFace(GL_BACK);
	renderer->glEnable(GL_CULL_FACE);

	if(!_usingGeometryShader) {
		// This is to draw the cube with a single triangle strip.
		// The cube vertices:
		static const QVector3D cubeVerts[14] = {
			{ 1,  1,  1},
			{ 1, -1,  1},
			{ 1,  1, -1},
			{ 1, -1, -1},
			{-1, -1, -1},
			{ 1, -1,  1},
			{-1, -1,  1},
			{ 1,  1,  1},
			{-1,  1,  1},
			{ 1,  1, -1},
			{-1,  1, -1},
			{-1, -1, -1},
			{-1,  1,  1},
			{-1, -1,  1},
		};
		OVITO_CHECK_OPENGL(renderer, shader->setUniformValueArray("cubeVerts", cubeVerts, 14));
	}

	if(particleShape() != SphericalShape && !renderer->isPicking()) {
		Matrix3 normal_matrix = renderer->modelViewTM().linear().inverse().transposed();
		normal_matrix.column(0).normalize();
		normal_matrix.column(1).normalize();
		normal_matrix.column(2).normalize();
		shader->setUniformValue("normal_matrix", (QMatrix3x3)normal_matrix);
		if(!_usingGeometryShader) {
			// The normal vectors for the cube triangle strip.
			static const QVector3D normals[14] = {
				{ 1,  0,  0},
				{ 1,  0,  0},
				{ 1,  0,  0},
				{ 1,  0,  0},
				{ 0,  0, -1},
				{ 0, -1,  0},
				{ 0, -1,  0},
				{ 0,  0,  1},
				{ 0,  0,  1},
				{ 0,  1,  0},
				{ 0,  1,  0},
				{ 0,  0, -1},
				{-1,  0,  0},
				{-1,  0,  0}
			};
			OVITO_CHECK_OPENGL(renderer, shader->setUniformValueArray("normals", normals, 14));
		}
	}

	shader->setUniformValue("projection_matrix", (QMatrix4x4)renderer->projParams().projectionMatrix);
	shader->setUniformValue("inverse_projection_matrix", (QMatrix4x4)renderer->projParams().inverseProjectionMatrix);
	shader->setUniformValue("modelview_matrix", (QMatrix4x4)renderer->modelViewTM());
	shader->setUniformValue("modelviewprojection_matrix", (QMatrix4x4)(renderer->projParams().projectionMatrix * renderer->modelViewTM()));
	shader->setUniformValue("is_perspective", renderer->projParams().isPerspective);
	shader->setUniformValue("radius_scalingfactor", (float)pow(renderer->modelViewTM().determinant(), FloatType(1.0/3.0)));

	GLint viewportCoords[4];
	renderer->glGetIntegerv(GL_VIEWPORT, viewportCoords);
	shader->setUniformValue("viewport_origin", (float)viewportCoords[0], (float)viewportCoords[1]);
	shader->setUniformValue("inverse_viewport_size", 2.0f / (float)viewportCoords[2], 2.0f / (float)viewportCoords[3]);

	//MODIF
	if(!renderer->isPicking() && translucentParticles()) {
		renderer->glEnable(GL_BLEND);
		renderer->glBlendEquation(GL_FUNC_ADD);
		renderer->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_COLOR, GL_ONE);
	}

	GLint pickingBaseID = 0;
	if(renderer->isPicking()) {
		pickingBaseID = renderer->registerSubObjectIDs(particleCount());
	}

	for(size_t chunkIndex = 0; chunkIndex < _positionsBuffers.size(); chunkIndex++) {
		int chunkSize = _positionsBuffers[chunkIndex].elementCount();

		_positionsBuffers[chunkIndex].bindPositions(renderer, shader);
		if(particleShape() == BoxShape || particleShape() == EllipsoidShape) {
			_shapeBuffers[chunkIndex].bind(renderer, shader, "shape", GL_FLOAT, 0, 3);
			_orientationBuffers[chunkIndex].bind(renderer, shader, "orientation", GL_FLOAT, 0, 4);
		}
		_radiiBuffers[chunkIndex].bind(renderer, shader, "particle_radius", GL_FLOAT, 0, 1);
		//Modif
		_transparenciesBuffers[chunkIndex].bind(renderer, shader, "particle_transparency", GL_FLOAT, 0, 1);
		if(!renderer->isPicking()) {
			_colorsBuffers[chunkIndex].bindColors(renderer, shader, 4);
		}
		else {
			shader->setUniformValue("pickingBaseID", pickingBaseID);
			pickingBaseID += _chunkSize;
			renderer->activateVertexIDs(shader, _positionsBuffers[chunkIndex].elementCount() * verticesPerElement);
		}

		if(_usingGeometryShader) {
			// Are we rendering translucent particles? If yes, render them in back to front order to avoid visual artifacts at overlapping particles.
			if(!renderer->isPicking() && translucentParticles() && !_particleCoordinates.empty()) {
				// Create OpenGL index buffer which can be used with glDrawElements.
				OpenGLBuffer<GLuint> primitiveIndices(QOpenGLBuffer::IndexBuffer);
				primitiveIndices.create(QOpenGLBuffer::StaticDraw, particleCount());
				primitiveIndices.fill(determineRenderingOrder(renderer).data());
				primitiveIndices.oglBuffer().bind();
				OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_POINTS, particleCount(), GL_UNSIGNED_INT, nullptr));
				primitiveIndices.oglBuffer().release();
			}
			else {
				// Fully opaque particles can be rendered in unsorted storage order.
				OVITO_CHECK_OPENGL(renderer, renderer->glDrawArrays(GL_POINTS, 0, chunkSize));
			}
		}
		else {
			renderer->activateVertexIDs(shader, chunkSize * _positionsBuffers[chunkIndex].verticesPerElement(), renderer->isPicking());
#ifndef Q_OS_WASM
			// Prepare arrays required for glMultiDrawArrays().

			// Are we rendering translucent particles? If yes, render them in back to front order to avoid visual artifacts at overlapping particles.
			if(!renderer->isPicking() && translucentParticles() && !_particleCoordinates.empty()) {
				auto indices = determineRenderingOrder(renderer);
				_primitiveStartIndices.clear();
				_primitiveStartIndices.resize(particleCount());
				std::transform(indices.begin(), indices.end(), _primitiveStartIndices.begin(), [verticesPerElement](GLuint i) { return i*verticesPerElement; });
				if(_primitiveVertexCounts.size() != particleCount()) {
					_primitiveVertexCounts.clear();
					_primitiveVertexCounts.resize(particleCount());
					std::fill(_primitiveVertexCounts.begin(), _primitiveVertexCounts.end(), verticesPerElement);
				}
			}
			else if(_primitiveStartIndices.size() < chunkSize) {
				_primitiveStartIndices.clear();
				_primitiveStartIndices.resize(chunkSize);
				_primitiveVertexCounts.clear();
				_primitiveVertexCounts.resize(chunkSize);
				GLint index = 0;
				for(GLint& s : _primitiveStartIndices) {
					s = index;
					index += verticesPerElement;
				}
				std::fill(_primitiveVertexCounts.begin(), _primitiveVertexCounts.end(), verticesPerElement);
			}

			OVITO_CHECK_OPENGL(renderer, renderer->glMultiDrawArrays(GL_TRIANGLE_STRIP,
					_primitiveStartIndices.data(),
					_primitiveVertexCounts.data(),
					chunkSize));

#else
			// glMultiDrawArrays() is not available in OpenGL ES. Use glDrawElements() instead.
			int indicesPerElement = 3 * 12; // (3 vertices per triangle) * (12 triangles per cube).
			if(!renderer->isPicking() && translucentParticles() && !_particleCoordinates.empty()) {
				auto indices = determineRenderingOrder(renderer);
				_trianglePrimitiveVertexIndices.clear();
				_trianglePrimitiveVertexIndices.resize(particleCount() * indicesPerElement);
				auto pvi = _trianglePrimitiveVertexIndices.begin();
				for(const auto& index : indices) {
					int baseIndex = index * 14;
					for(int u = 2; u < 14; u++) {
						if((u & 1) == 0) {
							*pvi++ = baseIndex + u - 2;
							*pvi++ = baseIndex + u - 1;
							*pvi++ = baseIndex + u - 0;
						}
						else {
							*pvi++ = baseIndex + u - 0;
							*pvi++ = baseIndex + u - 1;
							*pvi++ = baseIndex + u - 2;
						}
					}
				}
				OVITO_ASSERT(pvi == _trianglePrimitiveVertexIndices.end());
			}
			else if(_trianglePrimitiveVertexIndices.size() < chunkSize * indicesPerElement) {
				_trianglePrimitiveVertexIndices.clear();
				_trianglePrimitiveVertexIndices.resize(chunkSize * indicesPerElement);
				auto pvi = _trianglePrimitiveVertexIndices.begin();
				for(GLuint index = 0, baseIndex = 0; index < chunkSize; index++, baseIndex += 14) {
					for(int u = 2; u < 14; u++) {
						if((u & 1) == 0) {
							*pvi++ = baseIndex + u - 2;
							*pvi++ = baseIndex + u - 1;
							*pvi++ = baseIndex + u - 0;
						}
						else {
							*pvi++ = baseIndex + u - 0;
							*pvi++ = baseIndex + u - 1;
							*pvi++ = baseIndex + u - 2;
						}
					}
				}
				OVITO_ASSERT(pvi == _trianglePrimitiveVertexIndices.end());
			}
			OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_TRIANGLES, chunkSize * indicesPerElement, GL_UNSIGNED_INT, _trianglePrimitiveVertexIndices.data()));
#endif
			renderer->deactivateVertexIDs(shader, renderer->isPicking());
		}

		_positionsBuffers[chunkIndex].detachPositions(renderer, shader);
		if(!renderer->isPicking())
			_colorsBuffers[chunkIndex].detachColors(renderer, shader);
		if(particleShape() == BoxShape || particleShape() == EllipsoidShape) {
			_shapeBuffers[chunkIndex].detach(renderer, shader, "shape");
			_orientationBuffers[chunkIndex].detach(renderer, shader, "orientation");
		}
		_radiiBuffers[chunkIndex].detach(renderer, shader, "particle_radius");
		_transparenciesBuffers[chunkIndex].detach(renderer, shader, "particle_transparency");
	}

	//Modif
	//if(!renderer->isPicking())
		renderer->glDisable(GL_BLEND);
	//else
	//	renderer->deactivateVertexIDs(shader);

	shader->release();
	renderer->glDisable(GL_CULL_FACE);
}

/******************************************************************************
* Renders particles using quads.
******************************************************************************/
void OpenGLParticlePrimitive::renderImposters(OpenGLSceneRenderer* renderer)
{
	int verticesPerElement = _positionsBuffers.front().verticesPerElement();

	// Pick the right OpenGL shader program.
	QOpenGLShaderProgram* shader = renderer->isPicking() ? _pickingShader : _shader;
	if(!shader->bind())
		renderer->throwException(QStringLiteral("Failed to bind OpenGL shader program."));

	if(particleShape() == SphericalShape && shadingMode() == NormalShading && !renderer->isPicking())
		activateBillboardTexture(renderer);

	// Need to render only the front facing sides.
/*	//Modif
	renderer->glDisable(GL_CULL_FACE);
	renderer->glDepthFunc(GL_LESS);

	renderer->glCullFace(GL_BACK);
	renderer->glEnable(GL_CULL_FACE);

	//Modif
	renderer->glCullFace(GL_FRONT);
	renderer->glDepthFunc(GL_ALWAYS);

	//Modif
	renderer->glEnable(GL_CULL_FACE);
	renderer->glCullFace(GL_FRONT);
	renderer->glDepthFunc(GL_LEQUAL);

	//Modif
	renderer->glEnable(GL_CULL_FACE);
	renderer->glCullFace(GL_BACK);
	renderer->glDepthFunc(GL_ALWAYS);
*/
	if(!_usingGeometryShader) {
		// The texture coordinates of a quad made of two triangles.
		static const QVector2D texcoords[6] = {{0,1},{1,1},{1,0},{0,1},{1,0},{0,0}};
		OVITO_CHECK_OPENGL(renderer, shader->setUniformValueArray("imposter_texcoords", texcoords, 6));

		// The coordinate offsets of the six vertices of a quad made of two triangles.
		static const QVector4D voffsets[6] = {{-1,-1,0,0},{1,-1,0,0},{1,1,0,0},{-1,-1,0,0},{1,1,0,0},{-1,1,0,0}};
		OVITO_CHECK_OPENGL(renderer, shader->setUniformValueArray("imposter_voffsets", voffsets, 6));
	}

	shader->setUniformValue("projection_matrix", (QMatrix4x4)renderer->projParams().projectionMatrix);
	shader->setUniformValue("modelview_matrix", (QMatrix4x4)renderer->modelViewTM());
	shader->setUniformValue("modelviewprojection_matrix", (QMatrix4x4)(renderer->projParams().projectionMatrix * renderer->modelViewTM()));

	// Account for possible scaling in the model-view TM.
	shader->setUniformValue("radius_scalingfactor", (float)pow(renderer->modelViewTM().determinant(), FloatType(1.0/3.0)));

	//Modif
//	if(!renderer->isPicking() && translucentParticles()) {
		renderer->glEnable(GL_BLEND);
		renderer->glBlendEquation(GL_FUNC_ADD);
		renderer->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
	//}

	GLint pickingBaseID = 0;
	if(renderer->isPicking()) {
		pickingBaseID = renderer->registerSubObjectIDs(particleCount());
		renderer->activateVertexIDs(shader, _chunkSize);
	}

	for(size_t chunkIndex = 0; chunkIndex < _positionsBuffers.size(); chunkIndex++) {
		int chunkSize = _positionsBuffers[chunkIndex].elementCount();

		_positionsBuffers[chunkIndex].bindPositions(renderer, shader);
		_radiiBuffers[chunkIndex].bind(renderer, shader, "particle_radius", GL_FLOAT, 0, 1);
		_transparenciesBuffers[chunkIndex].bind(renderer, shader, "particle_transparency", GL_FLOAT, 0, 1);
		if(!renderer->isPicking()) {
			_colorsBuffers[chunkIndex].bindColors(renderer, shader, 4);
		}
		else {
			shader->setUniformValue("pickingBaseID", pickingBaseID);
			pickingBaseID += _chunkSize;
		}

		renderer->activateVertexIDs(shader, _positionsBuffers[chunkIndex].elementCount() * verticesPerElement);

		if(_usingGeometryShader) {
			OVITO_ASSERT(verticesPerElement == 1);
			// Are we rendering translucent particles? If yes, render them in back to front order to avoid visual artifacts at overlapping particles.
			if(!renderer->isPicking() && translucentParticles() && !_particleCoordinates.empty()) {
				// Create OpenGL index buffer which can be used with glDrawElements.
				OpenGLBuffer<GLuint> primitiveIndices(QOpenGLBuffer::IndexBuffer);
				primitiveIndices.create(QOpenGLBuffer::StaticDraw, particleCount());
				primitiveIndices.fill(determineRenderingOrder(renderer).data());
				primitiveIndices.oglBuffer().bind();
				OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_POINTS, particleCount(), GL_UNSIGNED_INT, nullptr));
				primitiveIndices.oglBuffer().release();
			}
			else {
				// Fully opaque particles can be rendered in unsorted storage order.
				OVITO_CHECK_OPENGL(renderer, renderer->glDrawArrays(GL_POINTS, 0, chunkSize));
			}
		}
		else {
			OVITO_ASSERT(verticesPerElement == 6);
			// Are we rendering translucent particles? If yes, render them in back to front order to avoid visual artifacts at overlapping particles.
			if(!renderer->isPicking() && translucentParticles() && !_particleCoordinates.empty()) {
				auto indices = determineRenderingOrder(renderer);
				// Create OpenGL index buffer which can be used with glDrawElements.
				OpenGLBuffer<GLuint> primitiveIndices(QOpenGLBuffer::IndexBuffer);
				primitiveIndices.create(QOpenGLBuffer::StaticDraw, verticesPerElement * particleCount());
				GLuint* p = primitiveIndices.map();
				for(size_t i = 0; i < indices.size(); i++, p += verticesPerElement)
					std::iota(p, p + verticesPerElement, indices[i]*verticesPerElement);
				primitiveIndices.unmap();
				primitiveIndices.oglBuffer().bind();
				OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_TRIANGLES, particleCount() * verticesPerElement, GL_UNSIGNED_INT, nullptr));
				primitiveIndices.oglBuffer().release();
			}
			else {
				// Fully opaque particles can be rendered in unsorted storage order.
				OVITO_CHECK_OPENGL(renderer, renderer->glDrawArrays(GL_TRIANGLES, 0, chunkSize * verticesPerElement));
			}
		}

		_positionsBuffers[chunkIndex].detachPositions(renderer, shader);
		_radiiBuffers[chunkIndex].detach(renderer, shader, "particle_radius");
		_transparenciesBuffers[chunkIndex].detach(renderer, shader, "particle_transparency");
		if(!renderer->isPicking())
			_colorsBuffers[chunkIndex].detachColors(renderer, shader);
	}

	renderer->deactivateVertexIDs(shader);
	shader->release();

	//Modif
//	if(!renderer->isPicking() && translucentParticles())
		renderer->glDisable(GL_BLEND);

	if(particleShape() == SphericalShape && shadingMode() == NormalShading && !renderer->isPicking())
		deactivateBillboardTexture(renderer);

	renderer->glDisable(GL_CULL_FACE);
}

/******************************************************************************
* Creates the textures used for billboard rendering of particles.
******************************************************************************/
void OpenGLParticlePrimitive::initializeBillboardTexture(OpenGLSceneRenderer* renderer)
{
	static std::vector<std::array<GLubyte,4>> textureImages[BILLBOARD_TEXTURE_LEVELS];
	static bool generatedImages = false;

	if(generatedImages == false) {
		generatedImages = true;
		for(int mipmapLevel = 0; mipmapLevel < BILLBOARD_TEXTURE_LEVELS; mipmapLevel++) {
			int resolution = (1 << (BILLBOARD_TEXTURE_LEVELS - mipmapLevel - 1));
			textureImages[mipmapLevel].resize(resolution*resolution);
			size_t pixelOffset = 0;
			for(int y = 0; y < resolution; y++) {
				for(int x = 0; x < resolution; x++, pixelOffset++) {
                    Vector2 r((FloatType(x - resolution/2) + 0.5f) / (resolution/2), (FloatType(y - resolution/2) + 0.5f) / (resolution/2));
					FloatType r2 = r.squaredLength();
					FloatType r2_clamped = std::min(r2, FloatType(1));
                    FloatType diffuse_brightness = sqrt(1 - r2_clamped) * 0.6f + 0.4f;

					textureImages[mipmapLevel][pixelOffset][0] =
                            (GLubyte)(std::min(diffuse_brightness, (FloatType)1.0f) * 255.0f);

					textureImages[mipmapLevel][pixelOffset][2] = 255;
					textureImages[mipmapLevel][pixelOffset][3] = 255;

                    if(r2 < 1.0f) {
						// Store specular brightness in alpha channel of texture.
                        Vector2 sr = r + Vector2(0.6883f, 0.982f);
						FloatType specular = std::max(FloatType(1) - sr.squaredLength(), FloatType(0));
						specular *= specular;
						specular *= specular * (1 - r2_clamped*r2_clamped);
						textureImages[mipmapLevel][pixelOffset][1] =
                                (GLubyte)(std::min(specular, FloatType(1)) * 255.0f);
					}
					else {
						// Set transparent pixel.
						textureImages[mipmapLevel][pixelOffset][1] = 0;
					}
				}
			}
		}
	}

	_billboardTexture.create();
	_billboardTexture.bind();

	// Transfer pixel data to OpenGL texture.
	for(int mipmapLevel = 0; mipmapLevel < BILLBOARD_TEXTURE_LEVELS; mipmapLevel++) {
		int resolution = (1 << (BILLBOARD_TEXTURE_LEVELS - mipmapLevel - 1));

		OVITO_CHECK_OPENGL(renderer, renderer->glTexImage2D(GL_TEXTURE_2D, mipmapLevel, GL_RGBA,
				resolution, resolution, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureImages[mipmapLevel].data()));
	}
}

/******************************************************************************
* Activates a texture for billboard rendering of spherical particles.
******************************************************************************/
void OpenGLParticlePrimitive::activateBillboardTexture(OpenGLSceneRenderer* renderer)
{
	OVITO_ASSERT(_billboardTexture.isCreated());
	OVITO_ASSERT(shadingMode() != FlatShading);
	OVITO_ASSERT(!renderer->isPicking());
	OVITO_ASSERT(particleShape() == SphericalShape);

	// Enable texture mapping when using compatibility OpenGL.
	// In the core profile, this is already enabled by default.
	if(renderer->isCoreProfile() == false && !renderer->glcontext()->isOpenGLES())
		OVITO_CHECK_OPENGL(renderer, renderer->glEnable(GL_TEXTURE_2D));

	_billboardTexture.bind();

	OVITO_CHECK_OPENGL(renderer, renderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST));
	OVITO_CHECK_OPENGL(renderer, renderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

#ifndef Q_OS_WASM
	OVITO_STATIC_ASSERT(BILLBOARD_TEXTURE_LEVELS >= 3);
	OVITO_CHECK_OPENGL(renderer, renderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, BILLBOARD_TEXTURE_LEVELS - 3));
#endif	
}

/******************************************************************************
* Deactivates the texture used for billboard rendering of spherical particles.
******************************************************************************/
void OpenGLParticlePrimitive::deactivateBillboardTexture(OpenGLSceneRenderer* renderer)
{
	// Disable texture mapping again when not using core profile.
	if(renderer->isCoreProfile() == false && !renderer->glcontext()->isOpenGLES())
		OVITO_CHECK_OPENGL(renderer, renderer->glDisable(GL_TEXTURE_2D));
}

/******************************************************************************
* Returns an array of particle indices, sorted back-to-front, which is used
* to render translucent particles.
******************************************************************************/
std::vector<GLuint> OpenGLParticlePrimitive::determineRenderingOrder(OpenGLSceneRenderer* renderer)
{
	// Create array of particle indices.
	std::vector<GLuint> indices(particleCount());
	std::iota(indices.begin(), indices.end(), 0);
	if(!_particleCoordinates.empty()) {
		// Viewing direction in object space:
		Vector3 direction = renderer->modelViewTM().inverse().column(2);

		OVITO_ASSERT(_particleCoordinates.size() == particleCount());
		// First compute distance of each particle from the camera along viewing direction (=camera z-axis).
		std::vector<FloatType> distances(particleCount());
		std::transform(_particleCoordinates.begin(), _particleCoordinates.end(), distances.begin(), [direction](const Point3& p) {
			return direction.dot(p - Point3::Origin());
		});
		// Now sort particle indices with respect to distance (back-to-front order).
		std::sort(indices.begin(), indices.end(), [&distances](GLuint a, GLuint b) {
			return distances[a] < distances[b];
		});
	}
	return indices;
}

}	// End of namespace
