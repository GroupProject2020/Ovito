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


namespace Ovito {

/******************************************************************************
* This macro performs a runtime-time assertion check.
******************************************************************************/
#ifdef OVITO_DEBUG
#define OVITO_ASSERT(condition) Q_ASSERT(condition)
#else
#define OVITO_ASSERT(condition)
#endif

/******************************************************************************
* This macro performs a runtime-time assertion check.
******************************************************************************/
#ifdef OVITO_DEBUG
#define OVITO_ASSERT_MSG(condition, where, what) Q_ASSERT_X(condition, where, what)
#else
#define OVITO_ASSERT_MSG(condition, where, what)
#endif

/******************************************************************************
* This macro performs a compile-time check.
******************************************************************************/
#define OVITO_STATIC_ASSERT(condition) Q_STATIC_ASSERT(condition)

/******************************************************************************
* This macro validates a memory pointer in debug mode.
* If the given pointer does not point to a valid position in memory then
* the debugger is activated.
******************************************************************************/
#define OVITO_CHECK_POINTER(pointer) OVITO_ASSERT_MSG((pointer), "OVITO_CHECK_POINTER", "Invalid object pointer.");

}	// End of namespace


