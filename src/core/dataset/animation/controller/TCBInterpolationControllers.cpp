///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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
#include <core/utilities/units/UnitsManager.h>
#include "TCBInterpolationControllers.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim)

IMPLEMENT_OVITO_CLASS_TEMPLATE(TCBAnimationKey<FloatAnimationKey>);
template<> DEFINE_PROPERTY_FIELD(TCBAnimationKey<FloatAnimationKey>, easeTo);
template<> DEFINE_PROPERTY_FIELD(TCBAnimationKey<FloatAnimationKey>, easeFrom);
template<> DEFINE_PROPERTY_FIELD(TCBAnimationKey<FloatAnimationKey>, tension);
template<> DEFINE_PROPERTY_FIELD(TCBAnimationKey<FloatAnimationKey>, continuity);
template<> DEFINE_PROPERTY_FIELD(TCBAnimationKey<FloatAnimationKey>, bias);

IMPLEMENT_OVITO_CLASS(FloatTCBAnimationKey);
SET_PROPERTY_FIELD_LABEL(FloatTCBAnimationKey, easeTo, "Ease to");
SET_PROPERTY_FIELD_LABEL(FloatTCBAnimationKey, easeFrom, "Ease from");
SET_PROPERTY_FIELD_LABEL(FloatTCBAnimationKey, tension, "Tension");
SET_PROPERTY_FIELD_LABEL(FloatTCBAnimationKey, continuity, "Continuity");
SET_PROPERTY_FIELD_LABEL(FloatTCBAnimationKey, bias, "Bias");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(FloatTCBAnimationKey, easeTo, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(FloatTCBAnimationKey, easeFrom, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(FloatTCBAnimationKey, tension, FloatParameterUnit, -1, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(FloatTCBAnimationKey, continuity, FloatParameterUnit, -1, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(FloatTCBAnimationKey, bias, FloatParameterUnit, -1, 1);

IMPLEMENT_OVITO_CLASS_TEMPLATE(TCBAnimationKey<PositionAnimationKey>);
template<> DEFINE_PROPERTY_FIELD(TCBAnimationKey<PositionAnimationKey>, easeTo);
template<> DEFINE_PROPERTY_FIELD(TCBAnimationKey<PositionAnimationKey>, easeFrom);
template<> DEFINE_PROPERTY_FIELD(TCBAnimationKey<PositionAnimationKey>, tension);
template<> DEFINE_PROPERTY_FIELD(TCBAnimationKey<PositionAnimationKey>, continuity);
template<> DEFINE_PROPERTY_FIELD(TCBAnimationKey<PositionAnimationKey>, bias);

IMPLEMENT_OVITO_CLASS(PositionTCBAnimationKey);
SET_PROPERTY_FIELD_LABEL(PositionTCBAnimationKey, easeTo, "Ease to");
SET_PROPERTY_FIELD_LABEL(PositionTCBAnimationKey, easeFrom, "Ease from");
SET_PROPERTY_FIELD_LABEL(PositionTCBAnimationKey, tension, "Tension");
SET_PROPERTY_FIELD_LABEL(PositionTCBAnimationKey, continuity, "Continuity");
SET_PROPERTY_FIELD_LABEL(PositionTCBAnimationKey, bias, "Bias");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(PositionTCBAnimationKey, easeTo, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(PositionTCBAnimationKey, easeFrom, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(PositionTCBAnimationKey, tension, FloatParameterUnit, -1, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(PositionTCBAnimationKey, continuity, FloatParameterUnit, -1, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(PositionTCBAnimationKey, bias, FloatParameterUnit, -1, 1);

IMPLEMENT_OVITO_CLASS(TCBPositionController);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
