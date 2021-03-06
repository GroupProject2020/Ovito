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

// OpenGL ES 2.0 has no built-in support for gl_FragDepth. 
// Need to request EXT_frag_depth extension in such a case.
#if __VERSION__ < 300 
	#extension GL_EXT_frag_depth : enable
	#define gl_FragDepth gl_FragDepthEXT
#endif

precision highp float; 

// Input from calling program:
uniform mat4 projection_matrix;
uniform mat4 inverse_projection_matrix;
uniform bool is_perspective;
uniform vec2 viewport_origin;		// Specifies the transformation from screen coordinates to viewport coordinates.
uniform vec2 inverse_viewport_size;	// Specifies the transformation from screen coordinates to viewport coordinates.

#if __VERSION__ >= 300 // OpenGL ES 3.0
	flat in vec4 particle_color_fs;
	flat in float particle_radius_squared_fs;
	flat in vec3 particle_view_pos_fs;
	out vec4 FragColor;
	#define gl_FragColor FragColor
#else // OpenGL ES 2.0
	varying vec4 particle_color_fs;
	varying float particle_radius_squared_fs;
	varying vec3 particle_view_pos_fs;
#endif

void main()
{
	// Calculate the pixel coordinate in viewport space.
	vec2 view_c = ((gl_FragCoord.xy - viewport_origin) * inverse_viewport_size) - 1.0;

	// Calculate viewing ray direction in view space
	vec3 ray_dir;
	vec3 ray_origin;
	if(is_perspective) {
		ray_dir = normalize(vec3(inverse_projection_matrix * vec4(view_c.x, view_c.y, 1.0, 1.0)));
		ray_origin = vec3(0.0);
	}
	else {
		ray_origin = vec3(inverse_projection_matrix * vec4(view_c.x, view_c.y, 0.0, 1.0));
		ray_dir = vec3(0.0, 0.0, -1.0);
	}

	vec3 sphere_dir = particle_view_pos_fs - ray_origin;

	// Perform ray-sphere intersection test.
	float b = dot(ray_dir, sphere_dir);
	float sphere_dir_sq = dot(sphere_dir, sphere_dir);
	vec3 delta = ray_dir * b - sphere_dir;
	float x = dot(delta, delta);
	float disc = particle_radius_squared_fs - x;

	// Only calculate the intersection closest to the viewer.
	if(disc <= 0.0)
		discard; // Ray missed sphere entirely, discard fragment

	// Calculate closest intersection position.
	float tnear = b - sqrt(disc);

	// Discard intersections behind the view point.
	if(is_perspective && tnear < 0.0)
		discard;

	// Calculate intersection point in view coordinate system.
	vec3 view_intersection_pnt = ray_origin + tnear * ray_dir;

	// Output the ray-sphere intersection point as the fragment depth
	// rather than the depth of the bounding box polygons.
	// The eye coordinate Z value must be transformed to normalized device
	// coordinates before being assigned as the final fragment depth.
#if __VERSION__ >= 300 || defined(GL_EXT_frag_depth)
	vec4 projected_intersection = projection_matrix * vec4(view_intersection_pnt, 1.0);
	gl_FragDepth = (projected_intersection.z / projected_intersection.w + 1.0) * 0.5;
#endif

	gl_FragColor = particle_color_fs;
}
