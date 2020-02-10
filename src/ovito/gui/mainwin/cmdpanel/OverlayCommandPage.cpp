////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/gui/mainwin/MainWindow.h>
#include "OverlayCommandPage.h"
#include "OverlayListModel.h"
#include "OverlayListItem.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Initializes the command panel page.
******************************************************************************/
OverlayCommandPage::OverlayCommandPage(MainWindow* mainWindow, QWidget* parent) : QWidget(parent),
		_datasetContainer(mainWindow->datasetContainer())
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(2,2,2,2);
	layout->setSpacing(4);

	_newLayerBox = new QComboBox(this);
    layout->addWidget(_newLayerBox);
    connect(_newLayerBox, (void (QComboBox::*)(int))&QComboBox::activated, this, &OverlayCommandPage::onNewLayer);

    _newLayerBox->addItem(tr("Add viewport layer..."));
    _newLayerBox->insertSeparator(1);
	for(OvitoClassPtr clazz : PluginManager::instance().listClasses(ViewportOverlay::OOClass())) {
		_newLayerBox->addItem(clazz->displayName(), QVariant::fromValue(clazz));
	}

	QSplitter* splitter = new QSplitter(Qt::Vertical);
	splitter->setChildrenCollapsible(false);

	class OverlayListWidget : public QListView {
	public:
		OverlayListWidget(QWidget* parent) : QListView(parent) {}
		virtual QSize sizeHint() const override { return QSize(256, 120); }
	protected:
		virtual bool edit(const QModelIndex& index, QAbstractItemView::EditTrigger trigger, QEvent* event) override {
			// Avoid triggering edit mode when user clicks the check box next to a list item.
			if(trigger == QAbstractItemView::SelectedClicked && event->type() == QEvent::MouseButtonRelease) {
				QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
				if(mouseEvent->pos().x() < visualRect(index).left() + 50)
					trigger = QAbstractItemView::NoEditTriggers;
			}
			return QListView::edit(index, trigger, event);
		}
	};

	QWidget* upperContainer = new QWidget();
	splitter->addWidget(upperContainer);
	QHBoxLayout* subLayout = new QHBoxLayout(upperContainer);
	subLayout->setContentsMargins(0,0,0,0);
	subLayout->setSpacing(2);

	_overlayListWidget = new OverlayListWidget(upperContainer);
	_overlayListModel = new OverlayListModel(this);
	_overlayListWidget->setEditTriggers(QAbstractItemView::SelectedClicked);
	_overlayListWidget->setModel(_overlayListModel);
	_overlayListWidget->setSelectionModel(_overlayListModel->selectionModel());
	subLayout->addWidget(_overlayListWidget);
	connect(_overlayListModel, &OverlayListModel::selectedItemChanged, this, &OverlayCommandPage::onItemSelectionChanged);
	connect(_overlayListWidget, &OverlayListWidget::doubleClicked, this, &OverlayCommandPage::onLayerDoubleClicked);

	QToolBar* editToolbar = new QToolBar(this);
	editToolbar->setOrientation(Qt::Vertical);
#ifndef Q_OS_MACX
	editToolbar->setStyleSheet("QToolBar { padding: 0px; margin: 0px; border: 0px none black; }");
#endif
	subLayout->addWidget(editToolbar);

	_deleteLayerAction = new QAction(QIcon(":/gui/actions/modify/delete_modifier.bw.svg"), tr("Delete Layer"), this);
	_deleteLayerAction->setEnabled(false);
	connect(_deleteLayerAction, &QAction::triggered, this, &OverlayCommandPage::onDeleteLayer);
	editToolbar->addAction(_deleteLayerAction);

	editToolbar->addSeparator();

	_moveLayerUpAction = new QAction(QIcon(":/gui/actions/modify/modifier_move_up.bw.svg"), tr("Move Layer Up"), this);
	connect(_moveLayerUpAction, &QAction::triggered, this, &OverlayCommandPage::onLayerMoveUp);
	editToolbar->addAction(_moveLayerUpAction);
	_moveLayerDownAction = new QAction(QIcon(":/gui/actions/modify/modifier_move_down.bw.svg"), tr("Move Layer Down"), this);
	connect(_moveLayerDownAction, &QAction::triggered, this, &OverlayCommandPage::onLayerMoveDown);
	editToolbar->addAction(_moveLayerDownAction);

	layout->addWidget(splitter, 1);

	// Create the properties panel.
	_propertiesPanel = new PropertiesPanel(nullptr, mainWindow);
	_propertiesPanel->setFrameStyle(QFrame::NoFrame | QFrame::Plain);
	splitter->addWidget(_propertiesPanel);
	splitter->setStretchFactor(1,1);

	connect(&_datasetContainer, &DataSetContainer::viewportConfigReplaced, this, &OverlayCommandPage::onViewportConfigReplaced);
}

/******************************************************************************
* Returns the selected viewport layer.
******************************************************************************/
ViewportOverlay* OverlayCommandPage::selectedLayer() const
{
	OverlayListItem* currentItem = overlayListModel()->selectedItem();
	return currentItem ? currentItem->overlay() : nullptr;
}

/******************************************************************************
* This is called whenever the current viewport configuration of current dataset
* has been replaced by a new one.
******************************************************************************/
void OverlayCommandPage::onViewportConfigReplaced(ViewportConfiguration* newViewportConfiguration)
{
	disconnect(_activeViewportChangedConnection);
	_propertiesPanel->setEditObject(nullptr);
	if(newViewportConfiguration) {
		_activeViewportChangedConnection = connect(newViewportConfiguration, &ViewportConfiguration::activeViewportChanged, this, &OverlayCommandPage::onActiveViewportChanged);
		onActiveViewportChanged(newViewportConfiguration->activeViewport());
	}
	else onActiveViewportChanged(nullptr);
}

/******************************************************************************
* This is called when another viewport became active.
******************************************************************************/
void OverlayCommandPage::onActiveViewportChanged(Viewport* activeViewport)
{
	overlayListModel()->setSelectedViewport(activeViewport);
	_newLayerBox->setEnabled(activeViewport != nullptr && _newLayerBox->count() > 1);
}

/******************************************************************************
* Is called when a new layer has been selected in the list box.
******************************************************************************/
void OverlayCommandPage::onItemSelectionChanged()
{
	ViewportOverlay* layer = selectedLayer();
	_propertiesPanel->setEditObject(layer);
	if(layer) {
		_deleteLayerAction->setEnabled(true);
		const QVector<ViewportOverlay*> overlays = overlayListModel()->selectedViewport()->overlays();
		const QVector<ViewportOverlay*> underlays = overlayListModel()->selectedViewport()->underlays();
		_moveLayerUpAction->setEnabled(!overlays.contains(layer) || overlays.indexOf(layer) < overlays.size() - 1);
		_moveLayerDownAction->setEnabled(!underlays.contains(layer) || underlays.indexOf(layer) > 0);
	}
	else {
		_deleteLayerAction->setEnabled(false);
		_moveLayerUpAction->setEnabled(false);
		_moveLayerDownAction->setEnabled(false);
	}
}

/******************************************************************************
* This inserts a new viewport layer.
******************************************************************************/
void OverlayCommandPage::onNewLayer(int index)
{
	if(index > 0) {
		OvitoClassPtr descriptor = _newLayerBox->itemData(index).value<OvitoClassPtr>();
		Viewport* vp = overlayListModel()->selectedViewport();
		if(descriptor && vp) {
			int overlayIndex = -1;
			int underlayIndex = -1;
			if(OverlayListItem* item = overlayListModel()->selectedItem()) {
				overlayIndex = vp->overlays().indexOf(item->overlay());
				underlayIndex = vp->underlays().indexOf(item->overlay());
			}
			UndoableTransaction::handleExceptions(vp->dataset()->undoStack(), tr("Add viewport layer"), [&]() {
				// Create an instance of the overlay class.
				OORef<ViewportOverlay> layer = static_object_cast<ViewportOverlay>(descriptor->createInstance(vp->dataset()));
				// Load user-defined default parameters.
				layer->loadUserDefaults();
				// Make sure the new overlay gets selected in the UI.
				overlayListModel()->setNextToSelectObject(layer);
				// Insert it into either the overlays or the underlays list.
				if(underlayIndex >= 0)
					vp->insertUnderlay(underlayIndex+1, layer);
				else if(overlayIndex >= 0)
					vp->insertOverlay(overlayIndex+1, layer);
				else
					vp->insertOverlay(vp->overlays().size(), layer);
				// Automatically activate preview mode to make the overlay visible.
				vp->setRenderPreviewMode(true);
			});
			_overlayListWidget->setFocus();
		}
		_newLayerBox->setCurrentIndex(0);
	}
}

/******************************************************************************
* This deletes the currently selected viewport layer.
******************************************************************************/
void OverlayCommandPage::onDeleteLayer()
{
	if(ViewportOverlay* layer = selectedLayer()) {
		UndoableTransaction::handleExceptions(layer->dataset()->undoStack(), tr("Delete layer"), [layer]() {
			layer->deleteReferenceObject();
		});
	}
}

/******************************************************************************
* This called when the user double clicks an item in the layer list.
******************************************************************************/
void OverlayCommandPage::onLayerDoubleClicked(const QModelIndex& index)
{
	if(OverlayListItem* item = overlayListModel()->item(index.row())) {
		if(ViewportOverlay* layer = item->overlay()) {
			// Toggle enabled state of layer.
			UndoableTransaction::handleExceptions(layer->dataset()->undoStack(), tr("Toggle layer visibility"), [layer]() {
				layer->setEnabled(!layer->isEnabled());
			});
		}
	}
}

/******************************************************************************
* Action handler moving the selected viewport layer up in the stack.
******************************************************************************/
void OverlayCommandPage::onLayerMoveUp()
{
	OverlayListItem* selectedItem = overlayListModel()->selectedItem();
	Viewport* vp = overlayListModel()->selectedViewport();
	if(!selectedItem || !vp) return;
	OORef<ViewportOverlay> layer = selectedItem->overlay();
	if(!layer) return;

	UndoableTransaction::handleExceptions(vp->dataset()->undoStack(), tr("Move layer up"), [&]() {
		int overlayIndex = vp->overlays().indexOf(layer);
		int underlayIndex = vp->underlays().indexOf(layer);
		if(overlayIndex >= 0 && overlayIndex < vp->overlays().size() - 1) {
			vp->removeOverlay(overlayIndex);
			vp->insertOverlay(overlayIndex+1, layer);
		}
		else if(underlayIndex >= 0) {
			if(underlayIndex == vp->underlays().size() - 1) {
				vp->removeUnderlay(underlayIndex);
				vp->insertOverlay(0, layer);
			}
			else {
				vp->removeUnderlay(underlayIndex);
				vp->insertUnderlay(underlayIndex+1, layer);
			}
		}
		// Make sure the new overlay gets selected in the UI.
		overlayListModel()->setNextToSelectObject(layer);
		_overlayListWidget->setFocus();	
	});
}

/******************************************************************************
* Action handler moving the selected viewport layer down in the stack.
******************************************************************************/
void OverlayCommandPage::onLayerMoveDown()
{
	OverlayListItem* selectedItem = overlayListModel()->selectedItem();
	Viewport* vp = overlayListModel()->selectedViewport();
	if(!selectedItem || !vp) return;
	OORef<ViewportOverlay> layer = selectedItem->overlay();
	if(!layer) return;

	UndoableTransaction::handleExceptions(vp->dataset()->undoStack(), tr("Move layer down"), [&]() {
		int underlayIndex = vp->underlays().indexOf(layer);
		int overlayIndex = vp->overlays().indexOf(layer);
		if(underlayIndex >= 1) {
			vp->removeUnderlay(underlayIndex);
			vp->insertUnderlay(underlayIndex-1, layer);
		}
		else if(overlayIndex >= 0) {
			if(overlayIndex == 0) {
				vp->removeOverlay(overlayIndex);
				vp->insertUnderlay(vp->underlays().size(), layer);
			}
			else {
				vp->removeOverlay(overlayIndex);
				vp->insertOverlay(overlayIndex-1, layer);
			}
		}
		// Make sure the new overlay gets selected in the UI.
		overlayListModel()->setNextToSelectObject(layer);
		_overlayListWidget->setFocus();	
	});
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
