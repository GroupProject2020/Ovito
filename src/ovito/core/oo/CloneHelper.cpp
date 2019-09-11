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
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/dataset/DataSet.h>
#include "CloneHelper.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/******************************************************************************
* This creates a copy of the RefTarget.
* If there has already a copy of this RefTarget
* created by this clone helper earlier, then it will be returned.
*    obj - The object to be cloned.
*    deepCopy - Controls whether sub-objects referenced by this RefTarget are copied too.
* Returns the copy of the source object.
******************************************************************************/
RefTarget* CloneHelper::cloneObjectImpl(const RefTarget* obj, bool deepCopy)
{
	if(obj == nullptr) return nullptr;
	OVITO_CHECK_OBJECT_POINTER(obj);

	for(const auto& entry : _cloneTable) {
		if(entry.first == obj)
			return entry.second;
	}

	OORef<RefTarget> copy = obj->clone(deepCopy, *this);
	if(!copy)
		obj->throwException(QString("Object of class %1 cannot be cloned. It does not implement the clone() method.").arg(obj->getOOClass().name()));

	OVITO_ASSERT_MSG(copy->getOOClass().isDerivedFrom(obj->getOOClass()), "CloneHelper::cloneObject", qPrintable(QString("The clone method of class %1 did not return a compatible class instance.").arg(obj->getOOClass().name())));

	_cloneTable.push_back(std::make_pair(obj, copy));
	return copy;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
