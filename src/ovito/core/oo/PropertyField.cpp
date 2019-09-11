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

#include <ovito/core/Core.h>
#include <ovito/core/oo/PropertyFieldDescriptor.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/DataSet.h>
#include "PropertyField.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/******************************************************************************
* Generates a notification event to inform the dependents of the field's owner
* that it has changed.
******************************************************************************/
void PropertyFieldBase::generateTargetChangedEvent(RefMaker* owner, const PropertyFieldDescriptor& descriptor, ReferenceEvent::Type eventType)
{
	// Make sure we are not trying to generate a change message for objects that are not RefTargets.
	OVITO_ASSERT_MSG(!descriptor.shouldGenerateChangeEvent() || descriptor.definingClass()->isDerivedFrom(RefTarget::OOClass()),
			"PropertyFieldBase::generateTargetChangedEvent()",
			qPrintable(QString("Flag PROPERTY_FIELD_NO_CHANGE_MESSAGE has not been set for property field '%1' of class '%2' even though '%2' is not derived from RefTarget.")
					.arg(descriptor.identifier()).arg(descriptor.definingClass()->name())));

	// Send notification message to dependents of owner object.
	if(eventType != ReferenceEvent::TargetChanged) {
		OVITO_ASSERT(owner->isRefTarget());
		static_object_cast<RefTarget>(owner)->notifyDependents(eventType);
	}
	else if(descriptor.shouldGenerateChangeEvent()) {
		OVITO_ASSERT(owner->isRefTarget());
		static_object_cast<RefTarget>(owner)->notifyTargetChanged(&descriptor);
	}
}

/******************************************************************************
* Generates a notification event to inform the dependents of the field's owner
* that it has changed.
******************************************************************************/
void PropertyFieldBase::generatePropertyChangedEvent(RefMaker* owner, const PropertyFieldDescriptor& descriptor) const
{
	owner->propertyChanged(descriptor);
}

/******************************************************************************
* Indicates whether undo records should be created.
******************************************************************************/
bool PropertyFieldBase::isUndoRecordingActive(RefMaker* owner, const PropertyFieldDescriptor& descriptor) const
{
	return descriptor.automaticUndo() && owner->dataset() && owner->dataset()->undoStack().isRecording();
}

/******************************************************************************
* Puts a record on the undo stack.
******************************************************************************/
void PropertyFieldBase::pushUndoRecord(RefMaker* owner, std::unique_ptr<UndoableOperation>&& operation)
{
	owner->dataset()->undoStack().push(std::move(operation));
}

/******************************************************************************
* Constructor.
******************************************************************************/
PropertyFieldBase::PropertyFieldOperation::PropertyFieldOperation(RefMaker* owner, const PropertyFieldDescriptor& descriptor) :
	_owner(owner != owner->dataset() ? owner : nullptr), _descriptor(descriptor)
{
}

/******************************************************************************
* Access to the object whose property was changed.
******************************************************************************/
RefMaker* PropertyFieldBase::PropertyFieldOperation::owner() const
{
	return static_cast<RefMaker*>(_owner.get());
}

/******************************************************************************
* Replaces the target stored in the reference field.
******************************************************************************/
void SingleReferenceFieldBase::swapReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, OORef<RefTarget>& inactiveTarget, bool generateNotificationEvents)
{
	OVITO_CHECK_POINTER(this);
	OVITO_CHECK_OBJECT_POINTER(owner);
	OVITO_ASSERT(!descriptor.isVector());

	// Check for cyclic references.
	if(inactiveTarget && (owner->isReferencedBy(inactiveTarget.get()) || owner == inactiveTarget.get())) {
		OVITO_ASSERT(!owner->isRefTarget() || !owner->dataset() || !owner->dataset()->undoStack().isUndoingOrRedoing());
		throw CyclicReferenceError();
	}

	OORef<RefTarget> oldTarget(_pointer);

	if(inactiveTarget && !descriptor.isWeakReference())
		inactiveTarget->incrementReferenceCount();

	if(_pointer && !descriptor.isWeakReference())
		_pointer->decrementReferenceCount();

	_pointer = inactiveTarget.get();

	// Remove the RefMaker from the old target's list of dependents if it has no
	// more references to it.
	if(oldTarget) {
		OVITO_ASSERT(oldTarget->_dependents.contains(owner));
		if(!owner->hasReferenceTo(oldTarget.get())) {
			oldTarget->_dependents.remove(owner);
		}
	}

	// Add the RefMaker to the list of dependents of the new target.
	if(_pointer) {
		if(!_pointer->_dependents.contains(owner))
			_pointer->_dependents.push_back(owner);
	}

	if(generateNotificationEvents) {

		// Inform derived classes.
		owner->referenceReplaced(descriptor, oldTarget.get(), _pointer);

		// Send auto change message.
		generateTargetChangedEvent(owner, descriptor);

		// An additional message can be requested by the user using the SET_PROPERTY_FIELD_CHANGE_EVENT macro.
		if(descriptor.extraChangeEventType() != 0)
			generateTargetChangedEvent(owner, descriptor, static_cast<ReferenceEvent::Type>(descriptor.extraChangeEventType()));
	}

	oldTarget.swap(inactiveTarget);
}

/******************************************************************************
* Replaces the reference target stored in a reference field.
* Creates an undo record so the old value can be restored at a later time.
******************************************************************************/
void SingleReferenceFieldBase::setInternal(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const RefTarget* newTarget)
{
	if(_pointer == newTarget) return;	// Nothing has changed.

    // Check object type
	if(newTarget && !newTarget->getOOClass().isDerivedFrom(*descriptor.targetClass())) {
		OVITO_ASSERT_MSG(false, "SingleReferenceFieldBase::set", "Tried to create a reference to an incompatible object for this reference field.");
		owner->throwException(QString("Cannot set a reference field of type %1 to an incompatible object of type %2.").arg(descriptor.targetClass()->name(), newTarget->getOOClass().name()));
	}

	// Make sure automatic undo is disabled for a reference field of a class that is not derived from RefTarget.
	OVITO_ASSERT_MSG(descriptor.automaticUndo() == false || owner->isRefTarget(), "SingleReferenceFieldBase::set()",
			qPrintable(QString("PROPERTY_FIELD_NO_UNDO flag has not been set for reference field '%1' of non-RefTarget derived class '%2'.")
				.arg(descriptor.identifier()).arg(descriptor.definingClass()->name())));

	if(isUndoRecordingActive(owner, descriptor)) {
		std::unique_ptr<SetReferenceOperation> op(new SetReferenceOperation(owner, const_cast<RefTarget*>(newTarget), *this, descriptor));
		op->redo();
		pushUndoRecord(owner, std::move(op));
		OVITO_ASSERT(_pointer == newTarget);
	}
	else {
		OORef<RefTarget> newTargetRef = const_cast<RefTarget*>(newTarget);
		swapReference(owner, descriptor, newTargetRef);
		OVITO_ASSERT(_pointer == newTarget);
	}
}

/******************************************************************************
* Constructor of the undo record.
******************************************************************************/
SingleReferenceFieldBase::SetReferenceOperation::SetReferenceOperation(RefMaker* owner, RefTarget* oldTarget, SingleReferenceFieldBase& reffield, const PropertyFieldDescriptor& descriptor)
	: PropertyFieldOperation(owner, descriptor), _inactiveTarget(oldTarget), _reffield(reffield)
{
	// Make sure we are not keeping a reference to the DataSet. That would be an invalid circular reference.
	OVITO_ASSERT(oldTarget != owner->dataset());
}

/******************************************************************************
* Returns a text representation of the undo record.
******************************************************************************/
QString SingleReferenceFieldBase::SetReferenceOperation::displayName() const
{
	return QStringLiteral("Setting ref field <%1> of %2 to object %3")
		.arg(descriptor().identifier())
		.arg(owner()->getOOClass().name())
		.arg(_inactiveTarget ? _inactiveTarget->getOOClass().name() : "<null>");
}

/******************************************************************************
* Removes a target from the list reference field.
******************************************************************************/
OORef<RefTarget> VectorReferenceFieldBase::removeReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int index, bool generateNotificationEvents)
{
	OVITO_CHECK_POINTER(this);
	OVITO_CHECK_OBJECT_POINTER(owner);
	OVITO_ASSERT(descriptor.isVector());

	OVITO_ASSERT(index >= 0 && index < pointers.size());
	OORef<RefTarget> target = pointers.at(index);

	// Remove reference.
	pointers.remove(index);

	// Release old reference target if there are no more references to it.
	if(target) {
		if(!descriptor.isWeakReference()) {
			OVITO_ASSERT(target->objectReferenceCount() >= 2);
			target->decrementReferenceCount();
		}

		// Remove the RefMaker from the old target's list of dependents.
		OVITO_CHECK_OBJECT_POINTER(target);
		OVITO_ASSERT(target->_dependents.contains(owner));
		if(!owner->hasReferenceTo(target.get())) {
			target->_dependents.remove(owner);
		}
	}

	if(generateNotificationEvents) {
		try {
			// Inform derived classes.
			owner->referenceRemoved(descriptor, target.get(), index);

			// Send auto change message.
			generateTargetChangedEvent(owner, descriptor);

			// An additional message can be requested by the user using the SET_PROPERTY_FIELD_CHANGE_EVENT macro.
			if(descriptor.extraChangeEventType() != 0)
				generateTargetChangedEvent(owner, descriptor, static_cast<ReferenceEvent::Type>(descriptor.extraChangeEventType()));
		}
		catch(...) {
			if(!owner->isRefTarget() || !owner->dataset() || !owner->dataset()->undoStack().isUndoingOrRedoing())
				throw;
			qDebug() << "Caught exception in VectorReferenceFieldBase::removeReference(). RefMaker is" << owner << ". RefTarget is" << target;
		}
	}

	return target;
}

/******************************************************************************
* Adds the target to the list reference field.
******************************************************************************/
int VectorReferenceFieldBase::addReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const OORef<RefTarget>& target, int index)
{
	OVITO_CHECK_POINTER(this);
	OVITO_CHECK_OBJECT_POINTER(owner);
	OVITO_ASSERT(descriptor.isVector());

	// Check for cyclic references.
	if(target && (owner->isReferencedBy(target.get()) || owner == target.get())) {
		OVITO_ASSERT(!owner->isRefTarget() || !owner->dataset() || !owner->dataset()->undoStack().isUndoingOrRedoing());
		throw CyclicReferenceError();
	}

	// Add new reference to list field.
	if(index == -1) {
		index = pointers.size();
		pointers.push_back(target.get());
	}
	else {
		OVITO_ASSERT(index >= 0 && index <= pointers.size());
		pointers.insert(index, target.get());
	}
	if(target && !descriptor.isWeakReference())
		target->incrementReferenceCount();

	// Add the RefMaker to the list of dependents of the new target.
	if(target && !target->_dependents.contains(owner))
		target->_dependents.push_back(owner);

	try {
		// Inform derived classes.
		owner->referenceInserted(descriptor, target.get(), index);

		// Send auto change message.
		generateTargetChangedEvent(owner, descriptor);

		// An additional message can be requested by the user using the SET_PROPERTY_FIELD_CHANGE_EVENT macro.
		if(descriptor.extraChangeEventType() != 0)
			generateTargetChangedEvent(owner, descriptor, static_cast<ReferenceEvent::Type>(descriptor.extraChangeEventType()));
	}
	catch(...) {
		if(!owner->isRefTarget() || !owner->dataset() || !owner->dataset()->undoStack().isUndoingOrRedoing())
			throw;
		qDebug() << "Caught exception in VectorReferenceFieldBase::addReference(). RefMaker is" << owner << ". RefTarget is" << target.get();
	}

	return index;
}

/******************************************************************************
* Adds a reference target to the internal list.
* Creates an undo record so the insertion can be undone at a later time.
******************************************************************************/
int VectorReferenceFieldBase::insertInternal(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const RefTarget* newTarget, int index)
{
    // Check object type
	if(newTarget && !newTarget->getOOClass().isDerivedFrom(*descriptor.targetClass())) {
		OVITO_ASSERT_MSG(false, "VectorReferenceFieldBase::insertInternal", "Cannot add incompatible object to this vector reference field.");
		owner->throwException(QString("Cannot add an object to a reference field of type %1 that has the incompatible type %2.").arg(descriptor.targetClass()->name(), newTarget->getOOClass().name()));
	}

	// Make sure automatic undo is disabled for a reference field of a class that is not derived from RefTarget.
	OVITO_ASSERT_MSG(descriptor.automaticUndo() == false || owner->isRefTarget(), "VectorReferenceFieldBase::insertInternal()",
			qPrintable(QString("PROPERTY_FIELD_NO_UNDO flag has not been set for reference field '%1' of non-RefTarget derived class '%2'.")
					.arg(descriptor.identifier()).arg(descriptor.definingClass()->name())));

	if(isUndoRecordingActive(owner, descriptor)) {
		auto op = std::make_unique<InsertReferenceOperation>(owner, const_cast<RefTarget*>(newTarget), *this, index, descriptor);
		op->redo();
		int index = op->insertionIndex();
		pushUndoRecord(owner, std::move(op));
		return index;
	}
	else {
		return addReference(owner, descriptor, const_cast<RefTarget*>(newTarget), index);
	}
}

/******************************************************************************
* Removes the element at index position i.
* Creates an undo record so the removal can be undone at a later time.
******************************************************************************/
void VectorReferenceFieldBase::remove(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int i)
{
	OVITO_ASSERT(i >= 0 && i < size());

	// Make sure automatic undo is disabled for a reference field of a class that is not derived from RefTarget.
	OVITO_ASSERT_MSG(descriptor.automaticUndo() == false || owner->isRefTarget(), "VectorReferenceFieldBase::remove()",
			qPrintable(QString("PROPERTY_FIELD_NO_UNDO flag has not been set for reference field '%1' of non-RefTarget derived class '%2'.")
					.arg(descriptor.identifier()).arg(descriptor.definingClass()->name())));

	if(isUndoRecordingActive(owner, descriptor)) {
		auto op = std::make_unique<RemoveReferenceOperation>(owner, *this, i, descriptor);
		op->redo();
		pushUndoRecord(owner, std::move(op));
	}
	else {
		removeReference(owner, descriptor, i);
	}
}

/******************************************************************************
* Clears all references at sets the vector size to zero.
******************************************************************************/
void VectorReferenceFieldBase::clear(RefMaker* owner, const PropertyFieldDescriptor& descriptor)
{
	while(!pointers.empty())
		remove(owner, descriptor, pointers.size() - 1);
}

/******************************************************************************
* Constructor of the undo record.
******************************************************************************/
VectorReferenceFieldBase::InsertReferenceOperation::InsertReferenceOperation(RefMaker* owner, RefTarget* target, VectorReferenceFieldBase& reffield, int index, const PropertyFieldDescriptor& descriptor) :
	PropertyFieldOperation(owner, descriptor), _target(target), _reffield(reffield), _index(index)
{
	// Make sure we are not keeping a reference to the DataSet. That would be an invalid circular reference.
	OVITO_ASSERT(!_target || _target != owner->dataset());
}

/******************************************************************************
* Constructor of the undo record.
******************************************************************************/
VectorReferenceFieldBase::RemoveReferenceOperation::RemoveReferenceOperation(RefMaker* owner, VectorReferenceFieldBase& reffield, int index, const PropertyFieldDescriptor& descriptor) :
	PropertyFieldOperation(owner, descriptor), _reffield(reffield), _index(index)
{
	// Make sure we are not keeping a reference to the DataSet. That would be an invalid circular reference.
	OVITO_ASSERT(_reffield[index] != owner->dataset());
}

/******************************************************************************
* Returns a text representation of the undo record.
******************************************************************************/
QString VectorReferenceFieldBase::InsertReferenceOperation::displayName() const
{
	return QStringLiteral("Insert ref to %1 into vector field <%2> of %3")
		.arg(_target ? _target->getOOClass().name() : "<null>")
		.arg(descriptor().identifier())
		.arg(owner()->getOOClass().name());
}

/******************************************************************************
* Returns a text representation of the undo record.
******************************************************************************/
QString VectorReferenceFieldBase::RemoveReferenceOperation::displayName() const
{
	return QStringLiteral("Remove ref to %1 from vector field <%2> of %3")
		.arg(_target ? _target->getOOClass().name() : "<null>")
		.arg(descriptor().identifier())
		.arg(owner()->getOOClass().name());
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
