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

#pragma once


#include <ovito/core/Core.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

// The minimum OpenGL version required by Ovito:
#define OVITO_OPENGL_MINIMUM_VERSION_MAJOR 			2
#define OVITO_OPENGL_MINIMUM_VERSION_MINOR			1

// The standard OpenGL version used by Ovito:
#define OVITO_OPENGL_REQUESTED_VERSION_MAJOR 		3
#define OVITO_OPENGL_REQUESTED_VERSION_MINOR		2

// OpenGL debugging macro:
#ifdef OVITO_DEBUG
	#define OVITO_CHECK_OPENGL(renderer, cmd)						\
	{																\
		cmd;														\
		renderer->checkOpenGLErrorStatus(#cmd, __FILE__, __LINE__);	\
	}
    #define OVITO_REPORT_OPENGL_ERRORS(renderer) renderer->checkOpenGLErrorStatus("", __FILE__, __LINE__);
#else
	#define OVITO_CHECK_OPENGL(renderer, cmd) cmd
    #define OVITO_REPORT_OPENGL_ERRORS(renderer)
#endif

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
