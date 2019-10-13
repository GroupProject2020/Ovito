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

/**
 * \file
 * \brief This header file defines the default floating-point type and numeric constants used throughout the program.
 */

#pragma once


namespace Ovito {

#ifdef FLOATTYPE_FLOAT

	/// The default floating-point type used by OVITO.
	using FloatType = float;

	/// A small epsilon, which is used in OVITO to test if a number is (almost) zero.
	#define FLOATTYPE_EPSILON	Ovito::FloatType(1e-6)

	/// The format specifier to be passed to the sscanf() function to parse floating-point numbers of type Ovito::FloatType.
	#define FLOATTYPE_SCANF_STRING 		"%g"

#else

	/// The default floating-point type used by OVITO.
	using FloatType = double;

	/// A small epsilon, which is used in OVITO to test if a number is (almost) zero.
	#define FLOATTYPE_EPSILON	Ovito::FloatType(1e-12)

	/// The format specifier to be passed to the sscanf() function to parse floating-point numbers of type Ovito::FloatType.
	#define FLOATTYPE_SCANF_STRING 		"%lg"

#endif

/// The maximum value for floating-point variables of type Ovito::FloatType.
#define FLOATTYPE_MAX	(std::numeric_limits<Ovito::FloatType>::max())

/// The lowest value for floating-point variables of type Ovito::FloatType.
#define FLOATTYPE_MIN	(std::numeric_limits<Ovito::FloatType>::lowest())

/// The constant PI.
#define FLOATTYPE_PI	Ovito::FloatType(3.14159265358979323846)


}	// End of namespace


