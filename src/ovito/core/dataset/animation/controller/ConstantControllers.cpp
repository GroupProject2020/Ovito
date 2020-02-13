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
#include "ConstantControllers.h"

namespace Ovito {

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

}	// End of namespace
