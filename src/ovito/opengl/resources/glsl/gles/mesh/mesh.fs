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

precision mediump float; 

varying vec4 vertex_color_fs;
varying vec3 vertex_normal_fs;

const float ambient = 0.4;
const float diffuse_strength = 0.6;
const float shininess = 6.0;
const vec3 specular_lightdir = normalize(vec3(1.8, -1.5, 0.2));

void main()
{
	float diffuse = abs(vertex_normal_fs.z) * diffuse_strength;
	float specular = pow(max(0.0, dot(reflect(specular_lightdir, vertex_normal_fs), vec3(0,0,1))), shininess) * 0.25;

	gl_FragColor = vec4(vertex_color_fs.rgb * (diffuse + ambient) + vec3(specular), vertex_color_fs.a);
}
