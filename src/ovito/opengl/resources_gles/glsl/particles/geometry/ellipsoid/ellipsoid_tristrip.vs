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

#if __VERSION__ >= 300 // OpenGL ES 3.0

// The particle data:
in vec3 position;
in vec4 color;
in vec3 shape;
in vec4 orientation;
in float particle_radius;

// Outputs to fragment shader
flat out vec4 particle_color_fs;
flat out mat3 particle_quadric_fs;
flat out vec3 particle_view_pos_fs;

void main()
{
	mat3 rot;
	if(orientation != vec4(0)) {
		// Normalize quaternion.
		vec4 quat = normalize(orientation);
		rot = mat3(
			1.0 - 2.0*(quat.y*quat.y + quat.z*quat.z),
			2.0*(quat.x*quat.y + quat.w*quat.z),
			2.0*(quat.x*quat.z - quat.w*quat.y),
			2.0*(quat.x*quat.y - quat.w*quat.z),
			1.0 - 2.0*(quat.x*quat.x + quat.z*quat.z),
			2.0*(quat.y*quat.z + quat.w*quat.x),
			2.0*(quat.x*quat.z + quat.w*quat.y),
			2.0*(quat.y*quat.z - quat.w*quat.x),
			1.0 - 2.0*(quat.x*quat.x + quat.y*quat.y)
		);
	}
	else {
		rot = mat3(1.0);
	}

	vec3 shape2 = shape;
	if(shape2.x == 0.0) shape2.x = particle_radius;
	if(shape2.y == 0.0) shape2.y = particle_radius;
	if(shape2.z == 0.0) shape2.z = particle_radius;

	mat3 qmat = mat3(1.0/(shape2.x*shape2.x), 0, 0,
			 		 0, 1.0/(shape2.y*shape2.y), 0,
			  		 0, 0, 1.0/(shape2.z*shape2.z));

	mat3 view_rot = mat3(modelview_matrix) * rot;

    particle_quadric_fs = view_rot * qmat * transpose(view_rot);

	// Forward color to fragment shader.
	particle_color_fs = color;

	// Transform and project vertex.
	int cubeCorner = gl_VvertexID % 14;
	vec3 delta = cubeVerts[cubeCorner] * shape2;
	gl_Position = modelviewprojection_matrix * vec4(position + rot * delta, 1);

	particle_view_pos_fs = (modelview_matrix * vec4(position, 1)).xyz;
}

#else // OpenGL ES 2.0

// The particle data:
attribute vec3 position;
attribute vec4 color;
attribute vec3 shape;
attribute vec4 orientation;
attribute float particle_radius;
attribute float vertexID;

// Outputs to fragment shader
varying vec4 particle_color_fs;
varying mat3 particle_quadric_fs;
varying vec3 particle_view_pos_fs;

// Transposes a 3x3 matrix.
// Note: OpenGL ES 2.0 has no built-in transpose() function.
mat3 transposeMat3(in mat3 inMatrix) {
    vec3 i0 = inMatrix[0];
    vec3 i1 = inMatrix[1];
    vec3 i2 = inMatrix[2];
    
    mat3 outMatrix = mat3(
                 vec3(i0.x, i1.x, i2.x),
                 vec3(i0.y, i1.y, i2.y),
                 vec3(i0.z, i1.z, i2.z));

    return outMatrix;
}

void main()
{
	mat3 rot;
	if(orientation != vec4(0)) {
		// Normalize quaternion.
		vec4 quat = normalize(orientation);
		rot = mat3(
			1.0 - 2.0*(quat.y*quat.y + quat.z*quat.z),
			2.0*(quat.x*quat.y + quat.w*quat.z),
			2.0*(quat.x*quat.z - quat.w*quat.y),
			2.0*(quat.x*quat.y - quat.w*quat.z),
			1.0 - 2.0*(quat.x*quat.x + quat.z*quat.z),
			2.0*(quat.y*quat.z + quat.w*quat.x),
			2.0*(quat.x*quat.z + quat.w*quat.y),
			2.0*(quat.y*quat.z - quat.w*quat.x),
			1.0 - 2.0*(quat.x*quat.x + quat.y*quat.y)
		);
	}
	else {
		rot = mat3(1.0);
	}

	vec3 shape2 = shape;
	if(shape2.x == 0.0) shape2.x = particle_radius;
	if(shape2.y == 0.0) shape2.y = particle_radius;
	if(shape2.z == 0.0) shape2.z = particle_radius;

	mat3 qmat = mat3(1.0/(shape2.x*shape2.x), 0, 0,
			 		 0, 1.0/(shape2.y*shape2.y), 0,
			  		 0, 0, 1.0/(shape2.z*shape2.z));

	mat3 view_rot = mat3(modelview_matrix) * rot;

    particle_quadric_fs = view_rot * qmat * transposeMat3(view_rot);

	// Forward color to fragment shader.
	particle_color_fs = color;

	// Transform and project vertex.
	int cubeCorner = int(mod(vertexID, 14.0));
	vec3 delta = cubeVerts[cubeCorner] * shape2;
	gl_Position = modelviewprojection_matrix * vec4(position + rot * delta, 1);

	particle_view_pos_fs = (modelview_matrix * vec4(position, 1)).xyz;
}

#endif