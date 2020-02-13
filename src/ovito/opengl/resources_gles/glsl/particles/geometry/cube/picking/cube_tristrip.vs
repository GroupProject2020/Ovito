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
uniform int pickingBaseID;
uniform vec3 cubeVerts[14];

#if __VERSION__ >= 300 // OpenGL ES 3.0

// The particle data:
in vec3 position;
in float particle_radius;

// Outputs to fragment shader
flat out vec4 particle_color_fs;

void main()
{
	// Compute sub-object ID from vertex ID.
	int objectID = pickingBaseID + gl_VertexID / 14;

	// Encode sub-object ID as an RGBA color in the rendered image.
	particle_color_fs = vec4(
		float(objectID & 0xFF) / 255.0,
		float((objectID >> 8) & 0xFF) / 255.0,
		float((objectID >> 16) & 0xFF) / 255.0,
		float((objectID >> 24) & 0xFF) / 255.0);

	// Transform and project vertex.
	int cubeCorner = gl_VertexID % 14;
	gl_Position = modelviewprojection_matrix * vec4(position + cubeVerts[cubeCorner] * particle_radius, 1.0);
}

#else // OpenGL ES 2.0

// The particle data:
attribute vec3 position;
attribute float particle_radius;
attribute float vertexID;

// Outputs to fragment shader
varying vec4 particle_color_fs;

void main()
{
	// Compute sub-object ID from vertex ID.
	float objectID = float(pickingBaseID + int(vertexID) / 14);

	// Encode sub-object ID as an RGBA color in the rendered image.
	particle_color_fs = vec4(
		floor(mod(objectID, 256.0)) / 255.0,
		floor(mod(objectID / 256.0, 256.0)) / 255.0,
		floor(mod(objectID / 65536.0, 256.0)) / 255.0,
		floor(mod(objectID / 16777216.0, 256.0)) / 255.0);

	// Transform and project vertex.
	int cubeCorner = int(mod(vertexID, 14.0));
	gl_Position = modelviewprojection_matrix * vec4(position + cubeVerts[cubeCorner] * particle_radius, 1.0);
}

#endif