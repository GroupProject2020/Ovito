///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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
#include <ovito/core/dataset/scene/SelectionSet.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(SelectionSet);
DEFINE_REFERENCE_FIELD(SelectionSet, nodes);
SET_PROPERTY_FIELD_LABEL(SelectionSet, nodes, "Nodes");

/******************************************************************************
* Default constructor.
******************************************************************************/
SelectionSet::SelectionSet(DataSet* dataset) : RefTarget(dataset)
{
}

/******************************************************************************
* Adds a scene node to this selection set.
******************************************************************************/
void SelectionSet::push_back(SceneNode* node)
{
	OVITO_CHECK_OBJECT_POINTER(node);
	if(nodes().contains(node))
		throwException(tr("Node is already in the selection set."));

	// Insert into children array.
	_nodes.push_back(this, PROPERTY_FIELD(nodes), node);
}

/******************************************************************************
* Inserts a scene node into this selection set.
******************************************************************************/
void SelectionSet::insert(int index, SceneNode* node)
{
	OVITO_CHECK_OBJECT_POINTER(node);
	if(nodes().contains(node))
		throwException(tr("Node is already in the selection set."));

	// Insert into children array.
	_nodes.insert(this, PROPERTY_FIELD(nodes), index, node);
}

/******************************************************************************
* Removes a scene node from this selection set.
******************************************************************************/
void SelectionSet::remove(SceneNode* node)
{
	int index = _nodes.indexOf(node);
	if(index == -1) return;
	removeByIndex(index);
	OVITO_ASSERT(!nodes().contains(node));
}

/******************************************************************************
* Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
******************************************************************************/
void SelectionSet::referenceInserted(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(nodes)) {
		Q_EMIT selectionChanged(this);
		if(!_selectionChangeInProgress) {
			_selectionChangeInProgress = true;
			QMetaObject::invokeMethod(this, "onSelectionChangeCompleted", Qt::QueuedConnection);
		}
	}
	RefTarget::referenceInserted(field, newTarget, listIndex);
}

/******************************************************************************
* Is called when a RefTarget has been removed from a VectorReferenceField of this RefMaker.
******************************************************************************/
void SelectionSet::referenceRemoved(const PropertyFieldDescriptor& field, RefTarget* oldTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(nodes)) {
		Q_EMIT selectionChanged(this);
		if(!_selectionChangeInProgress) {
			_selectionChangeInProgress = true;
			QMetaObject::invokeMethod(this, "onSelectionChangeCompleted", Qt::QueuedConnection);
		}
	}
	RefTarget::referenceRemoved(field, oldTarget, listIndex);
}

/******************************************************************************
* This method is invoked after the change of the selection set is complete.
* It emits the selectionChangeComplete() signal.
******************************************************************************/
void SelectionSet::onSelectionChangeCompleted()
{
	OVITO_ASSERT(_selectionChangeInProgress);
	_selectionChangeInProgress = false;
	Q_EMIT selectionChangeComplete(this);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
