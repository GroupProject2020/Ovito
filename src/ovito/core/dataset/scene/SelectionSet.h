////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Alexander Stukowski
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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/oo/RefTarget.h>
#include "SceneNode.h"

namespace Ovito {

/**
 * \brief Stores a selection of scene nodes.
 *
 * This selection set class holds a reference list to all SceneNode objects
 * that are selected.
 *
 * The current selection set can be accessed via the DataSetManager::currentSelection() method.
 */
class OVITO_CORE_EXPORT SelectionSet : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(SelectionSet)

public:

	/// \brief Creates an empty selection set.
	Q_INVOKABLE SelectionSet(DataSet* dataset);

	/// \brief Adds a scene node to this selection set.
	/// \param node The node to be added.
	/// \undoable
	void push_back(SceneNode* node);

	/// \brief Inserts a scene node into this selection set.
	/// \param index The index at which to insert the node into the list.
	/// \param node The node to be inserted.
	/// \undoable
	void insert(int index, SceneNode* node);

	/// \brief Removes a scene node from this selection set.
	/// \param node The node to be unselected.
	/// \undoable
	void remove(SceneNode* node);

	/// \brief Removes a scene node from this selection set.
	/// \param index The index of the node to be unselected.
	/// \undoable
	void removeByIndex(int index) { _nodes.remove(this, PROPERTY_FIELD(nodes), index); }

	/// \brief Clears the selection.
	///
	/// All nodes are removed from the selection set.
	/// \undoable
	void clear() { setNodes({}); }

	/// \brief Resets the selection set to contain only the given node.
	/// \param node The node to be selected.
	void setNode(SceneNode* node) {
		if(node)
			setNodes({node});
		else
			clear();
	}

	/// \brief Returns the first scene node from the selection, or NULL if the set is empty.
	SceneNode* firstNode() const {
		return nodes().empty() ? nullptr : nodes().front();
	}

Q_SIGNALS:

	/// \brief Is emitted when nodes have been added or removed from the selection set.
	/// \param selection This selection set.
	/// \note This signal is NOT emitted when a node in the selection set has changed.
	/// \note In contrast to the selectionChangeComplete() signal, this signal is emitted
	///       for every node that is added to or removed from the selection set. That means
	///       a call to SelectionSet::addAll() for example will generate multiple selectionChanged()
	///       events but only one final selectionChangeComplete() event.
	void selectionChanged(SelectionSet* selection);

	/// \brief This signal is emitted after all changes to the selection set have been completed.
	/// \param selection This selection set.
	/// \note This signal is NOT emitted when a node in the selection set has changed.
	/// \note In contrast to the selectionChange() signal this signal is emitted
	///       only once after the selection set has been changed. That is,
	///       a call to SelectionSet::addAll() for example will generate multiple selectionChanged()
	///       events but only one final selectionChangeComplete() event.
	void selectionChangeComplete(SelectionSet* selection);

protected:

	/// Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
	virtual void referenceInserted(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex) override;

	/// Is called when a RefTarget has been removed from a VectorReferenceField of this RefMaker.
	virtual void referenceRemoved(const PropertyFieldDescriptor& field, RefTarget* oldTarget, int listIndex) override;

	/// This method is invoked after the change of the selection set is complete. It emits the selectionChangeComplete() signal.
	Q_INVOKABLE void onSelectionChangeCompleted();

private:

	/// Holds the references to the selected scene nodes.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD_FLAGS(SceneNode, nodes, setNodes, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_NEVER_CLONE_TARGET);

	/// Indicates that there is a pending change event in the event queue.
	bool _selectionChangeInProgress = false;
};

}	// End of namespace


