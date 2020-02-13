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
uniform mat4 modelview_projection_matrix;
uniform bool is_perspective;
uniform vec3 parallel_view_dir;
uniform vec3 eye_pos;
uniform int pickingBaseID;
uniform int verticesPerElement;

#if __VERSION__ >= 300 // OpenGL ES 3.0

	in vec3 position;

	in vec3 cylinder_base;
	in vec3 cylinder_axis;

	flat out vec4 vertex_color_out;

#else // OpenGL ES 2.0

	attribute vec3 position;
	attribute float vertexID;

	attribute vec3 cylinder_base;
	attribute vec3 cylinder_axis;

	varying vec4 vertex_color_out;

#endif

void main()
{
#if __VERSION__ >= 300 // OpenGL ES 3.0

	// Compute sub-object ID from vertex ID.
	int objectID = pickingBaseID + gl_VertexID / verticesPerElement;

	// Encode sub-object ID as an RGBA color in the rendered image.
	vertex_color_out = vec4(
		float(objectID & 0xFF) / 255.0,
		float((objectID >> 8) & 0xFF) / 255.0,
		float((objectID >> 16) & 0xFF) / 255.0,
		float((objectID >> 24) & 0xFF) / 255.0);

#else

	// Compute sub-object ID from vertex ID.
	float objectID = float(pickingBaseID + int(vertexID) / verticesPerElement);

	// Encode sub-object ID as an RGBA color in the rendered image.
	vertex_color_out = vec4(
		floor(mod(objectID, 256.0)) / 255.0,
		floor(mod(objectID / 256.0, 256.0)) / 255.0,
		floor(mod(objectID / 65536.0, 256.0)) / 255.0,
		floor(mod(objectID / 16777216.0, 256.0)) / 255.0);

#endif

	if(cylinder_axis != vec3(0)) {

		// Get view direction.
		vec3 view_dir;
		if(!is_perspective)
			view_dir = parallel_view_dir;
		else
			view_dir = eye_pos - cylinder_base;

		// Build local coordinate system.
		vec3 u = normalize(cross(view_dir, cylinder_axis));
		vec3 rotated_pos = cylinder_axis * position.x + u * position.y + cylinder_base;
		gl_Position = modelview_projection_matrix * vec4(rotated_pos, 1.0);
	}
	else {
		gl_Position = vec4(0);
	}
}
