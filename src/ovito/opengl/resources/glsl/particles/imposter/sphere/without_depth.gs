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

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

// Inputs from calling program:
uniform mat4 projection_matrix;

// Inputs from vertex shader
in vec4 particle_color_gs[1];
in float particle_radius_gs[1];

// Outputs to fragment shader
flat out vec4 particle_color_fs;
out vec2 texcoords;

void main()
{
	float psizeX = particle_radius_gs[0] * projection_matrix[0][0];
	float psizeY = particle_radius_gs[0] * projection_matrix[1][1];

	particle_color_fs = particle_color_gs[0];
	texcoords = vec2(1,1);
	gl_Position = gl_in[0].gl_Position + vec4(psizeX, -psizeY, 0.0, 0.0);
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	texcoords = vec2(1,0);
	gl_Position = gl_in[0].gl_Position + vec4(psizeX, psizeY, 0.0, 0.0);
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	texcoords = vec2(0,1);
	gl_Position = gl_in[0].gl_Position + vec4(-psizeX, -psizeY, 0.0, 0.0);
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	texcoords = vec2(0,0);
	gl_Position = gl_in[0].gl_Position + vec4(-psizeX, psizeY, 0.0, 0.0);
	EmitVertex();
}
