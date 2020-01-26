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

#extension GL_EXT_frag_depth : enable
precision highp float; 

// Input from calling program:
uniform mat4 projection_matrix;
uniform mat4 inverse_projection_matrix;
uniform bool is_perspective;
uniform vec2 viewport_origin;		// Specifies the transformation from screen coordinates to viewport coordinates.
uniform vec2 inverse_viewport_size;	// Specifies the transformation from screen coordinates to viewport coordinates.

varying vec4 particle_color_fs;
varying mat3 particle_quadric_fs;
varying vec3 particle_view_pos_fs;

const float ambient = 0.4;
const float diffuse_strength = 1.0 - ambient;
const float shininess = 6.0;
const vec3 specular_lightdir = normalize(vec3(-1.8, 1.5, -0.2));

void main()
{
	// Calculate the pixel position in viewport coordinates.
	vec2 view_c = ((gl_FragCoord.xy - viewport_origin) * inverse_viewport_size) - 1.0;

	// Calculate viewing ray direction in view space.
	vec3 ray_dir;
	vec3 ray_origin;
	if(is_perspective) {
		ray_dir = normalize(vec3(inverse_projection_matrix * vec4(view_c.x, view_c.y, 1.0, 1.0)));
		ray_origin = vec3(0.0);
	}
	else {
		ray_origin = vec3(inverse_projection_matrix * vec4(view_c.x, view_c.y, 0.0, 1.0));
		// This is to improve numeric precision of intersection calculation:
		ray_origin.z = particle_view_pos_fs.z;
		ray_dir = vec3(0.0, 0.0, -1.0);
	}

	float a = particle_quadric_fs[0][0];
	float b = particle_quadric_fs[1][0];
	float c = particle_quadric_fs[2][0];
	float d = 0.0;
	float e = particle_quadric_fs[1][1];
	float f = particle_quadric_fs[2][1];
	float g = 0.0;
	float h = particle_quadric_fs[2][2];
	float i = 0.0;
	float j = -1.0;

	vec3 ro = ray_origin - particle_view_pos_fs;
	vec3 rd = ray_dir;

	float Aq = (a*(rd.x * rd.x)) +
        (2.0 * b * rd.x * rd.y) +
        (2.0 * c * rd.x * rd.z) +
        (e * (rd.y * rd.y)) +
        (2.0 * f * rd.y * rd.z) +
        (h * (rd.z * rd.z));

	float Bq = 2.0 * (
        (a * ro.x * rd.x) +
        (b * ((ro.x * rd.y) + (rd.x * ro.y))) +
        (c * ((ro.x * rd.z) + (rd.x * ro.z))) +
        (d * rd.x) +
        (e * ro.y * rd.y) +
        (f * ((ro.y * rd.z) + (rd.y * ro.z))) +
        (g * rd.y) +
        (h * ro.z * rd.z) +
        (i * rd.z));

	float Cq = (a * (ro.x * ro.x)) +
        (2.0 * b * ro.x * ro.y) +
        (2.0 * c * ro.x * ro.z) +
        (2.0 * d * ro.x) +
        (e * (ro.y * ro.y)) +
        (2.0 * f * ro.y * ro.z) +
        (2.0 * g * ro.y) +
        (h * (ro.z * ro.z)) +
        (2.0 * i * ro.z) + j;

	float tnear;
	if(Aq == 0.0) {
		tnear = -Cq / Bq;
	}
	else {
		float disc = (Bq*Bq - 4.0 * Aq * Cq);
		if(disc <= 0.0)
			discard;
		disc = sqrt(disc);
		float t1 = (-Bq + disc) / (2.0 * Aq);
		float t2 = (-Bq - disc) / (2.0 * Aq);
		tnear = min(t1, t2);
	}

	// Discard intersections behind the view point.
	if(is_perspective && tnear < 0.0)
		discard;

	// Calculate intersection point in view coordinate system.
	vec3 view_intersection_pnt = ray_origin + tnear * ray_dir;

	// Output the ray-sphere intersection point as the fragment depth
	// rather than the depth of the bounding box polygons.
	// The eye coordinate Z value must be transformed to normalized device
	// coordinates before being assigned as the final fragment depth.
	vec4 projected_intersection = projection_matrix * vec4(view_intersection_pnt, 1.0);
	gl_FragDepthEXT = ((gl_DepthRange.diff * (projected_intersection.z / projected_intersection.w)) + gl_DepthRange.near + gl_DepthRange.far) * 0.5;

	// Calculate surface normal in view space.
	vec3 r = view_intersection_pnt - particle_view_pos_fs;
	vec3 surface_normal = normalize(vec3(
			a*r.x + b*r.y + c*r.z + d,
			b*r.x + e*r.y + f*r.z + g,
			c*r.x + f*r.y + h*r.z + i));

	float diffuse = abs(surface_normal.z) * diffuse_strength;
	float specular = pow(max(0.0, dot(reflect(specular_lightdir, surface_normal), ray_dir)), shininess) * 0.25;
	gl_FragColor = vec4(particle_color_fs.rgb * (diffuse + ambient) + vec3(specular), particle_color_fs.a);
}
