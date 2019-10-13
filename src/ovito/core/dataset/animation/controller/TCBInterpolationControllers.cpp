////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
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
#include <ovito/core/utilities/units/UnitsManager.h>
#include "TCBInterpolationControllers.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim)

IMPLEMENT_OVITO_CLASS_TEMPLATE(TCBAnimationKey<FloatAnimationKey>);
template<> OVITO_CORE_EXPORT DEFINE_PROPERTY_FIELD(TCBAnimationKey<FloatAnimationKey>, easeTo);
template<> OVITO_CORE_EXPORT DEFINE_PROPERTY_FIELD(TCBAnimationKey<FloatAnimationKey>, easeFrom);
template<> OVITO_CORE_EXPORT DEFINE_PROPERTY_FIELD(TCBAnimationKey<FloatAnimationKey>, tension);
template<> OVITO_CORE_EXPORT DEFINE_PROPERTY_FIELD(TCBAnimationKey<FloatAnimationKey>, continuity);
template<> OVITO_CORE_EXPORT DEFINE_PROPERTY_FIELD(TCBAnimationKey<FloatAnimationKey>, bias);

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
template<> OVITO_CORE_EXPORT DEFINE_PROPERTY_FIELD(TCBAnimationKey<PositionAnimationKey>, easeTo);
template<> OVITO_CORE_EXPORT DEFINE_PROPERTY_FIELD(TCBAnimationKey<PositionAnimationKey>, easeFrom);
template<> OVITO_CORE_EXPORT DEFINE_PROPERTY_FIELD(TCBAnimationKey<PositionAnimationKey>, tension);
template<> OVITO_CORE_EXPORT DEFINE_PROPERTY_FIELD(TCBAnimationKey<PositionAnimationKey>, continuity);
template<> OVITO_CORE_EXPORT DEFINE_PROPERTY_FIELD(TCBAnimationKey<PositionAnimationKey>, bias);

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
