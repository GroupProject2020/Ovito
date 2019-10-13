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

#include <ovito/gui/GUI.h>
#include <ovito/core/dataset/data/DataObject.h>
#include <ovito/core/dataset/data/DataVis.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/gui/actions/ActionManager.h>
#include "OverlayListModel.h"
#include "ModifyCommandPage.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Constructor.
******************************************************************************/
OverlayListModel::OverlayListModel(QObject* parent) : QAbstractListModel(parent),
	_statusInfoIcon(":/gui/mainwin/status/status_info.png"),
	_statusWarningIcon(":/gui/mainwin/status/status_warning.png"),
	_statusErrorIcon(":/gui/mainwin/status/status_error.png"),
	_statusNoneIcon(":/gui/mainwin/status/status_none.png")
{
	_selectionModel = new QItemSelectionModel(this);
	connect(_selectionModel, &QItemSelectionModel::selectionChanged, this, &OverlayListModel::selectedItemChanged);
	connect(&_selectedViewport, &RefTargetListener<Viewport>::notificationEvent, this, &OverlayListModel::onViewportEvent);
}

/******************************************************************************
* Populates the model with the given list items.
******************************************************************************/
void OverlayListModel::setItems(const QList<OORef<OverlayListItem>>& newItems)
{
	beginResetModel();
	_items = newItems;
	for(OverlayListItem* item : _items) {
		connect(item, &OverlayListItem::itemChanged, this, &OverlayListModel::refreshItem);
	}
	endResetModel();
}

/******************************************************************************
* Returns the currently selected model item in the list.
******************************************************************************/
OverlayListItem* OverlayListModel::selectedItem() const
{
	QModelIndexList selection = _selectionModel->selectedRows();
	if(selection.empty())
		return nullptr;
	else
		return item(selection.front().row());
}

/******************************************************************************
* Returns the currently selected index in the overlay list.
******************************************************************************/
int OverlayListModel::selectedIndex() const
{
	QModelIndexList selection = _selectionModel->selectedRows();
	if(selection.empty())
		return -1;
	else
		return selection.front().row();
}

/******************************************************************************
* Rebuilds the viewport overlay list.
******************************************************************************/
void OverlayListModel::refreshList()
{
	// Determine the currently selected object and
	// select it again after the list has been rebuilt (and it is still there).
	// If _nextObjectToSelect is already non-null then the caller
	// has specified an object to be selected.
	if(_nextObjectToSelect == nullptr) {
		if(OverlayListItem* item = selectedItem())
			_nextObjectToSelect = item->overlay();
	}
	ViewportOverlay* defaultObjectToSelect = nullptr;

	// Create list items.
	QList<OORef<OverlayListItem>> items;
	if(selectedViewport()) {
		for(ViewportOverlay* overlay : selectedViewport()->overlays())
			items.push_back(new OverlayListItem(overlay));
	}
	int selIndex = -1;
	int selDefaultIndex = -1;
	for(int i = 0; i < items.size(); i++) {
		if(_nextObjectToSelect && _nextObjectToSelect == items[i]->overlay())
			selIndex = i;
		if(defaultObjectToSelect && defaultObjectToSelect == items[i]->overlay())
			selDefaultIndex = i;
	}
	if(selIndex == -1)
		selIndex = selDefaultIndex;
	setItems(items);

	_nextObjectToSelect = nullptr;

	// Select the proper item in the list box.
	if(!items.empty()) {
		if(selIndex == -1)
			selIndex = items.size()-1;
		_selectionModel->select(index(selIndex), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
	}
	else {
		Q_EMIT selectedItemChanged();
	}
}

/******************************************************************************
* Handles notification events generated by the active viewport.
******************************************************************************/
void OverlayListModel::onViewportEvent(const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::ReferenceAdded || event.type() == ReferenceEvent::ReferenceRemoved) {
		refreshList();
	}
}

/******************************************************************************
* Updates the appearance of a single list item.
******************************************************************************/
void OverlayListModel::refreshItem(OverlayListItem* item)
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
* Returns the data for the QListView widget.
******************************************************************************/
QVariant OverlayListModel::data(const QModelIndex& index, int role) const
{
	OVITO_ASSERT(index.row() >= 0 && index.row() < _items.size());
	OverlayListItem* item = this->item(index.row());

	if(role == Qt::DisplayRole) {
		return item->title();
	}
	else if(role == Qt::DecorationRole) {
		switch(item->status().type()) {
		case PipelineStatus::Warning: return QVariant::fromValue(_statusWarningIcon);
		case PipelineStatus::Error: return QVariant::fromValue(_statusErrorIcon);
		default: return QVariant::fromValue(_statusNoneIcon);
		}
	}
	else if(role == Qt::ToolTipRole) {
		return QVariant::fromValue(item->status().text());
	}
	else if(role == Qt::CheckStateRole) {
		return (item->overlay() && item->overlay()->isEnabled()) ? Qt::Checked : Qt::Unchecked;
	}
	return {};
}

/******************************************************************************
* Changes the data associated with a list entry.
******************************************************************************/
bool OverlayListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if(role == Qt::CheckStateRole) {
		OverlayListItem* item = this->item(index.row());
		if(ViewportOverlay* overlay = item->overlay()) {
			UndoableTransaction::handleExceptions(overlay->dataset()->undoStack(),
					(value == Qt::Checked) ? tr("Enable visual element") : tr("Disable visual element"), [overlay, &value]() {
				overlay->setEnabled(value == Qt::Checked);
			});
		}
	}
	return QAbstractListModel::setData(index, value, role);
}

/******************************************************************************
* Returns the flags for an item.
******************************************************************************/
Qt::ItemFlags OverlayListModel::flags(const QModelIndex& index) const
{
	if(index.row() >= 0 && index.row() < _items.size()) {
		OverlayListItem* item = this->item(index.row());
		return item->overlay() ? (QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable) : Qt::NoItemFlags;
	}
	return QAbstractListModel::flags(index);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
