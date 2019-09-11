///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2017) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

// Inputs from calling program:
uniform mat4 projection_matrix;
uniform mat4 viewprojection_matrix;
uniform mat4 model_matrix;
uniform mat4 modelview_matrix;
uniform vec3 cubeVerts[24];
uniform float marker_size;
uniform int pickingBaseID;

#if __VERSION__ >= 130

	// The marker data:
	in vec3 position;
	
	// Outputs to fragment shader
	flat out vec4 vertex_color_fs;
	
#else
	
	// The marker data:
	attribute float vertexID;

#endif

void main()
{	
	// Compute color from object ID.
#if __VERSION__ >= 130
	int objectID = pickingBaseID + gl_VertexID / 2;
	vertex_color_fs = vec4(
		float(objectID & 0xFF) / 255.0, 
		float((objectID >> 8) & 0xFF) / 255.0, 
		float((objectID >> 16) & 0xFF) / 255.0, 
		float((objectID >> 24) & 0xFF) / 255.0);		
#else
	float objectID = pickingBaseID + floor(vertexID / 2);
	gl_FrontColor = vec4(
		floor(mod(objectID, 256.0)) / 255.0,
		floor(mod(objectID / 256.0, 256.0)) / 255.0, 
		floor(mod(objectID / 65536.0, 256.0)) / 255.0, 
		floor(mod(objectID / 16777216.0, 256.0)) / 255.0);		
#endif

#if __VERSION__ >= 130

	// Determine marker size.
	vec4 view_position = modelview_matrix * vec4(position, 1.0);
	float w = projection_matrix[0][3] * view_position.x + projection_matrix[1][3] * view_position.y
			+ projection_matrix[2][3] * view_position.z + projection_matrix[3][3];

	// Transform and project vertex.
	int cubeCorner = gl_VertexID % 24;
	vec3 delta = cubeVerts[cubeCorner] * (w * marker_size);	
	vec3 ec_pos = (model_matrix * vec4(position, 1)).xyz;

#else

	// Determine marker size.
	vec4 view_position = modelview_matrix * gl_Vertex;
	float w = projection_matrix[0][3] * view_position.x + projection_matrix[1][3] * view_position.y
			+ projection_matrix[2][3] * view_position.z + projection_matrix[3][3];

	// Transform and project vertex.
	int cubeCorner = int(mod(vertexID+0.5, 24.0));
	vec3 delta = cubeVerts[cubeCorner] * (w * marker_size);
	vec3 ec_pos = (model_matrix * gl_Vertex).xyz;

#endif

	gl_Position = viewprojection_matrix * vec4(ec_pos + delta, 1);
}
