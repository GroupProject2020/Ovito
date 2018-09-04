///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2018) Alexander Stukowski
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
#include <core/dataset/data/DataObject.h>
#include <core/dataset/pipeline/StaticSource.h>
#include <core/dataset/DataSetContainer.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(DataObject);
DEFINE_PROPERTY_FIELD(DataObject, identifier);
DEFINE_REFERENCE_FIELD(DataObject, visElements);
SET_PROPERTY_FIELD_LABEL(DataObject, visElements, "Visual elements");

/******************************************************************************
* Constructor.
******************************************************************************/
DataObject::DataObject(DataSet* dataset) : RefTarget(dataset)
{
}

/******************************************************************************
* Sends an event to all dependents of this RefTarget.
******************************************************************************/
void DataObject::notifyDependentsImpl(const ReferenceEvent& event)
{
	// Automatically increment revision counter each time the object changes.
	if(event.type() == ReferenceEvent::TargetChanged)
		_revisionNumber++;

	RefTarget::notifyDependentsImpl(event);
}

/******************************************************************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool DataObject::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	// Automatically increment revision counter each time a sub-object of this object changes (except vis elements).
	if(event.type() == ReferenceEvent::TargetChanged && !visElements().contains(static_cast<DataVis*>(source))) {
		_revisionNumber++;
	}
	return RefTarget::referenceEvent(source, event);
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void DataObject::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	RefTarget::saveToStream(stream, excludeRecomputableData);
	stream.beginChunk(0x02);
	// For future use...
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void DataObject::loadFromStream(ObjectLoadStream& stream)
{
	RefTarget::loadFromStream(stream);
	stream.expectChunk(0x02);
	stream.closeChunk();
}

/******************************************************************************
* Returns the pipeline object that created this data object (may be NULL).
******************************************************************************/
PipelineObject* DataObject::dataSource() const
{
	return _dataSource;
}

/******************************************************************************
* Returns the pipeline object that created this data object (may be NULL).
******************************************************************************/
void DataObject::setDataSource(PipelineObject* dataSource)
{
	_dataSource = dataSource;
}

/******************************************************************************
* Duplicates the given sub-object from this container object if it is shared 
* with others. After this method returns, the returned sub-object will be 
* exclusively owned by this container can be safely modified without unwanted 
* side effects.
******************************************************************************/
DataObject* DataObject::makeMutable(const DataObject* subObject)
{
	OVITO_ASSERT(subObject);
	OVITO_ASSERT(hasReferenceTo(subObject));
	OVITO_ASSERT(subObject->numberOfStrongReferences() >= 1);
	if(subObject->numberOfStrongReferences() > 1) {
		OORef<DataObject> clone = CloneHelper().cloneObject(subObject, false);
		replaceReferencesTo(subObject, clone);
		subObject = clone;
	}
	OVITO_ASSERT(subObject->numberOfStrongReferences() == 1);
	return const_cast<DataObject*>(subObject);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
