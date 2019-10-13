////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Alexander Stukowski
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

//
// Standard precompiled header file included by all source files in this module
//

#ifndef __OVITO_PYSCRIPT_
#define __OVITO_PYSCRIPT_

#include <ovito/core/Core.h>

// Qt defines the 'slots' and 'signals' keyword macros. They are in conflict with identifiers used in the Python headers.
#ifdef slots
	#undef slots
#endif
#ifdef signals
	#undef signals
#endif

// Include pybind11, which is located in our 3rd party source directory.
#include <3rdparty/pybind11/pybind11.h>
#include <3rdparty/pybind11/eval.h>
#include <3rdparty/pybind11/operators.h>
#include <3rdparty/pybind11/stl_bind.h>
#include <3rdparty/pybind11/stl.h>
#include <3rdparty/pybind11/numpy.h>
#include <3rdparty/pybind11/embed.h>

#endif
