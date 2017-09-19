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

#include <core/Core.h>
#include <core/utilities/io/ObjectLoadStream.h>
#include <core/oo/OvitoClass.h>
#include "OvitoObject.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

#ifdef OVITO_DEBUG
/******************************************************************************
* Destructor.
******************************************************************************/
OvitoObject::~OvitoObject() 
{
	OVITO_CHECK_OBJECT_POINTER(this);
	OVITO_ASSERT_MSG(objectReferenceCount() == 0, "~OvitoObject()", "Destroying an object whose reference counter is non-zero.");
	_magicAliveCode = 0xFEDCBA87;
}
#endif

/******************************************************************************
* Returns true if this object is currently being loaded from an ObjectLoadStream.
******************************************************************************/
bool OvitoObject::isBeingLoaded() const
{
	return (qobject_cast<ObjectLoadStream*>(parent()) != nullptr);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
