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

#if __VERSION__ >= 130

// The particle data:
in vec3 position;
in vec4 color;
in vec3 shape;
in vec4 orientation;
in float particle_radius;

// Output to geometry shader.
out vec4 particle_color_gs;
out vec3 particle_shape_gs;
out vec4 particle_orientation_gs;

#endif

void main()
{
#if __VERSION__ >= 130
	// Forward information to geometry shader.
	particle_color_gs = color;
	if(shape != vec3(0))
		particle_shape_gs = shape;
	else
		particle_shape_gs = vec3(particle_radius, particle_radius, particle_radius);
	particle_orientation_gs = orientation;

	// Pass original particle position to geometry shader.
	gl_Position = vec4(position, 1);
#endif
}
