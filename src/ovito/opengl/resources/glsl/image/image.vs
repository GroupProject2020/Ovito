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

uniform vec2 uvcoords[4];

#if __VERSION__ >= 130

in vec3 position;
out vec2 tex_coords;

void main()
{
	gl_Position = vec4(positions.xy, 0, 1);
	tex_coords = uvcoords[positions.z];
}

#else

void main()
{
	gl_Position = vec4(gl_Vertex.xy, 0, 1);
	gl_TexCoord[0] = uvcoords[gl_Vertex.z];
}

#endif
