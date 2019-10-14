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
uniform float basePointSize;
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

#if __VERSION__ >= 130
	// The particle data:
	in vec3 position;
	in vec4 color;
	in float particle_radius;

	// Output passed to fragment shader.
	flat out vec4 particle_color_fs;
#else
	// The particle data:
	attribute float particle_radius;
#endif

void main()
{
#if __VERSION__ >= 130
	// Pass color to fragment shader.
	particle_color_fs = color;

	// Transform and project particle position.
	vec4 eye_position = modelview_matrix * vec4(position, 1);
#else
	// Pass color to fragment shader.
	gl_FrontColor = gl_Color;

	// Transform and project particle position.
	vec4 eye_position = modelview_matrix * gl_Vertex;
#endif

	gl_Position = projection_matrix * eye_position;

	// Compute sprite size.
	gl_PointSize = basePointSize * particle_radius / (eye_position.z * projection_matrix[2][3] + projection_matrix[3][3]);
}
