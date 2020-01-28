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
uniform mat4 projection_matrix;
uniform mat4 viewprojection_matrix;
uniform mat4 model_matrix;
uniform mat4 modelview_matrix;
uniform vec3 cubeVerts[24];
uniform float marker_size;

#if __VERSION__ >= 300 // OpenGL ES 3.0

	// The marker data:
	in vec3 position;
	in vec4 color;
	
	// Outputs to fragment shader
	flat out vec4 vertex_color_fs;

#else // OpenGL ES 2.0:

	// The marker data:
	attribute vec3 position;
	attribute vec4 color;
	attribute float vertexID;

	// Outputs to fragment shader
	varying vec4 vertex_color_fs;

#endif

void main()
{
	// Forward color to fragment shader.
	vertex_color_fs = color;

	// Determine marker size.
	vec4 view_position = modelview_matrix * vec4(position, 1.0);
	float w = projection_matrix[0][3] * view_position.x + projection_matrix[1][3] * view_position.y
			+ projection_matrix[2][3] * view_position.z + projection_matrix[3][3];

	// Transform and project vertex.
#if __VERSION__ >= 300 // OpenGL ES 3.0
	int cubeCorner = gl_VertexID % 24;
#else
	int cubeCorner = int(mod(vertexID, 24.0));
#endif
	vec3 delta = cubeVerts[cubeCorner] * (w * marker_size);
	vec3 ec_pos = (model_matrix * vec4(position, 1.0)).xyz;

	gl_Position = viewprojection_matrix * vec4(ec_pos + delta, 1.0);
}
