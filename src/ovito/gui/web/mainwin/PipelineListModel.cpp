////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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

#include <ovito/gui/web/GUIWeb.h>
#include <ovito/core/dataset/data/DataObject.h>
#include <ovito/core/dataset/data/DataVis.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include "PipelineListModel.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
PipelineListModel::PipelineListModel(DataSetContainer& datasetContainer, QObject* parent) : QAbstractListModel(parent),
	_datasetContainer(datasetContainer)
{
	connect(&_selectedPipeline, &RefTargetListener<PipelineSceneNode>::notificationEvent, this, &PipelineListModel::onPipelineEvent);
	connect(&_datasetContainer, &DataSetContainer::selectionChangeComplete, this, &PipelineListModel::refreshList);
}

/******************************************************************************
* Returns the list item that is currently selected in the pipeline editor.
******************************************************************************/
PipelineListItem* PipelineListModel::selectedItem() const
{
	if(_selectedIndex >= 0 && _selectedIndex < _items.size())
		return item(_selectedIndex);
	return nullptr;
}

/******************************************************************************
* Returns the RefTarget object from the pipeline that is currently selected 
* in the pipeline editor.
******************************************************************************/
RefTarget* PipelineListModel::selectedObject() const
{
	if(PipelineListItem* item = selectedItem())
		return item->object();
	return nullptr;
}

/******************************************************************************
* Populates the model with the given list items.
******************************************************************************/
void PipelineListModel::setItems(std::vector<OORef<PipelineListItem>> newItems)
{
	size_t oldCount = _items.size();
	if(newItems.size() > oldCount) {
		beginInsertRows(QModelIndex(), oldCount, newItems.size() - 1);
		_items.insert(_items.end(), std::make_move_iterator(newItems.begin() + oldCount), std::make_move_iterator(newItems.end()));
		endInsertRows();
	}
	else if(newItems.size() < oldCount) {
		beginRemoveRows(QModelIndex(), newItems.size(), oldCount - 1);
		_items.erase(_items.begin() + newItems.size(), _items.end());
		endRemoveRows();
	}
	for(size_t i = 0; i < newItems.size() && i < oldCount; i++) {
		swap(_items[i], newItems[i]);
		if(_items[i]->object() != newItems[i]->object() || _items[i]->itemType() != newItems[i]->itemType()) {
			Q_EMIT dataChanged(index(i), index(i));
		}
	}
	for(PipelineListItem* item : _items) {
		connect(item, &PipelineListItem::itemChanged, this, &PipelineListModel::refreshItem);
		connect(item, &PipelineListItem::subitemsChanged, this, &PipelineListModel::requestUpdate);
	}
}

/******************************************************************************
* Completely rebuilds the pipeline list.
******************************************************************************/
void PipelineListModel::refreshList()
{
	_needListUpdate = false;

	// Determine the selected pipeline.
	_selectedPipeline.setTarget(nullptr);
    if(_datasetContainer.currentSet()) {
		SelectionSet* selectionSet = _datasetContainer.currentSet()->selection();
		_selectedPipeline.setTarget(dynamic_object_cast<PipelineSceneNode>(selectionSet->firstNode()));
    }

	std::vector<OORef<PipelineListItem>> newItems;
	if(selectedPipeline()) {

		// Create list items for visualization elements.
		for(DataVis* vis : selectedPipeline()->visElements())
			newItems.push_back(new PipelineListItem(vis, PipelineListItem::VisualElement));
		if(!newItems.empty())
			newItems.insert(newItems.begin(), new PipelineListItem(nullptr, PipelineListItem::VisualElementsHeader));

		// Traverse the modifiers in the pipeline.
		PipelineObject* pipelineObject = selectedPipeline()->dataProvider();
		PipelineObject* firstPipelineObj = pipelineObject;
		while(pipelineObject) {

			// Create entries for the modifier applications.
			if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(pipelineObject)) {

				if(pipelineObject == firstPipelineObj)
					newItems.push_back(new PipelineListItem(nullptr, PipelineListItem::ModificationsHeader));

				if(pipelineObject->isPipelineBranch(true))
					newItems.push_back(new PipelineListItem(nullptr, PipelineListItem::PipelineBranch));

				PipelineListItem* item = new PipelineListItem(modApp, PipelineListItem::Modifier);
				newItems.push_back(item);

				pipelineObject = modApp->input();
			}
			else if(pipelineObject) {

				if(pipelineObject->isPipelineBranch(true))
					newItems.push_back(new PipelineListItem(nullptr, PipelineListItem::PipelineBranch));

				newItems.push_back(new PipelineListItem(nullptr, PipelineListItem::DataSourceHeader));

				// Create a list item for the data source.
				PipelineListItem* item = new PipelineListItem(pipelineObject, PipelineListItem::DataObject);
				newItems.push_back(item);

				// Create list items for the source's editable data objects.
				if(const DataCollection* collection = pipelineObject->getSourceDataCollection())
					createListItemsForSubobjects(collection, newItems, item);

				// Done.
				break;
			}
		}
	}

	setItems(std::move(newItems));
}

/******************************************************************************
* Create the pipeline editor entries for the subjects of the given
* object (and their subobjects).
******************************************************************************/
void PipelineListModel::createListItemsForSubobjects(const DataObject* dataObj, std::vector<OORef<PipelineListItem>>& items, PipelineListItem* parentItem)
{
	if(dataObj->showInPipelineEditor())
		items.push_back(parentItem = new PipelineListItem(const_cast<DataObject*>(dataObj), PipelineListItem::DataSubObject, parentItem));

	// Recursively visit the sub-objects of the object.
	dataObj->visitSubObjects([&](const DataObject* subObject) {
		createListItemsForSubobjects(subObject, items, parentItem);
		return false;
	});
}

/******************************************************************************
* Handles notification events generated by the selected pipeline node.
******************************************************************************/
void PipelineListModel::onPipelineEvent(const ReferenceEvent& event)
{
	// Update the entire modification list if the PipelineSceneNode has been assigned a new
	// data object, or if the list of visual elements has changed.
	if(event.type() == ReferenceEvent::ReferenceChanged
		|| event.type() == ReferenceEvent::ReferenceAdded
		|| event.type() == ReferenceEvent::ReferenceRemoved
		|| event.type() == ReferenceEvent::PipelineChanged)
	{
		requestUpdate();
	}
}

/******************************************************************************
* Updates the appearance of a single list item.
******************************************************************************/
void PipelineListModel::refreshItem(PipelineListItem* item)
{
	OVITO_CHECK_OBJECT_POINTER(item);
	auto iter = std::find(items().begin(), items().end(), item);
	if(iter != items().end()) {
		int i = iter - items().begin();
		Q_EMIT dataChanged(index(i), index(i));
	}
}

/******************************************************************************
* Returns the data for an item of the model.
******************************************************************************/
QVariant PipelineListModel::data(const QModelIndex& index, int role) const
{
	OVITO_ASSERT(index.row() >= 0 && index.row() < _items.size());

	PipelineListItem* item = this->item(index.row());

	if(role == TitleRole) {
		return item->title();
	}
	else if(role == ItemTypeRole) {
		return item->itemType();
	}
	else if(role == CheckedRole) {
		if(DataVis* vis = dynamic_object_cast<DataVis>(item->object()))
			return vis->isEnabled() ? Qt::Checked : Qt::Unchecked;
		else if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(item->object()))
			return (modApp->modifier() && modApp->modifier()->isEnabled()) ? Qt::Checked : Qt::Unchecked;
		else 
			return false;
	}

	return {};
}

/******************************************************************************
* Changes the data associated with a list entry.
******************************************************************************/
bool PipelineListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if(role == CheckedRole) {
		PipelineListItem* item = this->item(index.row());
		if(DataVis* vis = dynamic_object_cast<DataVis>(item->object())) {
			UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(),
					(value == Qt::Checked) ? tr("Enable visual element") : tr("Disable visual element"), [vis, &value]() {
				vis->setEnabled(value.toBool());
			});
		}
		else if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(item->object())) {
			UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(),
					(value == Qt::Checked) ? tr("Enable modifier") : tr("Disable modifier"), [modApp, &value]() {
				if(modApp->modifier()) modApp->modifier()->setEnabled(value.toBool());
			});
		}
	}
	return QAbstractListModel::setData(index, value, role);
}

/******************************************************************************
* Returns the model's role names.
******************************************************************************/
QHash<int, QByteArray> PipelineListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[TitleRole] = "title";
	roles[ItemTypeRole] = "type";
	roles[CheckedRole] = "ischecked";
	return roles;
}

/******************************************************************************
* Inserts the given modifier(s) into the currently selected data pipeline.
******************************************************************************/
void PipelineListModel::applyModifiers(const QVector<OORef<Modifier>>& modifiers)
{
	if(modifiers.empty())
		return;

	// Insert modifiers at the end of the selected pipeline.
	for(int index = modifiers.size() - 1; index >= 0; --index) {
		selectedPipeline()->applyModifier(modifiers[index]);
	}
}

/******************************************************************************
* Deletes the modifier at the given list index from the pipeline.
******************************************************************************/
void PipelineListModel::deleteModifier(int index)
{
	// Get the modifier list item.
	PipelineListItem* selectedItem = item(index);

	OORef<ModifierApplication> modApp = dynamic_object_cast<ModifierApplication>(selectedItem->object());
	if(!modApp) return;

	UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Delete modifier"), [modApp, this]() {
		auto dependentsList = modApp->dependents();
		for(RefMaker* dependent : dependentsList) {
			if(ModifierApplication* precedingModApp = dynamic_object_cast<ModifierApplication>(dependent)) {
				if(precedingModApp->input() == modApp) {
					precedingModApp->setInput(modApp->input());
				}
			}
			else if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(dependent)) {
				if(pipeline->dataProvider() == modApp) {
					pipeline->setDataProvider(modApp->input());
				}
			}
		}
		OORef<Modifier> modifier = modApp->modifier();
		modApp->setInput(nullptr);
		modApp->setModifier(nullptr);

		// Delete modifier if there are no more applications left.
		if(modifier->modifierApplications().empty())
			modifier->deleteReferenceObject();
	});	
}

}	// End of namespace
