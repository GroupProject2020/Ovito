////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
#include <ovito/core/dataset/data/DataObject.h>
#include <ovito/core/dataset/pipeline/StaticSource.h>
#include <ovito/core/dataset/DataSetContainer.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(DataObject);
DEFINE_PROPERTY_FIELD(DataObject, identifier);
DEFINE_REFERENCE_FIELD(DataObject, visElements);
SET_PROPERTY_FIELD_LABEL(DataObject, visElements, "Visual elements");

/******************************************************************************
* Produces a string representation of the object path.
******************************************************************************/
QString ConstDataObjectPath::toString() const
{
	QString s;
	for(const DataObject* o : *this) {
		if(!s.isEmpty()) s += QChar('/');
		s += o->identifier();
	}
	return s;
}

/******************************************************************************
* Produces a string representation of the object path that is suitable for the
* user interface.
******************************************************************************/
QString ConstDataObjectPath::toHumanReadableString() const
{
	if(empty()) return {};
	QString str = back()->getOOMetaClass().displayName();
	bool first = true;
	for(const DataObject* obj : *this) {
		if(!obj->identifier().isEmpty()) {
			if(first) {
				first = false;
				str += QStringLiteral(": ");
			}
			else str += QStringLiteral(" / ");
			str += obj->objectTitle();
		}
	}
	return str;
}

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
	// Chunk is for future use...
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void DataObject::loadFromStream(ObjectLoadStream& stream)
{
	RefTarget::loadFromStream(stream);
	stream.expectChunk(0x02);
	// For future use...
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
* Determines if it is safe to modify this data object without unwanted side effects.
* Returns true if there is only one exclusive owner of this data object (if any).
* Returns false if there are multiple references to this data object from several
* data collections or other container data objects.
******************************************************************************/
bool DataObject::isSafeToModify() const
{
	OVITO_CHECK_OBJECT_POINTER(this);
	if(dependents().empty()) {
		return _referringFlowStates <= 1;
	}
	else if(dependents().size() == 1) {
		// Recursively determine if the container of this data object is safe to modify as well.
		// Only if the entire hierarchy of objects is safe to modify, we can safely modify
		// the leaf object.
		if(DataObject* owner = dynamic_object_cast<DataObject>(dependents().front())) {
			return owner->isSafeToModify();
		}
		else return true;
	}
	else return false;
}

/******************************************************************************
* Duplicates the given sub-object from this container object if it is shared
* with others. After this method returns, the returned sub-object will be
* exclusively owned by this container can be safely modified without unwanted
* side effects.
******************************************************************************/
DataObject* DataObject::makeMutable(const DataObject* subObject)
{
	OVITO_CHECK_OBJECT_POINTER(this);
	OVITO_ASSERT(subObject);
	OVITO_ASSERT(hasReferenceTo(subObject));
	OVITO_ASSERT(subObject->numberOfStrongReferences() >= 1);
	if(subObject && subObject->numberOfStrongReferences() > 1) {
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
