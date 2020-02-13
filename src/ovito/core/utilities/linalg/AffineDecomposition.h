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
//  The matrix decomposition code has been taken from the book
//  Graphics Gems IV - Ken Shoemake, Polar AffineTransformation Decomposition.
//
///////////////////////////////////////////////////////////////////////////////

/**
 * \file
 * \brief Contains the definition of the Ovito::AffineDecomposition class.
 */

#pragma once


#include <ovito/core/Core.h>
#include "Vector3.h"
#include "Rotation.h"
#include "Scaling.h"
#include "AffineTransformation.h"

namespace Ovito {

/**
 * \brief Decomposes an affine transformation matrix into translation, rotation and scaling parts.
 *
 * A AffineTransformation matrix is decomposed in the following way:
 *
 * M = T * F * R * S
 *
 * with
 * \li T - Translation
 * \li F - Sign of determinant
 * \li R - Rotation
 * \li S - Scaling
 *
 * The scaling matrix is spectrally decomposed into S = U * K * U.transposed().
 *
 * \note Decomposing a matrix into its affine parts is a slow operation and should only be done when really necessary.
 */
class OVITO_CORE_EXPORT AffineDecomposition
{
public:

	/// Translation part.
	Vector3 translation;

	/// Rotation part.
	Quaternion rotation;

	/// Scaling part.
	Scaling scaling;

	/// Sign of determinant (either -1.0 or +1.0).
	FloatType sign;

	/// \brief Constructor that decomposes the given matrix into its affine parts.
	///
	/// After the constructor has been called the components of the decomposed
	/// transformation can be accessed through the #translation, #rotation,
	/// #scaling and #sign member variables.
	AffineDecomposition(const AffineTransformation& tm);

private:

	void decomp_affine(Matrix4& A);
};

}	// End of namespace


