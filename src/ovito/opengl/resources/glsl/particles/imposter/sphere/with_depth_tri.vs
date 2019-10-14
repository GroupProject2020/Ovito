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
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform vec2 imposter_texcoords[6];
uniform vec4 imposter_voffsets[6];
uniform float radius_scalingfactor;

#if __VERSION__ >= 130

	// The particle data:
	in vec3 position;
	in vec4 color;
	in float particle_radius;

	// Output to fragment shader:
	flat out vec4 particle_color_fs;
	flat out float particle_radius_fs;	// The particle's radius.
	flat out float ze0;					// The particle's Z coordinate in eye coordinates.
	out vec2 texcoords;

#else

	attribute float particle_radius;
	attribute float vertexID;

	#define particle_radius_fs gl_TexCoord[1].x
	#define ze0 gl_TexCoord[1].y
	#define particle_color_fs gl_FrontColor
#endif

void main()
{
#if __VERSION__ >= 130

	// Forward color to fragment shader.
	particle_color_fs = color;

	// Transform and project particle position.
	vec4 eye_position = modelview_matrix * vec4(position, 1);

	// Assign texture coordinates.
	texcoords = imposter_texcoords[gl_VertexID % 6];

	// Transform and project particle position.
	gl_Position = projection_matrix * (eye_position + (particle_radius * radius_scalingfactor) * imposter_voffsets[gl_VertexID % 6]);
#else

	// Pass color to fragment shader.
	particle_color_fs = gl_Color;

	// Transform and project particle position.
	vec4 eye_position = modelview_matrix * gl_Vertex;

	int cornerIndex = int(mod(vertexID+0.5, 6.0));

	// Assign texture coordinates.
	gl_TexCoord[0].xy = imposter_texcoords[cornerIndex];

	// Transform and project particle position.
	gl_Position = projection_matrix * (eye_position + (particle_radius * radius_scalingfactor) * imposter_voffsets[cornerIndex]);
#endif

	// Forward particle radius to fragment shader.
	particle_radius_fs = particle_radius * radius_scalingfactor;

	// Pass particle position in eye coordinates to fragment shader.
	ze0 = eye_position.z;
}

