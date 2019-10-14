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

// Input from calling program:
uniform mat4 projection_matrix;
uniform sampler2D tex;			// The imposter texture.

#if __VERSION__ >= 130

	// Input from vertex shader:
	flat in vec4 particle_color_fs;
	flat in float particle_radius_fs;	// The particle radius.
	flat in float ze0;					// The particle's Z coordinate in eye coordinates.
	in vec2 texcoords;

	out vec4 FragColor;

#else

	#define particle_radius_fs gl_TexCoord[1].x
	#define ze0 gl_TexCoord[1].y

	#define particle_color_fs gl_Color
	#define FragColor gl_FragColor
	#define texture texture2D
	#define texcoords gl_TexCoord[0].xy

#endif

void main()
{
	vec2 shifted_coords = texcoords - vec2(0.5, 0.5);
	float rsq = dot(shifted_coords, shifted_coords);
	if(rsq >= 0.25) discard;
	vec4 texValue = texture(tex, texcoords);

	// Specular highlights are stored in the green channel of the texture.
	// Modulate diffuse color with brightness value stored in the red channel of the texture.
	FragColor = vec4(texValue.r * particle_color_fs.rgb + texValue.g, particle_color_fs.a);

	// Vary the depth value across the imposter to obtain proper intersections between particles.
	float dz = sqrt(1.0 - 4.0 * rsq) * particle_radius_fs;
	float ze = ze0 + dz;
	float zn = (projection_matrix[2][2] * ze + projection_matrix[3][2]) / (projection_matrix[2][3] * ze + projection_matrix[3][3]);
	gl_FragDepth = 0.5 * (zn * gl_DepthRange.diff + (gl_DepthRange.far + gl_DepthRange.near));
}

