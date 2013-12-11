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

#include <core/Core.h>
#include <core/scene/SceneNode.h>
#include <core/scene/SceneRoot.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/DataSet.h>
#include "SceneNodesListModel.h"

namespace Ovito {

/******************************************************************************
* Constructs the model.
******************************************************************************/
SceneNodesListModel::SceneNodesListModel(DataSetContainer& datasetContainer, QWidget* parent) : QAbstractListModel(parent),
		_datasetContainer(datasetContainer)
{
	// Listen for changes of the data set.
	connect(&datasetContainer, &DataSetContainer::dataSetChanged, this, &SceneNodesListModel::onDataSetChanged);

	// Listen for events of the root node.
	connect(&_rootNodeListener, SIGNAL(notificationEvent(ReferenceEvent*)), this, SLOT(onRootNodeNotificationEvent(ReferenceEvent*)));

	// Listen for events of the other scene nodes.
	connect(&_nodeListener, SIGNAL(notificationEvent(RefTarget*, ReferenceEvent*)), this, SLOT(onNodeNotificationEvent(RefTarget*, ReferenceEvent*)));
}

/******************************************************************************
* Returns the number of rows of the model.
******************************************************************************/
int SceneNodesListModel::rowCount(const QModelIndex& parent) const
{
	return _nodeListener.targets().size();
}

/******************************************************************************
* Returns the model's data stored under the given role for the item referred to by the index.
******************************************************************************/
QVariant SceneNodesListModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::DisplayRole) {
		SceneNode* node = dynamic_object_cast<SceneNode>(_nodeListener.targets()[index.row()]);
		if(node)
			return qVariantFromValue(node->objectTitle());
	}
	else if(role == Qt::UserRole) {
		SceneNode* node = dynamic_object_cast<SceneNode>(_nodeListener.targets()[index.row()]);
		return qVariantFromValue(node);
	}
	return QVariant();
}

/******************************************************************************
* This is called when a new dataset has been loaded.
******************************************************************************/
void SceneNodesListModel::onDataSetChanged(DataSet* newDataSet)
{
	beginResetModel();
	_nodeListener.clear();
	_rootNodeListener.setTarget(nullptr);
	if(newDataSet) {
		_rootNodeListener.setTarget(newDataSet->sceneRoot());
		newDataSet->sceneRoot()->visitChildren([this](SceneNode* node) -> bool {
			_nodeListener.push_back(node);
			return true;
		});
	}
	endResetModel();
}

/******************************************************************************
* This handles reference events generated by the root node.
******************************************************************************/
void SceneNodesListModel::onRootNodeNotificationEvent(ReferenceEvent* event)
{
	onNodeNotificationEvent(_rootNodeListener.target(), event);
}

/******************************************************************************
* This handles reference events generated by the scene nodes.
******************************************************************************/
void SceneNodesListModel::onNodeNotificationEvent(RefTarget* source, ReferenceEvent* event)
{
	// Whenever a new node is being inserted into the scene, add it to our internal list.
	if(event->type() == ReferenceEvent::ReferenceAdded) {
		ReferenceFieldEvent* refEvent = static_cast<ReferenceFieldEvent*>(event);
		if(refEvent->field() == PROPERTY_FIELD(SceneNode::_children)) {
			if(SceneNode* node = dynamic_object_cast<SceneNode>(refEvent->newTarget())) {
				beginInsertRows(QModelIndex(), _nodeListener.targets().size(), _nodeListener.targets().size());
				_nodeListener.push_back(node);
				endInsertRows();
				// Add all child nodes too.
				node->visitChildren([this](SceneNode* node) -> bool {
					beginInsertRows(QModelIndex(), _nodeListener.targets().size(), _nodeListener.targets().size());
					_nodeListener.push_back(node);
					endInsertRows();
					return true;
				});
			}
		}
	}

	// If a node is being removed from the scene, remove it from our internal list.
	if(event->type() == ReferenceEvent::ReferenceRemoved) {
		// Don't know how else to do this in a safe manner. Rebuild the entire model from scratch.
		onDataSetChanged(_datasetContainer.currentSet());
	}

	// If a node is being renamed, let the model emit an update signal.
	if(event->type() == ReferenceEvent::TitleChanged) {
		int index = _nodeListener.targets().indexOf(source);
		if(index >= 0) {
			QModelIndex modelIndex = createIndex(index, 0, source);
			dataChanged(modelIndex, modelIndex);
		}
	}
}


};