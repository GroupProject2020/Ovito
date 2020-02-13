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

precision highp float; 

// Inputs from calling program:
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform mat4 modelviewprojection_matrix;
uniform vec3 cubeVerts[14];
uniform float radius_scalingfactor;

#if __VERSION__ >= 300 // OpenGL ES 3.0

// The particle data:
in vec3 position;
in vec4 color;
in float particle_radius;

// Outputs to fragment shader:
flat out vec4 particle_color_fs;
flat out float particle_radius_squared_fs;
flat out vec3 particle_view_pos_fs;

void main()
{
	particle_color_fs = color;
	particle_radius_squared_fs = particle_radius * particle_radius * radius_scalingfactor * radius_scalingfactor;
	particle_view_pos_fs = vec3(modelview_matrix * vec4(position, 1.0));

	// Transform and project vertex.
	int cubeCorner = gl_VertexID % 14;
	gl_Position = modelviewprojection_matrix * vec4(position + cubeVerts[cubeCorner] * (particle_radius * radius_scalingfactor), 1.0);
}

#else // OpenGL ES 2.0

// The particle data:
attribute vec3 position;
attribute vec4 color;
attribute float particle_radius;
attribute float vertexID;

// Outputs to fragment shader:
varying vec4 particle_color_fs;
varying float particle_radius_squared_fs;
varying vec3 particle_view_pos_fs;

void main()
{
	particle_color_fs = color;
	particle_radius_squared_fs = particle_radius * particle_radius * radius_scalingfactor * radius_scalingfactor;
	particle_view_pos_fs = vec3(modelview_matrix * vec4(position, 1.0));

	// Transform and project vertex.
	int cubeCorner = int(mod(vertexID, 14.0));
	gl_Position = modelviewprojection_matrix * vec4(position + cubeVerts[cubeCorner] * (particle_radius * radius_scalingfactor), 1.0);
}

#endif