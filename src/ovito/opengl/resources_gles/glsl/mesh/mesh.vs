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
uniform mat4 modelview_projection_matrix;
uniform mat3 normal_matrix;

#if __VERSION__ >= 300 // OpenGL ES 3.0

	// Vertex shader inputs:
	in vec3 position;
	in vec3 normal;
	in vec4 color;

	// Vertex shader outputs to fragment shader:
	out vec4 vertex_color_fs;
	out vec3 vertex_normal_fs;

#else // OpenGL ES 2.0:

	// Vertex shader inputs:
	attribute vec3 position;
	attribute vec3 normal;
	attribute vec4 color;

	// Vertex shader outputs to fragment shader:
	varying vec4 vertex_color_fs;
	varying vec3 vertex_normal_fs;

#endif

void main()
{
	vertex_color_fs = color;
	vertex_normal_fs = normalize(normal_matrix * normal);
	gl_Position = modelview_projection_matrix * vec4(position, 1.0);
}
