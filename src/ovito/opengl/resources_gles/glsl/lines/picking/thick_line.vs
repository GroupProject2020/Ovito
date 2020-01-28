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

precision highp float;

// Inputs from calling program:
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform bool is_perspective;
uniform float line_width;
uniform int pickingBaseID;

#if __VERSION__ >= 300 // OpenGL ES 3.0

	in vec3 position;
	in vec3 vector;

	flat out vec4 vertex_color_fs;

#else

	attribute vec3 position;
	attribute float vertexID;
	attribute vec3 vector;

	varying vec4 vertex_color_fs;

#endif

void main()
{
#if __VERSION__ >= 300 // OpenGL ES 3.0

	// Compute sub-object ID from vertex ID.
	int objectID = pickingBaseID + gl_VertexID / 4;

	// Encode sub-object ID as an RGBA color in the rendered image.
	vertex_color_fs = vec4(
		float(objectID & 0xFF) / 255.0,
		float((objectID >> 8) & 0xFF) / 255.0,
		float((objectID >> 16) & 0xFF) / 255.0,
		float((objectID >> 24) & 0xFF) / 255.0);

#else // OpenGL ES 2.0:

	// Compute sub-object ID from vertex ID.
	float objectID = float(pickingBaseID + int(vertexID) / 4);

	// Encode sub-object ID as an RGBA color in the rendered image.
	vertex_color_fs = vec4(
		floor(mod(objectID, 256.0)) / 255.0,
		floor(mod(objectID / 256.0, 256.0)) / 255.0,
		floor(mod(objectID / 65536.0, 256.0)) / 255.0,
		floor(mod(objectID / 16777216.0, 256.0)) / 255.0);

#endif

	vec4 view_position = modelview_matrix * vec4(position, 1.0);

	vec3 view_dir;
	if(is_perspective)
		view_dir = view_position.xyz;
	else
		view_dir = vec3(0.0, 0.0, -1.0);
	vec3 u = cross(view_dir, (modelview_matrix * vec4(vector,0.0)).xyz);
	if(u != vec3(0)) {
		float w = projection_matrix[0][3] * view_position.x + projection_matrix[1][3] * view_position.y
			+ projection_matrix[2][3] * view_position.z + projection_matrix[3][3];
		gl_Position = projection_matrix * (view_position - vec4((w * line_width / length(u)) * u, 0.0));
	}
	else {
		gl_Position = vec4(0);
	}
}
