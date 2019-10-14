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
uniform mat4 modelviewprojection_matrix;
uniform mat4 modelview_matrix;
uniform float radius_scalingfactor;

#if __VERSION__ >= 130

// The particle data:
in vec3 position;
in vec4 color;
in float particle_radius;

// Output to geometry shader.
out vec4 particle_color_gs;
out float particle_radius_gs;
out float particle_ze0_gs;

#endif

void main()
{
#if __VERSION__ >= 130
	particle_color_gs = color;
	particle_radius_gs = particle_radius * radius_scalingfactor;
	particle_ze0_gs = modelview_matrix[0][2] * position.x + modelview_matrix[1][2] * position.y + modelview_matrix[2][2] * position.z + modelview_matrix[3][2];
	gl_Position = modelviewprojection_matrix * vec4(position, 1.0);
#endif
}
