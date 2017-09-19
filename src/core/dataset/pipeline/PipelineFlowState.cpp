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
#include <core/dataset/pipeline/PipelineFlowState.h>
#include <core/dataset/data/DataObject.h>
#include <core/oo/CloneHelper.h>

#include <boost/optional.hpp>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/******************************************************************************
* Tries to convert one of the to data objects stored in this flow state to
* the given object type.
******************************************************************************/
OORef<DataObject> PipelineFlowState::convertObject(const OvitoClass& objectClass, TimePoint time) const
{
	for(DataObject* o : objects()) {
		if(OORef<DataObject> obj = o->convertTo(objectClass, time))
			return obj;
	}
	return {};
}

/******************************************************************************
* Returns true if the given object is part of this pipeline flow state.
* The method ignores the revision number of the object.
******************************************************************************/
bool PipelineFlowState::contains(DataObject* obj) const
{
	for(DataObject* o : objects())
		if(o == obj) return true;
	return false;
}

/******************************************************************************
* Adds an additional data object to this state.
******************************************************************************/
void PipelineFlowState::addObject(DataObject* obj)
{
	OVITO_CHECK_OBJECT_POINTER(obj);
	OVITO_ASSERT_MSG(!contains(obj), "PipelineFlowState::addObject", "Cannot add the same data object more than once.");
	_objects.emplace_back(obj);
}

/******************************************************************************
* Inserts an additional data object into this state.
******************************************************************************/
void PipelineFlowState::insertObject(int index, DataObject* obj)
{
	OVITO_CHECK_OBJECT_POINTER(obj);
	OVITO_ASSERT_MSG(!contains(obj), "PipelineFlowState::insertObject", "Cannot insert the same data object more than once.");
	OVITO_ASSERT(index >= 0 && index <= _objects.size());
	_objects.emplace(_objects.begin() + index, obj);
}

/******************************************************************************
* Replaces a data object with a new one.
******************************************************************************/
void PipelineFlowState::removeObjectByIndex(int index)
{
	OVITO_ASSERT(index >= 0 && index < _objects.size());
	_objects.erase(_objects.begin() + index);
}

/******************************************************************************
* Replaces a data object with a new one.
******************************************************************************/
bool PipelineFlowState::replaceObject(DataObject* oldObj, DataObject* newObj)
{
	OVITO_CHECK_OBJECT_POINTER(oldObj);
	for(auto iter = _objects.begin(); iter != _objects.end(); ++iter) {
		if(iter->get() == oldObj) {
			if(newObj)
				*iter = newObj;
			else
				_objects.erase(iter);
			return true;
		}
	}
	OVITO_ASSERT_MSG(false, "PipelineFlowState::replaceObject", "Old data object not found.");
	return false;
}

/******************************************************************************
* Replaces objects with copies if there are multiple references.
* After calling this method, none of the objects in the flow state is referenced by anybody else.
* Thus, it becomes safe to modify the data objects.
******************************************************************************/
void PipelineFlowState::cloneObjectsIfNeeded(bool deepCopy)
{
	boost::optional<CloneHelper> cloneHelper;
	for(auto& ref : _objects) {
		OVITO_ASSERT(ref->numberOfStrongReferences() >= 1);
		if(ref->numberOfStrongReferences() > 1) {
			if(!cloneHelper) cloneHelper.emplace();
			ref = cloneHelper->cloneObject(ref.get(), deepCopy);
		}
		OVITO_ASSERT(ref->numberOfStrongReferences() == 1);
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
