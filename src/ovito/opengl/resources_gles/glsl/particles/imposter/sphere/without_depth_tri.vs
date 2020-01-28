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

precision mediump float;

// Inputs from calling program:
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform vec2 imposter_texcoords[6];
uniform vec4 imposter_voffsets[6];
uniform float radius_scalingfactor;

#if __VERSION__ >= 300 // OpenGL ES 3.0:

// The particle data:
int vec3 position;
int vec4 color;
int float particle_radius;

// Outputs to fragment shader:
flat out vec4 particle_color_fs;
out vec2 texcoords;

void main()
{
	// Pass color to fragment shader.
	particle_color_fs = color;

	// Assign texture coordinates.
	int cornerIndex = gl_VertexID % 6;
	texcoords = imposter_texcoords[cornerIndex];

	// Transform and project particle position.
	vec4 eye_position = modelview_matrix * vec4(position, 1.0);

	gl_Position = projection_matrix * (eye_position + (particle_radius * radius_scalingfactor) * imposter_voffsets[cornerIndex]);
}

#else // OpenGL ES 2.0:

// The particle data:
attribute vec3 position;
attribute vec4 color;
attribute float particle_radius;
attribute float vertexID;

// Outputs to fragment shader:
varying vec4 particle_color_fs;
varying vec2 texcoords;

void main()
{
	// Pass color to fragment shader.
	particle_color_fs = color;

	// Assign texture coordinates.
	int cornerIndex = int(mod(vertexID, 6.0));
	texcoords = imposter_texcoords[cornerIndex];

	// Transform and project particle position.
	vec4 eye_position = modelview_matrix * vec4(position, 1.0);

	gl_Position = projection_matrix * (eye_position + (particle_radius * radius_scalingfactor) * imposter_voffsets[cornerIndex]);
}

#endif