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
#include <ovito/core/utilities/io/ObjectLoadStream.h>
#include <ovito/core/oo/OvitoClass.h>
#include "OvitoObject.h"

namespace Ovito {

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

}	// End of namespace
