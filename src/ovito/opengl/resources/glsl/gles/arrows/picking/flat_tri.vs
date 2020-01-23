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

uniform mat4 modelview_projection_matrix;
uniform bool is_perspective;
uniform vec3 parallel_view_dir;
uniform vec3 eye_pos;
uniform float pickingBaseID;
uniform float verticesPerElement;

attribute vec3 position;
attribute float vertexID;

attribute vec3 cylinder_base;
attribute vec3 cylinder_axis;

varying vec4 vertex_color_out;

void main()
{
	// Compute color from object ID.
	float objectID = pickingBaseID + floor(vertexID / verticesPerElement);
	vertex_color_out = vec4(
		floor(mod(objectID, 256.0)) / 255.0,
		floor(mod(objectID / 256.0, 256.0)) / 255.0,
		floor(mod(objectID / 65536.0, 256.0)) / 255.0,
		floor(mod(objectID / 16777216.0, 256.0)) / 255.0);

	if(cylinder_axis != vec3(0)) {

		// Get view direction.
		vec3 view_dir;
		if(!is_perspective)
			view_dir = parallel_view_dir;
		else
			view_dir = eye_pos - cylinder_base;

		// Build local coordinate system.
		vec3 u = normalize(cross(view_dir, cylinder_axis));
		vec3 rotated_pos = cylinder_axis * position.x + u * position.y + cylinder_base;
		gl_Position = modelview_projection_matrix * vec4(rotated_pos, 1.0);
	}
	else {
		gl_Position = vec4(0);
	}
}
