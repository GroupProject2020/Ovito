///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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

#include <ovito/core/Core.h>
#include "ConstantControllers.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim)

IMPLEMENT_OVITO_CLASS(ConstFloatController);
IMPLEMENT_OVITO_CLASS(ConstIntegerController);
IMPLEMENT_OVITO_CLASS(ConstVectorController);
IMPLEMENT_OVITO_CLASS(ConstPositionController);
IMPLEMENT_OVITO_CLASS(ConstRotationController);
IMPLEMENT_OVITO_CLASS(ConstScalingController);
DEFINE_PROPERTY_FIELD(ConstFloatController, value);
DEFINE_PROPERTY_FIELD(ConstIntegerController, value);
DEFINE_PROPERTY_FIELD(ConstVectorController, value);
DEFINE_PROPERTY_FIELD(ConstPositionController, value);
DEFINE_PROPERTY_FIELD(ConstRotationController, value);
DEFINE_PROPERTY_FIELD(ConstScalingController, value);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
