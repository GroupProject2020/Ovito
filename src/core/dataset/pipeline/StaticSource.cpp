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
#include <core/dataset/pipeline/StaticSource.h>
#include <core/utilities/concurrent/SharedFuture.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(StaticSource);
DEFINE_REFERENCE_FIELD(StaticSource, dataObjects);
SET_PROPERTY_FIELD_LABEL(StaticSource, dataObjects, "Objects");

/******************************************************************************
* Constructor.
******************************************************************************/
StaticSource::StaticSource(DataSet* dataset) : PipelineObject(dataset)
{
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
SharedFuture<PipelineFlowState> StaticSource::evaluate(TimePoint time)
{
    TimeInterval iv = TimeInterval::infinite();
    for(DataObject* obj : dataObjects())
        iv.intersect(obj->objectValidity(time));
    return Future<PipelineFlowState>::createImmediateEmplace(PipelineStatus::Success, dataObjects(), iv, attributes());
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
PipelineFlowState StaticSource::evaluatePreliminary()
{
    return PipelineFlowState(PipelineStatus::Success, dataObjects(), TimeInterval::infinite(), attributes());
}

/******************************************************************************
* Returns the number of sub-objects that should be displayed in the modifier stack.
******************************************************************************/
int StaticSource::editableSubObjectCount()
{
	return dataObjects().size();
}

/******************************************************************************
* Returns a sub-object that should be listed in the modifier stack.
******************************************************************************/
RefTarget* StaticSource::editableSubObject(int index)
{
	return dataObjects()[index];
}

/******************************************************************************
* Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
******************************************************************************/
void StaticSource::referenceInserted(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(dataObjects))
		notifyDependents(ReferenceEvent::SubobjectListChanged);

    PipelineObject::referenceInserted(field, newTarget, listIndex);
}

/******************************************************************************
* Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
******************************************************************************/
void StaticSource::referenceRemoved(const PropertyFieldDescriptor& field, RefTarget* oldTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(dataObjects))
		notifyDependents(ReferenceEvent::SubobjectListChanged);

        PipelineObject::referenceRemoved(field, oldTarget, listIndex);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
