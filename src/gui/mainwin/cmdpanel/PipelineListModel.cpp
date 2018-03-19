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

#include <gui/GUI.h>
#include <core/dataset/data/DataObject.h>
#include <core/dataset/data/DataVis.h>
#include <core/dataset/pipeline/PipelineObject.h>
#include <core/dataset/pipeline/Modifier.h>
#include <core/dataset/scene/PipelineSceneNode.h>
#include <core/dataset/scene/SelectionSet.h>
#include <core/dataset/DataSetContainer.h>
#include <gui/actions/ActionManager.h>
#include "PipelineListModel.h"
#include "ModifyCommandPage.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Constructor.
******************************************************************************/
PipelineListModel::PipelineListModel(DataSetContainer& datasetContainer, QObject* parent) : QAbstractListModel(parent),
	_datasetContainer(datasetContainer),
	_nextObjectToSelect(nullptr), _needListUpdate(false),
	_statusInfoIcon(":/gui/mainwin/status/status_info.png"),
	_statusWarningIcon(":/gui/mainwin/status/status_warning.png"),
	_statusErrorIcon(":/gui/mainwin/status/status_error.png"),
	_statusNoneIcon(":/gui/mainwin/status/status_none.png"),
	_statusPendingIcon(":/gui/mainwin/status/status_pending.gif"),
	_sectionHeaderFont(QGuiApplication::font())
{
	_statusPendingIcon.setCacheMode(QMovie::CacheAll);
	connect(&_statusPendingIcon, &QMovie::frameChanged, this, &PipelineListModel::iconAnimationFrameChanged);
	_selectionModel = new QItemSelectionModel(this);
	connect(_selectionModel, &QItemSelectionModel::selectionChanged, this, &PipelineListModel::selectedItemChanged);
	connect(&_selectedNodes, &VectorRefTargetListener<PipelineSceneNode>::notificationEvent, this, &PipelineListModel::onNodeEvent);
	if(_sectionHeaderFont.pixelSize() < 0)
		_sectionHeaderFont.setPointSize(_sectionHeaderFont.pointSize() * 4 / 5);
	else
		_sectionHeaderFont.setPixelSize(_sectionHeaderFont.pixelSize() * 4 / 5);
	_sectionHeaderBackgroundBrush = QBrush(Qt::lightGray, Qt::Dense4Pattern);
	_sectionHeaderForegroundBrush = QBrush(Qt::blue);

	_sharedObjectFont.setItalic(true);
}

/******************************************************************************
* Populates the model with the given list items.
******************************************************************************/
void PipelineListModel::setItems(const QList<OORef<PipelineListItem>>& newItems)
{
	beginResetModel();
	_items = newItems;
	for(PipelineListItem* item : _items) {
		connect(item, &PipelineListItem::itemChanged, this, &PipelineListModel::refreshItem);
		connect(item, &PipelineListItem::subitemsChanged, this, &PipelineListModel::requestUpdate);
	}
	endResetModel();
}

/******************************************************************************
* Returns the currently selected item in the modification list.
******************************************************************************/
PipelineListItem* PipelineListModel::selectedItem() const
{
	QModelIndexList selection = _selectionModel->selectedRows();
	if(selection.empty())
		return nullptr;
	else
		return item(selection.front().row());
}

/******************************************************************************
* Completely rebuilds the pipeline list.
******************************************************************************/
void PipelineListModel::refreshList()
{
	_needListUpdate = false;

	// Determine the currently selected object and
	// select it again after the list has been rebuilt (and it is still there).
	// If _nextObjectToSelect is already non-null then the caller
	// has specified an object to be selected.
	if(_nextObjectToSelect == nullptr) {
		if(PipelineListItem* item = selectedItem())
			_nextObjectToSelect = item->object();
	}
	RefTarget* defaultObjectToSelect = nullptr;

	// Collect all selected pipeline nodes.
	// Also check if all selected pipeline nodes reference the same pipeline.
	_selectedNodes.clear();
    PipelineObject* cmnObject = nullptr;

    if(_datasetContainer.currentSet()) {
		for(SceneNode* node : _datasetContainer.currentSet()->selection()->nodes()) {
			if(PipelineSceneNode* objNode = dynamic_object_cast<PipelineSceneNode>(node)) {
				_selectedNodes.push_back(objNode);

				if(cmnObject == nullptr) cmnObject = objNode->dataProvider();
				else if(cmnObject != objNode->dataProvider()) {
					cmnObject = nullptr;
					break;	// The scene nodes are not compatible.
				}
			}
		}
    }

	QList<OORef<PipelineListItem>> items;
	if(cmnObject) {

		// Create list items for visualization elements.
		for(PipelineSceneNode* pipeline : _selectedNodes.targets()) {
			for(DataVis* vis : pipeline->visElements())
				items.push_back(new PipelineListItem(vis, PipelineListItem::Object));
		}
		if(!items.empty())
			items.push_front(new PipelineListItem(nullptr, PipelineListItem::VisualElementsHeader));

		// Traverse the modifiers in the pipeline.
		PipelineObject* firstPipelineObj = cmnObject;
		do {
			OVITO_CHECK_OBJECT_POINTER(cmnObject);

			// Create entries for the modifier applications.
			if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(cmnObject)) {

				if(cmnObject == firstPipelineObj)
					items.push_back(new PipelineListItem(nullptr, PipelineListItem::ModificationsHeader));

				if(cmnObject->isPipelineBranch(true))
					items.push_back(new PipelineListItem(nullptr, PipelineListItem::PipelineBranch));

				PipelineListItem* item = new PipelineListItem(modApp, PipelineListItem::Object);
				items.push_back(item);

				cmnObject = modApp->input();
			}
			else if(cmnObject) {

				if(cmnObject->isPipelineBranch(true))
					items.push_back(new PipelineListItem(nullptr, PipelineListItem::PipelineBranch));
				
				items.push_back(new PipelineListItem(nullptr, PipelineListItem::DataSourceHeader));

				// Create an entry for the data object.
				PipelineListItem* item = new PipelineListItem(cmnObject, PipelineListItem::Object);
				items.push_back(item);
				if(defaultObjectToSelect == nullptr)
					defaultObjectToSelect = cmnObject;

				// Create list items for the object's editable sub-objects.
				for(int i = 0; i < cmnObject->editableSubObjectCount(); i++) {
					RefTarget* subobject = cmnObject->editableSubObject(i);
					if(subobject != nullptr && subobject->isSubObjectEditable()) {
						items.push_back(new PipelineListItem(subobject, PipelineListItem::SubObject, item));
					}
				}

				break;
			}
		}
		while(cmnObject);
	}

	int selIndex = -1;
	int selDefaultIndex = -1;
	for(int i = 0; i < items.size(); i++) {
		if(_nextObjectToSelect && _nextObjectToSelect == items[i]->object())
			selIndex = i;
		if(defaultObjectToSelect && defaultObjectToSelect == items[i]->object())
			selDefaultIndex = i;
	}
	if(selIndex == -1)
		selIndex = selDefaultIndex;

	setItems(items);
	_nextObjectToSelect = nullptr;

	// Select the proper item in the list box.
	if(!items.empty()) {
		if(selIndex == -1) {
			for(int index = 0; index < items.size(); index++) {
				if(items[index]->object()) {
					selIndex = index;
					break;
				}
			}
		}
		_selectionModel->select(index(selIndex), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
	}
	else {
		Q_EMIT selectedItemChanged();
	}
}

/******************************************************************************
* Handles notification events generated by the selected object nodes.
******************************************************************************/
void PipelineListModel::onNodeEvent(RefTarget* source, const ReferenceEvent& event)
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
* scurrently elected scene nodes.
******************************************************************************/
void PipelineListModel::applyModifiers(const QVector<OORef<Modifier>>& modifiers)
{
	if(modifiers.empty())
		return;

	// Get the selected pipeline entry. The new modifier is inserted right behind it in the pipeline.
	PipelineListItem* currentItem = selectedItem();

	_nextObjectToSelect = nullptr;
	if(currentItem) {
		while(currentItem->parent()) {
			currentItem = currentItem->parent();
		}
		if(OORef<PipelineObject> pobj = dynamic_object_cast<PipelineObject>(currentItem->object())) {
			for(Modifier* modifier : modifiers) {
				auto dependentsList = pobj->dependents();
				OORef<ModifierApplication> modApp = modifier->createModifierApplication();
				modApp->setModifier(modifier);
				modApp->setInput(pobj);
				modifier->initializeModifier(modApp);
				if(!_nextObjectToSelect) _nextObjectToSelect = modApp;
				for(RefMaker* dependent : dependentsList) {
					if(ModifierApplication* predecessorModApp = dynamic_object_cast<ModifierApplication>(dependent)) {
						predecessorModApp->setInput(modApp);
					}
					else if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(dependent)) {
						pipeline->setDataProvider(modApp);
					}
				}
				pobj = modApp;
			}
			return;
		}
	}

	// Insert modifiers at the end of the selected pipelines.
	for(PipelineSceneNode* pipeline : selectedNodes()) {
		for(int index = modifiers.size() - 1; index >= 0; --index) {
			pipeline->applyModifier(modifiers[index]);
		}
	}
}

/******************************************************************************
* Is called by the system when the animated status icon changed.
******************************************************************************/
void PipelineListModel::iconAnimationFrameChanged()
{
	bool stopMovie = true;
	for(int i = 0; i < _items.size(); i++) {
		if(_items[i]->status().type() == PipelineStatus::Pending) {
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
QVariant PipelineListModel::data(const QModelIndex& index, int role) const
{
	OVITO_ASSERT(index.row() >= 0 && index.row() < _items.size());

	PipelineListItem* item = this->item(index.row());

	if(role == Qt::DisplayRole) {
		return item->title();
	}
	else if(role == Qt::DecorationRole) {
		if(item->object()) {
			switch(item->status().type()) {
			case PipelineStatus::Warning: return QVariant::fromValue(_statusWarningIcon);
			case PipelineStatus::Error: return QVariant::fromValue(_statusErrorIcon);
			case PipelineStatus::Pending:
				const_cast<QMovie&>(_statusPendingIcon).start();
				return QVariant::fromValue(_statusPendingIcon.currentPixmap());
			default: return QVariant::fromValue(_statusNoneIcon);
			}
		}
	}
	else if(role == Qt::ToolTipRole) {
		return QVariant::fromValue(item->status().text());
	}
	else if(role == Qt::CheckStateRole) {
		if(DataVis* vis = dynamic_object_cast<DataVis>(item->object()))
			return vis->isEnabled() ? Qt::Checked : Qt::Unchecked;
		else if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(item->object()))
			return modApp->modifier()->isEnabled() ? Qt::Checked : Qt::Unchecked;
	}
	else if(role == Qt::TextAlignmentRole) {
		if(item->object() == nullptr) {
			return Qt::AlignCenter;
		}
	}
	else if(role == Qt::BackgroundRole) {
		if(item->object() == nullptr) {
			if(item->itemType() != PipelineListItem::PipelineBranch)
				return _sectionHeaderBackgroundBrush;
			else
				return QBrush(Qt::lightGray, Qt::Dense6Pattern);
		}
	}
	else if(role == Qt::ForegroundRole) {
		if(item->object() == nullptr) {
			return _sectionHeaderForegroundBrush;
		}
	}
	else if(role == Qt::FontRole) {
		if(item->object() == nullptr) {
			return _sectionHeaderFont;
		}
		else if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(item->object())) {
			if(modApp->modifier()) {
				QSet<PipelineSceneNode*> pipelines;
				for(ModifierApplication* modApp : modApp->modifier()->modifierApplications())
					pipelines.unite(modApp->pipelines(true));
				if(pipelines.size() > 1)
					return _sharedObjectFont;
			}
		}
		else if(PipelineObject* pipelineObject = dynamic_object_cast<PipelineObject>(item->object())) {
			if(pipelineObject->pipelines(true).size() > 1)
				return _sharedObjectFont;
		}
		else if(DataVis* visElement = dynamic_object_cast<DataVis>(item->object())) {
			if(visElement->pipelines(true).size() > 1)
				return _sharedObjectFont;
		}
	}

	return {};
}

/******************************************************************************
* Changes the data associated with a list entry.
******************************************************************************/
bool PipelineListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if(role == Qt::CheckStateRole) {
		PipelineListItem* item = this->item(index.row());
		if(DataVis* vis = dynamic_object_cast<DataVis>(item->object())) {
			UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(),
					(value == Qt::Checked) ? tr("Enable visual element") : tr("Disable visual element"), [vis, &value]() {
				vis->setEnabled(value == Qt::Checked);
			});
		}
		else if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(item->object())) {
			UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(),
					(value == Qt::Checked) ? tr("Enable modifier") : tr("Disable modifier"), [modApp, &value]() {
				modApp->modifier()->setEnabled(value == Qt::Checked);
			});
		}
	}
	return QAbstractListModel::setData(index, value, role);
}

/******************************************************************************
* Returns the flags for an item.
******************************************************************************/
Qt::ItemFlags PipelineListModel::flags(const QModelIndex& index) const
{
	if(index.row() >= 0 && index.row() < _items.size()) {
		PipelineListItem* item = this->item(index.row());
		if(item->object() == nullptr) {
			return Qt::NoItemFlags;
		}
		else {
			if(dynamic_object_cast<DataVis>(item->object())) {
				return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
			}
			else if(dynamic_object_cast<ModifierApplication>(item->object())) {
#if 0				
				return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
#else
				return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
#endif				
			}
		}
	}
	return QAbstractListModel::flags(index);
}

/******************************************************************************
* Returns the list of allowed MIME types.
******************************************************************************/
QStringList PipelineListModel::mimeTypes() const
{
    return QStringList() << QStringLiteral("application/ovito.modifier.list");
}

/******************************************************************************
* Returns an object that contains serialized items of data corresponding to the
* list of indexes specified.
******************************************************************************/
QMimeData* PipelineListModel::mimeData(const QModelIndexList& indexes) const
{
	QByteArray encodedData;
	QDataStream stream(&encodedData, QIODevice::WriteOnly);
	for(const QModelIndex& index : indexes) {
		if(index.isValid()) {
			stream << index.row();
		}
	}
	std::unique_ptr<QMimeData> mimeData(new QMimeData());
	mimeData->setData(QStringLiteral("application/ovito.modifier.list"), encodedData);
	return mimeData.release();
}

/******************************************************************************
* Returns true if the model can accept a drop of the data.
******************************************************************************/
bool PipelineListModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
	if(!data->hasFormat(QStringLiteral("application/ovito.modifier.list")))
		return false;

	if(column > 0)
		return false;

	return true;
}

/******************************************************************************
* Handles the data supplied by a drag and drop operation that ended with the
* given action.
******************************************************************************/
bool PipelineListModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
	if(!canDropMimeData(data, action, row, column, parent))
		return false;

	if(action == Qt::IgnoreAction)
		return true;

	if(row == -1 && parent.isValid())
		row = parent.row();
	if(row == -1)
		return false;

    QByteArray encodedData = data->data(QStringLiteral("application/ovito.modifier.list"));
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QVector<int> indexList;
    while(!stream.atEnd()) {
    	int index;
    	stream >> index;
    	indexList.push_back(index);
    }
    if(indexList.size() != 1)
    	return false;

	// The list item being dragged.
    PipelineListItem* movedItem = item(indexList[0]);
//	if(!movedItem->modifierApplication())
//		return false;

#if 0		
	// The ModifierApplication being dragged.
	OORef<ModifierApplication> modApp = movedItem->modifierApplications()[0];
	int indexDelta = -(row - indexList[0]);

	UndoableTransaction::handleExceptions(modApp->dataset()->undoStack(), tr("Move modifier"), [movedItem, modApp, indexDelta]() {
		// Determine old position in list.
		int index = _items.indexOf(movedItem);
		if(indexDelta == 0 || index + indexDelta < 0 || index+indexDelta >= pipelineObj->modifierApplications().size())
			return;
		// Remove ModifierApplication from the PipelineObject.
		pipelineObj->removeModifierApplication(index);
		// Re-insert ModifierApplication into the PipelineObject.
		pipelineObj->insertModifierApplication(index + indexDelta, modApp);
	});
#endif	

	return true;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
