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
layout(triangle_strip, max_vertices=24) out;

// Inputs from calling program:
uniform mat4 projection_matrix;
uniform float radius_scalingfactor;

// Inputs from vertex shader
in vec4 particle_color_gs[1];
in float particle_radius_gs[1];

// Outputs to fragment shader
flat out vec4 particle_color_fs;
flat out float particle_radius_squared_fs;
flat out vec3 particle_view_pos_fs;

void main()
{
	float radius = radius_scalingfactor * particle_radius_gs[0];
	float rsq = radius * radius;

#if 0
	// This code leads, which generates a single triangle strip for the cube, seems to be
	// incompatible with the Intel graphics driver on Linux.

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = projection_matrix *
		(gl_in[0].gl_Position + vec4(particle_radius_gs[0], particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = projection_matrix *
		(gl_in[0].gl_Position + vec4(particle_radius_gs[0], -particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = projection_matrix *
		(gl_in[0].gl_Position + vec4(particle_radius_gs[0], particle_radius_gs[0], -particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = projection_matrix *
		(gl_in[0].gl_Position + vec4(particle_radius_gs[0], -particle_radius_gs[0], -particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = projection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_radius_gs[0], -particle_radius_gs[0], -particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = projection_matrix *
		(gl_in[0].gl_Position + vec4(particle_radius_gs[0], -particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = projection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_radius_gs[0], -particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = projection_matrix *
		(gl_in[0].gl_Position + vec4(particle_radius_gs[0], particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = projection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_radius_gs[0], particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = projection_matrix *
		(gl_in[0].gl_Position + vec4(particle_radius_gs[0], particle_radius_gs[0], -particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = projection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_radius_gs[0], particle_radius_gs[0], -particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = projection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_radius_gs[0], -particle_radius_gs[0], -particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = projection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_radius_gs[0], particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = projection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_radius_gs[0], -particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

#else

	// Generate 6 triangle strips to be compatible with the Intel graphics driver on Linux.

	vec4 corner = projection_matrix * (gl_in[0].gl_Position - vec4(radius, radius, radius, 0));
	vec4 dx = projection_matrix * vec4(2 * radius, 0, 0, 0);
	vec4 dy = projection_matrix * vec4(0, 2 * radius, 0, 0);
	vec4 dz = projection_matrix * vec4(0, 0, 2 * radius, 0);

	// -X
	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dy + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dz;
	EmitVertex();
	EndPrimitive();

	// +X
	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dx;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dx + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dx + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dx + dy + dz;
	EmitVertex();
	EndPrimitive();

	// -Y
	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dx;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dx + dz;
	EmitVertex();
	EndPrimitive();

	// +Y
	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dy + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dx + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dy + dx + dz;
	EmitVertex();
	EndPrimitive();

	// -Z
	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dx + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dx;
	EmitVertex();
	EndPrimitive();

	// +Z
	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dx + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dy + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_radius_squared_fs = rsq;
	particle_view_pos_fs = gl_in[0].gl_Position.xyz;
	gl_Position = corner + dx + dy + dz;
	EmitVertex();
	EndPrimitive();

#endif
}
