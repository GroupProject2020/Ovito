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

#pragma once


#include <ovito/gui/web/GUIWeb.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/oo/RefTargetListener.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/scene/SceneNode.h>
#include "PipelineListItem.h"

namespace Ovito {

/**
 * This Qt model class is used to populate the ListView item.
 */
class PipelineListModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(Ovito::RefTarget* selectedObject READ selectedObject NOTIFY selectedItemChanged);
	Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedItemChanged);

public:

	enum ItemRoles {
		TitleRole = Qt::UserRole + 1,
		ItemTypeRole,
		CheckedRole
	};

	/// Constructor.
	PipelineListModel(DataSetContainer& datasetContainer, QObject* parent);

	/// Returns the number of list items.
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override { return _items.size(); }

	/// Returns the data for an item of the model.
	virtual QVariant data(const QModelIndex& index, int role) const override;

	/// Changes the data associated with a list entry.
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override;

	/// Returns the model's role names.
	virtual QHash<int, QByteArray> roleNames() const override;

	/// Discards all list items.
	void clear() {
		if(_items.empty()) return;
		beginRemoveRows(QModelIndex(), 0, _items.size() - 1);
		_items.clear();
		_selectedPipeline.setTarget(nullptr);
		endRemoveRows();
		_needListUpdate = false;
	}

	/// Returns an item from the list model.
	PipelineListItem* item(int index) const {
		OVITO_ASSERT(index >= 0 && index < _items.size());
		return _items[index];
	}

	/// Populates the model with the given list items.
	void setItems(std::vector<OORef<PipelineListItem>> newItems);

	/// Returns the list of items.
	const std::vector<OORef<PipelineListItem>>& items() const { return _items; }

	/// Returns true if the list model is currently in a valid state.
	bool isUpToDate() const { return !_needListUpdate; }

	/// The list of currently selected PipelineSceneNode.
	PipelineSceneNode* selectedPipeline() const { return _selectedPipeline.target(); }

	/// Returns the index of the item that is currently selected in the pipeline editor.
	int selectedIndex() const { return _selectedIndex; }

	/// Sets the index of the item that is currently selected in the pipeline editor.
	void setSelectedIndex(int index) { 
		if(_selectedIndex != index) {
			_selectedIndex = index; 
			Q_EMIT selectedItemChanged();
		}
	}

	/// Returns the list item that is currently selected in the pipeline editor.
	PipelineListItem* selectedItem() const;

	/// Returns the RefTarget object from the pipeline that is currently selected in the pipeline editor.
	RefTarget* selectedObject() const;

	/// Returns the container of the dataset being edited.
	DataSetContainer& datasetContainer() { return _datasetContainer; }

    /// Inserts the given modifier(s) into the currently selected data pipeline.
	void applyModifiers(const QVector<OORef<Modifier>>& modifiers);

Q_SIGNALS:

	/// This signal is emitted if a different item has been selected in the pipeline editor, 
	/// or if the already selected item's state has changed.
	void selectedItemChanged();

public Q_SLOTS:

	/// Rebuilds the complete list immediately.
	void refreshList();

	/// Updates the appearance of a single list item.
	void refreshItem(PipelineListItem* item);

	/// Rebuilds the list of modification items as soon as possible.
	void requestUpdate() {
		if(_needListUpdate) return;	// Update is already pending.
		_needListUpdate = true;
		// Invoke actual refresh function at some later time.
		QMetaObject::invokeMethod(this, "refreshList", Qt::QueuedConnection);
	}

	/// Deletes the modifier at the given list index from the pipeline.
	void deleteModifier(int index);

private Q_SLOTS:

	/// Handles notification events generated by the selected pipeline node.
	void onPipelineEvent(const ReferenceEvent& event);

private:

	/// Create the pipeline editor entries for the subjects of the given object (and their subobjects).
	static void createListItemsForSubobjects(const DataObject* dataObj, std::vector<OORef<PipelineListItem>>& items, PipelineListItem* parentItem);

	/// List of visible items in the model.
	std::vector<OORef<PipelineListItem>> _items;

	/// Reference to the currently selected PipelineSceneNode.
	RefTargetListener<PipelineSceneNode> _selectedPipeline;

	/// The list item index that is currently selected in the pipeline editor.
	int _selectedIndex = -1;

	/// Indicates that the list of items needs to be updated.
	bool _needListUpdate = false;

	/// Container of the dataset being edited.
	DataSetContainer& _datasetContainer;
};

}	// End of namespace
