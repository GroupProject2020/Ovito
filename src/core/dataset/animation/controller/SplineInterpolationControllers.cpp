///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
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

#include <core/Core.h>
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
