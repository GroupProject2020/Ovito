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

uniform sampler2D tex;			// The imposter texture.

#if __VERSION__ >= 130

	flat in vec4 particle_color_fs;
	in vec2 texcoords;
	out vec4 FragColor;

#else

	#define particle_color_fs gl_Color
	#define FragColor gl_FragColor
	#define texture texture2D
	#define texcoords gl_TexCoord[0].xy

#endif

void main()
{
	vec2 shifted_coords = texcoords - vec2(0.5, 0.5);
	if(dot(shifted_coords, shifted_coords) >= 0.25) discard;
	vec4 texValue = texture(tex, texcoords);

	// Specular highlights are stored in the green channel of the texture.
	// Modulate diffuse color with brightness value stored in the red channel of the texture.
	FragColor = vec4(texValue.r * particle_color_fs.rgb + texValue.g, particle_color_fs.a);
}
