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

precision mediump float;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform bool is_perspective;
uniform float line_width;

attribute vec3 position;
attribute vec4 color;
attribute vec3 vector;

varying vec4 vertex_color_fs;

void main()
{
	vertex_color_fs = color;

	vec4 view_position = modelview_matrix * vec4(position, 1.0);
	vec3 view_dir;
	if(is_perspective)
		view_dir = view_position.xyz;
	else
		view_dir = vec3(0.0, 0.0, -1.0);
	vec3 u = cross(view_dir, (modelview_matrix * vec4(vector, 0.0)).xyz);
	if(u != vec3(0)) {
		float w = projection_matrix[0][3] * view_position.x + projection_matrix[1][3] * view_position.y
			+ projection_matrix[2][3] * view_position.z + projection_matrix[3][3];
		gl_Position = projection_matrix * (view_position - vec4((w * line_width / length(u)) * u, 0.0));
	}
	else {
		gl_Position = vec4(0);
	}
}
