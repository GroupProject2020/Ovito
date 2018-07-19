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

#include <plugins/stdobj/StdObj.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/UndoStack.h>
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
OORef<RefTarget> ElementSelectionSet::clone(bool deepCopy, CloneHelper& cloneHelper)
{
	// Let the base class create an instance of this class.
	OORef<ElementSelectionSet> clone = static_object_cast<ElementSelectionSet>(RefTarget::clone(deepCopy, cloneHelper));
	clone->_selection = this->_selection;
	clone->_selectedIdentifiers = this->_selectedIdentifiers;
	return clone;
}

/******************************************************************************
* Adopts the selection state from the modifier's input.
******************************************************************************/
void ElementSelectionSet::resetSelection(const PipelineFlowState& state, const PropertyClass& propertyClass)
{
	// Take a snapshot of the current selection state.
	if(PropertyObject* selProperty = propertyClass.findInState(state, PropertyStorage::GenericSelectionProperty)) {

		// Make a backup of the old snapshot so it may be restored.
		dataset()->undoStack().pushIfRecording<ReplaceSelectionOperation>(this);

		PropertyObject* identifierProperty = propertyClass.findInState(state, PropertyStorage::GenericIdentifierProperty);
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
		clearSelection(state, propertyClass);
	}
}

/******************************************************************************
* Clears the selection set.
******************************************************************************/
void ElementSelectionSet::clearSelection(const PipelineFlowState& state, const PropertyClass& propertyClass)
{
	// Make a backup of the old selection state so it may be restored.
	dataset()->undoStack().pushIfRecording<ReplaceSelectionOperation>(this);

	if(useIdentifiers() && propertyClass.findInState(state, PropertyStorage::GenericIdentifierProperty)) {
		_selection.clear();
		_selectedIdentifiers.clear();
	}
	else {
		_selection.reset();
		_selection.resize(propertyClass.elementCount(state), false);
		_selectedIdentifiers.clear();
	}
	notifyTargetChanged();
}

/******************************************************************************
* Replaces the selection set.
******************************************************************************/
void ElementSelectionSet::setSelection(const PipelineFlowState& state, const PropertyClass& propertyClass, const boost::dynamic_bitset<>& selection, SelectionMode mode)
{
	// Make a backup of the old snapshot so it may be restored.
	dataset()->undoStack().pushIfRecording<ReplaceSelectionOperation>(this);

	PropertyObject* identifierProperty = propertyClass.findInState(state, PropertyStorage::GenericIdentifierProperty);
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
void ElementSelectionSet::toggleElement(const PipelineFlowState& state, const PropertyClass& propertyClass, size_t elementIndex)
{
	if(elementIndex >= propertyClass.elementCount(state))
		return;

	PropertyObject* identifiers = propertyClass.findInState(state, PropertyStorage::GenericIdentifierProperty);
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
void ElementSelectionSet::selectAll(const PipelineFlowState& state, const PropertyClass& propertyClass)
{
	// Make a backup of the old selection state so it may be restored.
	dataset()->undoStack().pushIfRecording<ReplaceSelectionOperation>(this);

	PropertyObject* identifiers = propertyClass.findInState(state, PropertyStorage::GenericIdentifierProperty);
	if(useIdentifiers() && identifiers != nullptr) {
		_selection.clear();
		_selectedIdentifiers.clear();
		for(auto id : identifiers->constInt64Range())
			_selectedIdentifiers.insert(id);
	}
	else {
		_selection.set();
		_selection.resize(propertyClass.elementCount(state), true);
		_selectedIdentifiers.clear();
	}
	notifyTargetChanged();
}

/******************************************************************************
* Copies the stored selection set into the given output selection property.
******************************************************************************/
PipelineStatus ElementSelectionSet::applySelection(PropertyObject* outputSelectionProperty, PropertyObject* identifierProperty)
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
