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
uniform mat4 projection_matrix;
uniform mat4 inverse_projection_matrix;
uniform bool is_perspective;
uniform vec2 viewport_origin;		// Specifies the transformation from screen coordinates to viewport coordinates.
uniform vec2 inverse_viewport_size;	// Specifies the transformation from screen coordinates to viewport coordinates.

// Inputs from vertex shader:
#if __VERSION__ >= 300 // OpenGL ES 3.0
	flat in vec4 particle_color_fs;
	flat in vec3 surface_normal_fs;
	out vec4 FragColor;
	#define gl_FragColor FragColor
#else
	varying vec4 particle_color_fs;
	varying vec3 surface_normal_fs;
#endif

// Constants:
const float ambient = 0.4;
const float diffuse_strength = 1.0 - ambient;
const float shininess = 6.0;
const vec3 specular_lightdir = normalize(vec3(-1.8, 1.5, -0.2));

void main()
{
	// Calculate viewing ray direction in view space
	vec3 ray_dir;
	if(is_perspective) {
		// Calculate the pixel coordinate in viewport space.
		vec2 view_c = ((gl_FragCoord.xy - viewport_origin) * inverse_viewport_size) - 1.0;
		ray_dir = normalize(vec3(inverse_projection_matrix * vec4(view_c.x, view_c.y, 1.0, 1.0)));
	}
	else {
		ray_dir = vec3(0.0, 0.0, -1.0);
	}

	float diffuse = abs(surface_normal_fs.z) * diffuse_strength;
	float specular = pow(max(0.0, dot(reflect(specular_lightdir, surface_normal_fs), ray_dir)), shininess) * 0.25;
	gl_FragColor = vec4(particle_color_fs.rgb * (diffuse + ambient) + vec3(specular), particle_color_fs.a);
}
