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
uniform mat4 modelview_projection_matrix;
uniform float modelview_uniform_scale;
uniform int pickingBaseID;
uniform int verticesPerElement;

#if __VERSION__ >= 300 // OpenGL ES 3.0

	// The vertex data:
	in vec3 position;

	// The cylinder data:
	in vec3 cylinder_base;				// The position of the cylinder in model coordinates.
	in vec3 cylinder_axis;				// The axis of the cylinder in model coordinates.
	in float cylinder_radius;			// The radius of the cylinder in model coordinates.

	// Outputs to fragment shader
	flat out vec4 cylinder_color_fs;		// The base color of the cylinder.
	flat out vec3 cylinder_view_base;		// Transformed cylinder position in view coordinates
	flat out vec3 cylinder_view_axis;		// Transformed cylinder axis in view coordinates
	flat out float cylinder_radius_sq_fs;	// The squared radius of the cylinder
	flat out float cylinder_length;			// The length of the cylinder

#else // OpenGL ES 2.0:

	// The vertex data:
	attribute vec3 position;
	attribute float vertexID;

	// The cylinder data:
	attribute vec3 cylinder_base;				// The position of the cylinder in model coordinates.
	attribute vec3 cylinder_axis;				// The axis of the cylinder in model coordinates.
	attribute float cylinder_radius;			// The radius of the cylinder in model coordinates.

	// Outputs to fragment shader
	varying vec4 cylinder_color_fs;			// The base color of the cylinder.
	varying vec3 cylinder_view_base;		// Transformed cylinder position in view coordinates
	varying vec3 cylinder_view_axis;		// Transformed cylinder axis in view coordinates
	varying float cylinder_radius_sq_fs;	// The squared radius of the cylinder
	varying float cylinder_length;			// The length of the cylinder

#endif

void main()
{
#if __VERSION__ >= 300 // OpenGL ES 3.0

	// Compute sub-object ID from vertex ID.
	int objectID = pickingBaseID + gl_VertexID / verticesPerElement;

	// Encode sub-object ID as an RGBA color in the rendered image.
	cylinder_color_fs = vec4(
		float(objectID & 0xFF) / 255.0,
		float((objectID >> 8) & 0xFF) / 255.0,
		float((objectID >> 16) & 0xFF) / 255.0,
		float((objectID >> 24) & 0xFF) / 255.0);

#else // OpenGL ES 2.0:

	// Compute sub-object ID from vertex ID.
	float objectID = float(pickingBaseID + int(vertexID) / verticesPerElement);

	// Encode sub-object ID as an RGBA color in the rendered image.
	cylinder_color_fs = vec4(
		floor(mod(objectID, 256.0)) / 255.0,
		floor(mod(objectID / 256.0, 256.0)) / 255.0,
		floor(mod(objectID / 65536.0, 256.0)) / 255.0,
		floor(mod(objectID / 16777216.0, 256.0)) / 255.0);

#endif

	// Transform and project vertex position.
	gl_Position = modelview_projection_matrix * vec4(position, 1.0);

	// Pass radius to fragment shader.
	cylinder_radius_sq_fs = cylinder_radius * modelview_uniform_scale;
	cylinder_radius_sq_fs *= cylinder_radius_sq_fs;

	// Transform cylinder to eye coordinates.
	cylinder_view_base = vec3(modelview_matrix * vec4(cylinder_base, 1));
	cylinder_view_axis = vec3(modelview_matrix * vec4(cylinder_axis, 0));

	// Pass length to fragment shader.
	cylinder_length = length(cylinder_view_axis);
}
