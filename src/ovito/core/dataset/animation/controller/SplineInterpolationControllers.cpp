////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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

#include <ovito/core/Core.h>
#include "SplineInterpolationControllers.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim)

IMPLEMENT_OVITO_CLASS_TEMPLATE(SplineAnimationKey<FloatAnimationKey>);
template<> OVITO_CORE_EXPORT DEFINE_PROPERTY_FIELD(SplineAnimationKey<FloatAnimationKey>, inTangent);
template<> OVITO_CORE_EXPORT DEFINE_PROPERTY_FIELD(SplineAnimationKey<FloatAnimationKey>, outTangent);
IMPLEMENT_OVITO_CLASS(FloatSplineAnimationKey);
SET_PROPERTY_FIELD_LABEL(FloatSplineAnimationKey, inTangent, "In Tangent");
SET_PROPERTY_FIELD_LABEL(FloatSplineAnimationKey, outTangent, "Out Tangent");

IMPLEMENT_OVITO_CLASS_TEMPLATE(SplineAnimationKey<PositionAnimationKey>);
template<> OVITO_CORE_EXPORT DEFINE_PROPERTY_FIELD(SplineAnimationKey<PositionAnimationKey>, inTangent);
template<> OVITO_CORE_EXPORT DEFINE_PROPERTY_FIELD(SplineAnimationKey<PositionAnimationKey>, outTangent);
IMPLEMENT_OVITO_CLASS(PositionSplineAnimationKey);
SET_PROPERTY_FIELD_LABEL(PositionSplineAnimationKey, inTangent, "In Tangent");
SET_PROPERTY_FIELD_LABEL(PositionSplineAnimationKey, outTangent, "Out Tangent");
IMPLEMENT_OVITO_CLASS(SplinePositionController);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
