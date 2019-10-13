////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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

#include <ovito/stdobj/StdObj.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/UndoStack.h>
#include "ElementSelectionSet.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(ElementSelectionSet);
DEFINE_PROPERTY_FIELD(ElementSelectionSet, useIdentifiers);

/* Undo record that can restore an old selection state. */
class ReplaceSelectionOperation : public UndoableOperation
{
public:
	ReplaceSelectionOperation(ElementSelectionSet* owner) :
		_owner(owner), _selection(owner->_selection), _selectedIdentifiers(owner->_selectedIdentifiers) {}

	virtual void undo() override {
		_selection.swap(_owner->_selection);
		_selectedIdentifiers.swap(_owner->_selectedIdentifiers);
		_owner->notifyTargetChanged();
	}

	virtual QString displayName() const override {
		return QStringLiteral("Replace selection set");
	}

private:

	OORef<ElementSelectionSet> _owner;
	boost::dynamic_bitset<> _selection;
	QSet<qlonglong> _selectedIdentifiers;
};

/* Undo record that can restore selection state of a single element. */
class ToggleSelectionOperation : public UndoableOperation
{
public:
	ToggleSelectionOperation(ElementSelectionSet* owner, qlonglong id, size_t elementIndex = std::numeric_limits<size_t>::max()) :
		_owner(owner), _index(elementIndex), _id(id) {}

	virtual void undo() override {
		if(_index != std::numeric_limits<size_t>::max())
			_owner->toggleElementByIndex(_index);
		else
			_owner->toggleElementById(_id);
	}

	virtual QString displayName() const override {
		return QStringLiteral("Toggle element selection");
	}

private:

	OORef<ElementSelectionSet> _owner;
	qlonglong _id;
	size_t _index;
};

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void ElementSelectionSet::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	RefTarget::saveToStream(stream, excludeRecomputableData);
	stream.beginChunk(0x02);
	stream << _selection;
	stream << _selectedIdentifiers;
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void ElementSelectionSet::loadFromStream(ObjectLoadStream& stream)
{
	RefTarget::loadFromStream(stream);
	stream.expectChunk(0x02);
	stream >> _selection;
	stream >> _selectedIdentifiers;
	stream.closeChunk();
}

/******************************************************************************
* Creates a copy of this object.
******************************************************************************/
OORef<RefTarget> ElementSelectionSet::clone(bool deepCopy, CloneHelper& cloneHelper) const
{
	// Let the base class create an instance of this class.
	OORef<ElementSelectionSet> clone = static_object_cast<ElementSelectionSet>(RefTarget::clone(deepCopy, cloneHelper));
	clone->_selection = this->_selection;
	clone->_selectedIdentifiers = this->_selectedIdentifiers;
	return clone;
}

/******************************************************************************
* Adopts the selection set from the given input property container.
******************************************************************************/
void ElementSelectionSet::resetSelection(const PropertyContainer* container)
{
	OVITO_ASSERT(container != nullptr);

	// Take a snapshot of the current selection state.
	if(const PropertyObject* selProperty = container->getProperty(PropertyStorage::GenericSelectionProperty)) {

		// Make a backup of the old snapshot so it may be restored.
		dataset()->undoStack().pushIfRecording<ReplaceSelectionOperation>(this);

		const PropertyObject* identifierProperty = container->getOOMetaClass().isValidStandardPropertyId(PropertyStorage::GenericIdentifierProperty) ?
			container->getProperty(PropertyStorage::GenericIdentifierProperty) : nullptr;
		OVITO_ASSERT(!identifierProperty || selProperty->size() == identifierProperty->size());

		if(identifierProperty && selProperty->size() == identifierProperty->size() && useIdentifiers()) {
			_selectedIdentifiers.clear();
			_selection.clear();
			auto s = selProperty->constDataInt();
			for(auto id : identifierProperty->constInt64Range()) {
				if(*s++)
					_selectedIdentifiers.insert(id);
			}
		}
		else {
			// Take a snapshot of the selection state.
			_selectedIdentifiers.clear();
			_selection.resize(selProperty->size());
			auto s = selProperty->constDataInt();
			auto s_end = s + selProperty->size();
			for(size_t index = 0; s != s_end; ++s, index++) {
				_selection.set(index, *s);
			}
		}

		notifyTargetChanged();
	}
	else {
		// Reset selection snapshot if input doesn't contain a selection state.
		clearSelection(container);
	}
}

/******************************************************************************
* Clears the selection set.
******************************************************************************/
void ElementSelectionSet::clearSelection(const PropertyContainer* container)
{
	OVITO_ASSERT(container != nullptr);

	// Make a backup of the old selection state so it may be restored.
	dataset()->undoStack().pushIfRecording<ReplaceSelectionOperation>(this);

	if(useIdentifiers() && container->getOOMetaClass().isValidStandardPropertyId(PropertyStorage::GenericIdentifierProperty) && container->getProperty(PropertyStorage::GenericIdentifierProperty)) {
		_selection.clear();
		_selectedIdentifiers.clear();
	}
	else {
		_selection.reset();
		_selection.resize(container->elementCount(), false);
		_selectedIdentifiers.clear();
	}
	notifyTargetChanged();
}

/******************************************************************************
* Replaces the selection set.
******************************************************************************/
void ElementSelectionSet::setSelection(const PropertyContainer* container, const boost::dynamic_bitset<>& selection, SelectionMode mode)
{
	// Make a backup of the old snapshot so it may be restored.
	dataset()->undoStack().pushIfRecording<ReplaceSelectionOperation>(this);

	const PropertyObject* identifierProperty = container->getOOMetaClass().isValidStandardPropertyId(PropertyStorage::GenericIdentifierProperty) ?
		container->getProperty(PropertyStorage::GenericIdentifierProperty) : nullptr;
	OVITO_ASSERT(!identifierProperty || selection.size() == identifierProperty->size());

	if(identifierProperty && useIdentifiers()) {
		_selection.clear();
		size_t index = 0;
		if(mode == SelectionReplace) {
			_selectedIdentifiers.clear();
			for(auto id : identifierProperty->constInt64Range()) {
				if(selection.test(index++))
					_selectedIdentifiers.insert(id);
			}
		}
		else if(mode == SelectionAdd) {
			for(auto id : identifierProperty->constInt64Range()) {
				if(selection.test(index++))
					_selectedIdentifiers.insert(id);
			}
		}
		else if(mode == SelectionSubtract) {
			for(auto id : identifierProperty->constInt64Range()) {
				if(selection.test(index++))
					_selectedIdentifiers.remove(id);
			}
		}
	}
	else {
		_selectedIdentifiers.clear();
		if(mode == SelectionReplace)
			_selection = selection;
		else if(mode == SelectionAdd) {
			_selection.resize(selection.size());
			_selection |= selection;
		}
		else if(mode == SelectionSubtract) {
			_selection.resize(selection.size());
			_selection &= ~selection;
		}
	}

	notifyTargetChanged();
}

/******************************************************************************
* Toggles the selection state of a single element.
******************************************************************************/
void ElementSelectionSet::toggleElement(const PropertyContainer* container, size_t elementIndex)
{
	if(elementIndex >= container->elementCount())
		return;

	const PropertyObject* identifiers = container->getOOMetaClass().isValidStandardPropertyId(PropertyStorage::GenericIdentifierProperty) ?
		container->getProperty(PropertyStorage::GenericIdentifierProperty) : nullptr;
	if(useIdentifiers() && identifiers) {
		_selection.clear();
		toggleElementById(identifiers->getInt64(elementIndex));
	}
	else if(elementIndex < _selection.size()) {
		_selectedIdentifiers.clear();
		toggleElementByIndex(elementIndex);
	}
}

/******************************************************************************
* Toggles the selection state of a single element.
******************************************************************************/
void ElementSelectionSet::toggleElementById(qlonglong elementId)
{
	// Make a backup of the old selection state so it may be restored.
	dataset()->undoStack().pushIfRecording<ToggleSelectionOperation>(this, elementId);

	if(useIdentifiers()) {
		if(_selectedIdentifiers.contains(elementId))
			_selectedIdentifiers.remove(elementId);
		else
			_selectedIdentifiers.insert(elementId);
	}
	notifyTargetChanged();
}

/******************************************************************************
* Toggles the selection state of a single element.
******************************************************************************/
void ElementSelectionSet::toggleElementByIndex(size_t elementIndex)
{
	// Make a backup of the old selection state so it may be restored.
	dataset()->undoStack().pushIfRecording<ToggleSelectionOperation>(this, -1, elementIndex);

	if(elementIndex < _selection.size())
		_selection.flip(elementIndex);
	notifyTargetChanged();
}

/******************************************************************************
* Selects all elements.
******************************************************************************/
void ElementSelectionSet::selectAll(const PropertyContainer* container)
{
	// Make a backup of the old selection state so it may be restored.
	dataset()->undoStack().pushIfRecording<ReplaceSelectionOperation>(this);

	const PropertyObject* identifiers = container->getOOMetaClass().isValidStandardPropertyId(PropertyStorage::GenericIdentifierProperty) ?
		container->getProperty(PropertyStorage::GenericIdentifierProperty) : nullptr;
	if(useIdentifiers() && identifiers != nullptr) {
		_selection.clear();
		_selectedIdentifiers.clear();
		for(auto id : identifiers->constInt64Range())
			_selectedIdentifiers.insert(id);
	}
	else {
		_selection.set();
		_selection.resize(container->elementCount(), true);
		_selectedIdentifiers.clear();
	}
	notifyTargetChanged();
}

/******************************************************************************
* Copies the stored selection set into the given output selection property.
******************************************************************************/
PipelineStatus ElementSelectionSet::applySelection(PropertyObject* outputSelectionProperty, const PropertyObject* identifierProperty)
{
	size_t nselected = 0;
	if(!identifierProperty || !useIdentifiers()) {

		// When not using identifiers, the number of input elements must match.
		if(outputSelectionProperty->size() != _selection.size())
			throwException(tr("Stored selection state became invalid, because the number of input elements has changed."));

		// Restore selection simply by placing the snapshot into the pipeline.
		size_t index = 0;
		for(auto& s : outputSelectionProperty->intRange()) {
			if((s = _selection.test(index++)))
				nselected++;
		}
	}
	else {
		OVITO_ASSERT(outputSelectionProperty->size() == identifierProperty->size());

		auto id = identifierProperty->constDataInt64();
		for(auto& s : outputSelectionProperty->intRange()) {
			if((s = _selectedIdentifiers.contains(*id++)))
				nselected++;
		}
	}
	outputSelectionProperty->notifyTargetChanged();

	return PipelineStatus(PipelineStatus::Success, tr("%1 elements selected").arg(nselected));
}

}	// End of namespace
}	// End of namespace
