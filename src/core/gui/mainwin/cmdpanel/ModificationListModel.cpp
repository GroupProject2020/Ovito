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
#include <core/scene/objects/SceneObject.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/scene/pipeline/Modifier.h>
#include <core/scene/ObjectNode.h>
#include <core/viewport/ViewportManager.h>
#include <core/gui/actions/ActionManager.h>
#include "ModificationListModel.h"
#include "ModifyCommandPage.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
ModificationListModel::ModificationListModel(QObject* parent) : QAbstractListModel(parent),
	_nextToSelectObject(nullptr), _needListUpdate(false),
	_statusInfoIcon(":/core/mainwin/status/status_info.png"),
	_statusWarningIcon(":/core/mainwin/status/status_warning.png"),
	_statusErrorIcon(":/core/mainwin/status/status_error.png"),
	_statusNoneIcon(":/core/mainwin/status/status_none.png"),
	_statusPendingIcon(":/core/mainwin/status/status_pending.gif"),
	_sectionHeaderFont(QGuiApplication::font())
{
	connect(&_statusPendingIcon, SIGNAL(frameChanged(int)), this, SLOT(iconAnimationFrameChanged()));
	_selectionModel = new QItemSelectionModel(this);
	connect(_selectionModel, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SIGNAL(selectedItemChanged()));
	connect(&_selectedNodes, SIGNAL(notificationEvent(RefTarget*, ReferenceEvent*)), this, SLOT(onNodeEvent(RefTarget*, ReferenceEvent*)));
	if(_sectionHeaderFont.pixelSize() < 0)
		_sectionHeaderFont.setPointSize(_sectionHeaderFont.pointSize() * 4 / 5);
	else
		_sectionHeaderFont.setPixelSize(_sectionHeaderFont.pixelSize() * 4 / 5);
}

/******************************************************************************
* Populates the model with the given list items.
******************************************************************************/
void ModificationListModel::setItems(const QVector<OORef<ModificationListItem>>& newItems, const QVector<OORef<ModificationListItem>>& newHiddenItems)
{
	beginResetModel();
	_items = newItems;
	_hiddenItems = newHiddenItems;
	for(const auto& item : _items) {
		connect(item.get(), SIGNAL(itemChanged(ModificationListItem*)), this, SLOT(refreshItem(ModificationListItem*)));
		connect(item.get(), SIGNAL(subitemsChanged(ModificationListItem*)), this, SLOT(requestUpdate()));
	}
	for(const auto& item : _hiddenItems) {
		connect(item.get(), SIGNAL(itemChanged(ModificationListItem*)), this, SLOT(refreshItem(ModificationListItem*)));
		connect(item.get(), SIGNAL(subitemsChanged(ModificationListItem*)), this, SLOT(requestUpdate()));
	}
	endResetModel();
}

/******************************************************************************
* Returns the currently selected item in the modification list.
******************************************************************************/
ModificationListItem* ModificationListModel::selectedItem() const
{
	QModelIndexList selection = _selectionModel->selectedRows();
	if(selection.empty())
		return nullptr;
	else
		return item(selection.front().row());
}

/******************************************************************************
* Completely rebuilds the modifier list.
******************************************************************************/
void ModificationListModel::refreshList()
{
	_needListUpdate = false;
	UndoSuspender noUndo;

	// Determine the currently selected object and
	// select it again after the list has been rebuilt (and it is still there).
	// If _currentObject is already non-NULL then the caller
	// has specified an object to be selected.
	if(_nextToSelectObject == nullptr) {
		ModificationListItem* item = selectedItem();
		if(item)
			_nextToSelectObject = item->object();
	}

	// Collect all selected ObjectNodes.
	// Also check if all selected object nodes reference the same scene object.
	_selectedNodes.clear();
    SceneObject* cmnObject = nullptr;
	for(SceneNode* node : DataSetManager::instance().currentSelection()->nodes()) {
		if(node->isObjectNode()) {
			ObjectNode* objNode = static_object_cast<ObjectNode>(node);
			_selectedNodes.push_back(objNode);

			if(cmnObject == nullptr) cmnObject = objNode->sceneObject();
			else if(cmnObject != objNode->sceneObject()) {
				cmnObject = nullptr;
				break;	// The scene nodes are not compatible.
			}
		}
	}

	QVector<OORef<ModificationListItem>> items;
	QVector<OORef<ModificationListItem>> hiddenItems;
	if(cmnObject) {

		// Create list items for display objects.
		for(RefTarget* objNode : _selectedNodes.targets()) {
			for(DisplayObject* displayObj : static_object_cast<ObjectNode>(objNode)->displayObjects())
				items.push_back(new ModificationListItem(displayObj));
		}
		if(!items.empty())
			items.push_front(new ModificationListItem(nullptr, false, tr("Display")));

		// Walk up the pipeline.
		do {
			OVITO_CHECK_OBJECT_POINTER(cmnObject);

			// Create entries for the modifier applications if this is a PipelineObject.
			PipelineObject* modObj = dynamic_object_cast<PipelineObject>(cmnObject);
			if(modObj) {

				if(!modObj->modifierApplications().empty())
					items.push_back(new ModificationListItem(nullptr, false, tr("Modifications")));

				hiddenItems.push_back(new ModificationListItem(modObj));

				for(int i = modObj->modifierApplications().size(); i--; ) {
					ModifierApplication* app = modObj->modifierApplications()[i];
					ModificationListItem* item = new ModificationListItem(app->modifier());
					item->setModifierApplications({1, app});
					items.push_back(item);
				}
			}
			else {

				items.push_back(new ModificationListItem(nullptr, false, tr("Input")));

				// Create an entry for the scene object.
				items.push_back(new ModificationListItem(cmnObject));
				if(_nextToSelectObject == nullptr)
					_nextToSelectObject = cmnObject;

				// Create list items for the object's editable sub-objects.
				for(int i = 0; i < cmnObject->editableSubObjectCount(); i++) {
					RefTarget* subobject = cmnObject->editableSubObject(i);
					if(subobject != NULL && subobject->isSubObjectEditable()) {
						items.push_back(new ModificationListItem(subobject, true));
					}
				}
			}

			// In case the current object has multiple input slots, determine if they all point to the same input object.
			SceneObject* nextObj = nullptr;
			for(int i = 0; i < cmnObject->inputObjectCount(); i++) {
				if(!nextObj)
					nextObj = cmnObject->inputObject(i);
				else if(nextObj != cmnObject->inputObject(i)) {
					nextObj = nullptr;  // The input objects do not match.
					break;
				}
			}
			cmnObject = nextObj;
		}
		while(cmnObject != nullptr);
	}

	int selIndex = 0;
	for(int index = 0; index < items.size(); index++) {
		if(_nextToSelectObject == items[index]->object()) {
			selIndex = index;
			break;
		}
	}
	setItems(items, hiddenItems);
	_nextToSelectObject = nullptr;

	// Select the right item in the list box.
	if(!items.empty())
		_selectionModel->select(index(selIndex), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
}

/******************************************************************************
* Handles notification events generated by the selected object nodes.
******************************************************************************/
void ModificationListModel::onNodeEvent(RefTarget* source, ReferenceEvent* event)
{
	// Update the entire modification list if the ObjectNode has been assigned a new
	// scene object, or if the list of display objects has changed.
	if(event->type() == ReferenceEvent::ReferenceChanged || event->type() == ReferenceEvent::ReferenceAdded || event->type() == ReferenceEvent::ReferenceRemoved) {
		requestUpdate();
	}
}

/******************************************************************************
* Updates the appearance of a single list item.
******************************************************************************/
void ModificationListModel::refreshItem(ModificationListItem* item)
{
	OVITO_CHECK_OBJECT_POINTER(item);
	int i = _items.indexOf(item);
	if(i != -1) {
		Q_EMIT dataChanged(index(i), index(i));

		// Also update available actions if the changed item is currently selected.
		if(selectedItem() == item)
			Q_EMIT selectedItemChanged();
	}
}

/******************************************************************************
* Inserts the given modifier into the modification pipeline of the
* selected scene nodes.
******************************************************************************/
void ModificationListModel::applyModifier(Modifier* modifier)
{
	// Get the selected stack entry. The new modifier is inserted just behind it.
	ModificationListItem* currentItem = selectedItem();

	// On the next list update, the new modifier should be selected.
	_nextToSelectObject = modifier;

	if(currentItem) {
		if(dynamic_object_cast<Modifier>(currentItem->object())) {
			for(ModifierApplication* modApp : currentItem->modifierApplications()) {
				PipelineObject* pipelineObj = modApp->pipelineObject();
				OVITO_CHECK_OBJECT_POINTER(pipelineObj);
				pipelineObj->insertModifier(modifier, pipelineObj->modifierApplications().indexOf(modApp) + 1);
			}
			return;
		}
		else if(dynamic_object_cast<PipelineObject>(currentItem->object())) {
			PipelineObject* pipelineObj = static_object_cast<PipelineObject>(currentItem->object());
			OVITO_CHECK_OBJECT_POINTER(pipelineObj);
			pipelineObj->insertModifier(modifier, 0);
			return;
		}
		else if(dynamic_object_cast<SceneObject>(currentItem->object())) {
			for(int i = _hiddenItems.size() - 1; i >= 0; i--) {
				PipelineObject* pipelineObj = dynamic_object_cast<PipelineObject>(_hiddenItems[i]->object());
				if(pipelineObj) {
					OVITO_CHECK_OBJECT_POINTER(pipelineObj);
					pipelineObj->insertModifier(modifier, 0);
					return;
				}
			}
		}
	}

	// Apply modifier to each selected node.
	for(RefTarget* objNode : _selectedNodes.targets()) {
		static_object_cast<ObjectNode>(objNode)->applyModifier(modifier);
	}
}

/******************************************************************************
* Is called by the system when the animated status icon changed.
******************************************************************************/
void ModificationListModel::iconAnimationFrameChanged()
{
	bool stopMovie = true;
	for(int i = 0; i < _items.size(); i++) {
		if(_items[i]->status() == ModificationListItem::Pending) {
			dataChanged(index(i), index(i), { Qt::DecorationRole });
			stopMovie = false;
		}
	}
	if(stopMovie)
		_statusPendingIcon.stop();
}

/******************************************************************************
* Returns the data for the QListView widget.
******************************************************************************/
QVariant ModificationListModel::data(const QModelIndex& index, int role) const
{
	OVITO_ASSERT(index.row() >= 0 && index.row() < _items.size());

	ModificationListItem* item = this->item(index.row());

	if(role == Qt::DisplayRole) {
		if(item->object()) {
			if(item->isSubObject())
				return "   " + item->object()->objectTitle();
			else
				return item->object()->objectTitle();
		}
		else return item->title();
	}
	else if(role == Qt::DecorationRole) {
		if(item->object()) {
			switch(item->status()) {
			case ModificationListItem::Info: return qVariantFromValue(_statusInfoIcon);
			case ModificationListItem::Warning: return qVariantFromValue(_statusWarningIcon);
			case ModificationListItem::Error: return qVariantFromValue(_statusErrorIcon);
			case ModificationListItem::Pending:
				const_cast<QMovie&>(_statusPendingIcon).start();
				return qVariantFromValue(_statusPendingIcon.currentImage());
			case ModificationListItem::None: return qVariantFromValue(_statusNoneIcon);
			}
		}
	}
	else if(role == Qt::ToolTipRole) {
		return item->toolTip();
	}
	else if(role == Qt::CheckStateRole) {
		DisplayObject* displayObj = dynamic_object_cast<DisplayObject>(item->object());
		if(displayObj)
			return displayObj->isEnabled() ? Qt::Checked : Qt::Unchecked;
		Modifier* modifier = dynamic_object_cast<Modifier>(item->object());
		if(modifier)
			return modifier->isEnabled() ? Qt::Checked : Qt::Unchecked;
	}
	else if(role == Qt::TextAlignmentRole) {
		if(item->object() == nullptr) {
			return Qt::AlignCenter;
		}
	}
	else if(role == Qt::BackgroundRole) {
		if(item->object() == nullptr) {
			return QBrush(Qt::lightGray, Qt::Dense4Pattern);
		}
	}
	else if(role == Qt::ForegroundRole) {
		if(item->object() == nullptr) {
			return QBrush(Qt::blue);
		}
	}
	else if(role == Qt::FontRole) {
		if(item->object() == nullptr) {
			return _sectionHeaderFont;
		}
	}

	return QVariant();
}

/******************************************************************************
* Changes the data associated with a list entry.
******************************************************************************/
bool ModificationListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if(role == Qt::CheckStateRole) {
		ModificationListItem* item = this->item(index.row());
		DisplayObject* displayObj = dynamic_object_cast<DisplayObject>(item->object());
		if(displayObj) {
			UndoManager::instance().beginCompoundOperation(tr("Enable/disable display"));
			try {
				displayObj->setEnabled(value == Qt::Checked);
			}
			catch(const Exception& ex) {
				ex.showError();
				UndoManager::instance().currentCompoundOperation()->clear();
			}
			UndoManager::instance().endCompoundOperation();
		}
		else {
			Modifier* modifier = dynamic_object_cast<Modifier>(item->object());
			if(modifier) {
				UndoManager::instance().beginCompoundOperation(tr("Enable/disable modifier"));
				try {
					modifier->setEnabled(value == Qt::Checked);
				}
				catch(const Exception& ex) {
					ex.showError();
					UndoManager::instance().currentCompoundOperation()->clear();
				}
				UndoManager::instance().endCompoundOperation();
			}
		}
	}
	return QAbstractListModel::setData(index, value, role);
}

/******************************************************************************
* Returns the flags for an item.
******************************************************************************/
Qt::ItemFlags ModificationListModel::flags(const QModelIndex& index) const
{
	OVITO_ASSERT(index.row() >= 0 && index.row() < _items.size());
	ModificationListItem* item = this->item(index.row());
	if(item->object() == nullptr) {
		return Qt::NoItemFlags;
	}
	else {
		if(dynamic_object_cast<DisplayObject>(item->object()) || dynamic_object_cast<Modifier>(item->object())) {
			return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
		}
	}
	return QAbstractListModel::flags(index);
}

};