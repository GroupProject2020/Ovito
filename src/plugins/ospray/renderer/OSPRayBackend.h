///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once


#include <core/Core.h>
#include <core/oo/RefTarget.h>

// Foward declarations from OSPRay library:
namespace ospray { namespace cpp {
	class Renderer;
	class Material;
	class Light;
}}

namespace Ovito { namespace OSPRay {

/**
 * \brief Base wrapper class for OSPRay rendering backends.
 */
class OVITO_OSPRAYRENDERER_EXPORT OSPRayBackend : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(OSPRayBackend)
	Q_CLASSINFO("DisplayName", "OSPRay backend");
	
public:

	/// Constructor.
	OSPRayBackend(DataSet* dataset) : RefTarget(dataset) {}

	/// Creates the OSPRay renderer object and configures it.
	virtual ospray::cpp::Renderer createOSPRenderer(const Color& backgroundColor) = 0;

	/// Creates an OSPRay material.
	virtual ospray::cpp::Material createOSPMaterial(const char* type) = 0;	

	/// Creates an OSPRay light.
	virtual ospray::cpp::Light createOSPLight(const char* type) = 0;	
};

/**
 * \brief Wrapper class for the OSPRay SciVis rendering backend.
 */
class OVITO_OSPRAYRENDERER_EXPORT OSPRaySciVisBackend : public OSPRayBackend
{
	Q_OBJECT
	OVITO_CLASS(OSPRaySciVisBackend)
	Q_CLASSINFO("DisplayName", "SciVis");
	
public:

	/// Constructor.
	Q_INVOKABLE OSPRaySciVisBackend(DataSet* dataset);

	/// Creates the OSPRay renderer object and configures it.
	virtual ospray::cpp::Renderer createOSPRenderer(const Color& backgroundColor) override;

	/// Creates an OSPRay material.
	virtual ospray::cpp::Material createOSPMaterial(const char* type) override;	

	/// Creates an OSPRay light.
	virtual ospray::cpp::Light createOSPLight(const char* type) override;	
	
private:

	/// Enables shadows for the direct light source.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, shadowsEnabled, setShadowsEnabled, PROPERTY_FIELD_MEMORIZE);	

	/// Enables ambient occlusion lighting.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, ambientOcclusionEnabled, setAmbientOcclusionEnabled, PROPERTY_FIELD_MEMORIZE);
	
	/// Controls quality of ambient occlusion.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, ambientOcclusionSamples, setAmbientOcclusionSamples, PROPERTY_FIELD_MEMORIZE);		
};

/**
 * \brief Wrapper class for the OSPRay Path Tracer rendering backend.
 */
class OVITO_OSPRAYRENDERER_EXPORT OSPRayPathTracerBackend : public OSPRayBackend
{
	Q_OBJECT
	OVITO_CLASS(OSPRayPathTracerBackend)
	Q_CLASSINFO("DisplayName", "Path Tracer");
	
public:

	/// Constructor.
	Q_INVOKABLE OSPRayPathTracerBackend(DataSet* dataset);

	/// Creates the OSPRay renderer object and configures it.
	virtual ospray::cpp::Renderer createOSPRenderer(const Color& backgroundColor) override;
	
	/// Creates an OSPRay material.
	virtual ospray::cpp::Material createOSPMaterial(const char* type) override;	

	/// Creates an OSPRay light.
	virtual ospray::cpp::Light createOSPLight(const char* type) override;	

private:

	/// Controls ray recursion depth at which to start Russian roulette termination.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, rouletteDepth, setRouletteDepth);
};

}	// End of namespace
}	// End of namespace
