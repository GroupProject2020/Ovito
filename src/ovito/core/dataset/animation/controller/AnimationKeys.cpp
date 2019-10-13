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

#include <ovito/core/Core.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "AnimationKeys.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim)

IMPLEMENT_OVITO_CLASS(AnimationKey);
IMPLEMENT_OVITO_CLASS(FloatAnimationKey);
IMPLEMENT_OVITO_CLASS(IntegerAnimationKey);
IMPLEMENT_OVITO_CLASS(Vector3AnimationKey);
IMPLEMENT_OVITO_CLASS(PositionAnimationKey);
IMPLEMENT_OVITO_CLASS(RotationAnimationKey);
IMPLEMENT_OVITO_CLASS(ScalingAnimationKey);
DEFINE_PROPERTY_FIELD(AnimationKey, time);
DEFINE_PROPERTY_FIELD(FloatAnimationKey, value);
DEFINE_PROPERTY_FIELD(IntegerAnimationKey, value);
DEFINE_PROPERTY_FIELD(Vector3AnimationKey, value);
DEFINE_PROPERTY_FIELD(PositionAnimationKey, value);
DEFINE_PROPERTY_FIELD(RotationAnimationKey, value);
DEFINE_PROPERTY_FIELD(ScalingAnimationKey, value);
SET_PROPERTY_FIELD_LABEL(AnimationKey, time, "Time");
SET_PROPERTY_FIELD_UNITS(AnimationKey, time, TimeParameterUnit);
SET_PROPERTY_FIELD_LABEL(FloatAnimationKey, value, "Value");
SET_PROPERTY_FIELD_LABEL(IntegerAnimationKey, value, "Value");
SET_PROPERTY_FIELD_LABEL(Vector3AnimationKey, value, "Value");
SET_PROPERTY_FIELD_LABEL(PositionAnimationKey, value, "Value");
SET_PROPERTY_FIELD_LABEL(RotationAnimationKey, value, "Value");
SET_PROPERTY_FIELD_LABEL(ScalingAnimationKey, value, "Value");

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
