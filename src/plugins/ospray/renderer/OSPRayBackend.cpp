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

#include <core/Core.h>
#include "OSPRayBackend.h"

#include <ospray/ospray_cpp.h>

namespace Ovito { namespace OSPRay {

IMPLEMENT_OVITO_CLASS(OSPRayBackend);

IMPLEMENT_OVITO_CLASS(OSPRaySciVisBackend);
DEFINE_PROPERTY_FIELD(OSPRaySciVisBackend, shadowsEnabled);
DEFINE_PROPERTY_FIELD(OSPRaySciVisBackend, ambientOcclusionEnabled);
DEFINE_PROPERTY_FIELD(OSPRaySciVisBackend, ambientOcclusionSamples);
SET_PROPERTY_FIELD_LABEL(OSPRaySciVisBackend, shadowsEnabled, "Shadows");
SET_PROPERTY_FIELD_LABEL(OSPRaySciVisBackend, ambientOcclusionEnabled, "Ambient occlusion");
SET_PROPERTY_FIELD_LABEL(OSPRaySciVisBackend, ambientOcclusionSamples, "Ambient occlusion samples");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRaySciVisBackend, ambientOcclusionSamples, IntegerParameterUnit, 1, 100);

IMPLEMENT_OVITO_CLASS(OSPRayPathTracerBackend);
DEFINE_PROPERTY_FIELD(OSPRayPathTracerBackend, rouletteDepth);
SET_PROPERTY_FIELD_LABEL(OSPRayPathTracerBackend, rouletteDepth, "Roulette depth");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(OSPRayPathTracerBackend, rouletteDepth, IntegerParameterUnit, 1, 100);

/******************************************************************************
* Constructor.
******************************************************************************/
OSPRaySciVisBackend::OSPRaySciVisBackend(DataSet* dataset) : OSPRayBackend(dataset),
	_shadowsEnabled(true),
	_ambientOcclusionEnabled(true), 
	_ambientOcclusionSamples(12)
{
}

/******************************************************************************
* Creates the OSPRay renderer object and configures it.
******************************************************************************/
ospray::cpp::Renderer OSPRaySciVisBackend::createOSPRenderer(const Color& backgroundColor)
{
	ospray::cpp::Renderer renderer("scivis"); 
	renderer.set("shadowsEnabled", shadowsEnabled());
	renderer.set("aoSamples", ambientOcclusionEnabled() ? ambientOcclusionSamples() : 0);
	renderer.set("aoTransparencyEnabled", true);
	renderer.set("bgColor", backgroundColor.r(), backgroundColor.g(), backgroundColor.b(), 0.0);
	return renderer;
}

/******************************************************************************
* Creates an OSPRay material.
******************************************************************************/
ospray::cpp::Material OSPRaySciVisBackend::createOSPMaterial(const char* type)
{
	return ospray::cpp::Material("scivis", type); 
}

/******************************************************************************
* Creates an OSPRay light.
******************************************************************************/
ospray::cpp::Light OSPRaySciVisBackend::createOSPLight(const char* type)
{
	return ospray::cpp::Light("scivis", type); 
}

/******************************************************************************
* Constructor.
******************************************************************************/
OSPRayPathTracerBackend::OSPRayPathTracerBackend(DataSet* dataset) : OSPRayBackend(dataset),
	_rouletteDepth(5)
{
}

/******************************************************************************
* Creates the OSPRay renderer object and configures it.
******************************************************************************/
ospray::cpp::Renderer OSPRayPathTracerBackend::createOSPRenderer(const Color& backgroundColor)
{
	ospray::cpp::Renderer renderer("pathtracer"); 
	renderer.set("rouletteDepth", rouletteDepth());
	return renderer;
}

/******************************************************************************
* Creates an OSPRay material.
******************************************************************************/
ospray::cpp::Material OSPRayPathTracerBackend::createOSPMaterial(const char* type)
{
	return ospray::cpp::Material("pathtracer", type); 
}

/******************************************************************************
* Creates an OSPRay light.
******************************************************************************/
ospray::cpp::Light OSPRayPathTracerBackend::createOSPLight(const char* type)
{
	return ospray::cpp::Light("pathtracer", type); 
}

}	// End of namespace
}	// End of namespace
