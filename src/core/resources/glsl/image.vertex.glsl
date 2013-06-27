///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

uniform vec2 corners[4];

const vec2 uvcoords[4] = vec2[4]( 
	vec2(0,0),
	vec2(1,0),
	vec2(0,1),
	vec2(1,1) 
);

out vec2 tex_coords;

void main()
{
	gl_Position = vec4(corners[gl_VertexID], 0, 1.0);
	tex_coords = uvcoords[gl_VertexID];
}