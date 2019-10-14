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
layout(triangle_fan, max_vertices=7) out;

// Inputs from calling program:
uniform mat4 modelview_projection_matrix;
uniform bool is_perspective;
uniform vec3 parallel_view_dir;
uniform vec3 eye_pos;

// Inputs from vertex shader
in vec4 color_gs[1];
in vec3 cylinder_axis_gs[1];
in float cylinder_radius_gs[1];

// Outputs to fragment shader
flat out vec4 vertex_color_out;

void main()
{

	if(cylinder_axis_gs[0] != vec3(0)) {
		// Get view direction.
		vec3 view_dir;
		if(!is_perspective)
			view_dir = parallel_view_dir;
		else
			view_dir = eye_pos - gl_in[0].gl_Position.xyz;

		// Build local coordinate system.
		vec3 u = normalize(cross(view_dir, cylinder_axis_gs[0]));
		vec3 v = cylinder_axis_gs[0];

		gl_Position = modelview_projection_matrix * vec4(v * position.x + u * position.y + gl_in[0].gl_Position.xyz, 1.0);
		vertex_color_out = color_gs[0];
		EmitVertex();
	}
}
