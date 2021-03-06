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

// Inputs from calling program:
uniform float basePointSize;
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform int pickingBaseID;
uniform float radius_scalingfactor;

#if __VERSION__ >= 130

	// The particle data:
	in vec3 position;
	in float particle_radius;

	// Output to fragment shader:
	flat out vec4 particle_color_fs;
	flat out float particle_radius_fs;	// The particle's radius.
	flat out float ze0;					// The particle's Z coordinate in eye coordinates.

#else

	// The particle data:
	attribute float particle_radius;
	attribute float vertexID;

	// Output to fragment shader:
	#define particle_radius_fs gl_TexCoord[1].x
	#define ze0 gl_TexCoord[1].y

#endif

void main()
{
#if __VERSION__ >= 130
	// Compute color from object ID.
	int objectID = pickingBaseID + gl_VertexID;

	particle_color_fs = vec4(
		float(objectID & 0xFF) / 255.0,
		float((objectID >> 8) & 0xFF) / 255.0,
		float((objectID >> 16) & 0xFF) / 255.0,
		float((objectID >> 24) & 0xFF) / 255.0);

	// Transform and project particle position.
	vec4 eye_position = modelview_matrix * vec4(position, 1);

#else
	// Compute color from object ID.
	float objectID = pickingBaseID + vertexID;

	gl_FrontColor = vec4(
		floor(mod(objectID, 256.0)) / 255.0,
		floor(mod(objectID / 256.0, 256.0)) / 255.0,
		floor(mod(objectID / 65536.0, 256.0)) / 255.0,
		floor(mod(objectID / 16777216.0, 256.0)) / 255.0);

	// Transform and project particle position.
	vec4 eye_position = modelview_matrix * gl_Vertex;
#endif

	// Transform and project particle position.
	gl_Position = projection_matrix * eye_position;

	// Compute sprite size.
	gl_PointSize = basePointSize * particle_radius / (eye_position.z * projection_matrix[2][3] + projection_matrix[3][3]);

	// Forward particle radius to fragment shader.
	particle_radius_fs = particle_radius * radius_scalingfactor;

	// Pass particle position in eye coordinates to fragment shader.
	ze0 = eye_position.z;
}
