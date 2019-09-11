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

	_newOverlayBox = new QComboBox(this);
    layout->addWidget(_newOverlayBox);
    connect(_newOverlayBox, (void (QComboBox::*)(int))&QComboBox::activated, this, &OverlayCommandPage::onNewOverlay);

    _newOverlayBox->addItem(tr("Add overlay..."));
    _newOverlayBox->insertSeparator(1);
	for(OvitoClassPtr clazz : PluginManager::instance().listClasses(ViewportOverlay::OOClass())) {
		_newOverlayBox->addItem(clazz->displayName(), QVariant::fromValue(clazz));
	}

	QSplitter* splitter = new QSplitter(Qt::Vertical);
	splitter->setChildrenCollapsible(false);

	class OverlayListWidget : public QListView {
	public:
		OverlayListWidget(QWidget* parent) : QListView(parent) {}
		virtual QSize sizeHint() const override { return QSize(256, 120); }
	};

	QWidget* upperContainer = new QWidget();
	splitter->addWidget(upperContainer);
	QHBoxLayout* subLayout = new QHBoxLayout(upperContainer);
	subLayout->setContentsMargins(0,0,0,0);
	subLayout->setSpacing(2);

	_overlayListWidget = new OverlayListWidget(upperContainer);
	_overlayListModel = new OverlayListModel(this);
	_overlayListWidget->setModel(_overlayListModel);
	_overlayListWidget->setSelectionModel(_overlayListModel->selectionModel());
	subLayout->addWidget(_overlayListWidget);
	connect(_overlayListModel, &OverlayListModel::selectedItemChanged, this, &OverlayCommandPage::onItemSelectionChanged);
	connect(_overlayListWidget, &OverlayListWidget::doubleClicked, this, &OverlayCommandPage::onOverlayDoubleClicked);

	QToolBar* editToolbar = new QToolBar(this);
	editToolbar->setOrientation(Qt::Vertical);
#ifndef Q_OS_MACX
	editToolbar->setStyleSheet("QToolBar { padding: 0px; margin: 0px; border: 0px none black; }");
#endif
	subLayout->addWidget(editToolbar);

	_deleteOverlayAction = new QAction(QIcon(":/gui/actions/modify/delete_modifier.bw.svg"), tr("Delete Overlay"), this);
	_deleteOverlayAction->setEnabled(false);
	connect(_deleteOverlayAction, &QAction::triggered, this, &OverlayCommandPage::onDeleteOverlay);
	editToolbar->addAction(_deleteOverlayAction);

	layout->addWidget(splitter, 1);

	// Create the properties panel.
	_propertiesPanel = new PropertiesPanel(nullptr, mainWindow);
	_propertiesPanel->setFrameStyle(QFrame::NoFrame | QFrame::Plain);
	splitter->addWidget(_propertiesPanel);
	splitter->setStretchFactor(1,1);

	connect(&_datasetContainer, &DataSetContainer::viewportConfigReplaced, this, &OverlayCommandPage::onViewportConfigReplaced);
}

/******************************************************************************
* Returns the selected overlay.
******************************************************************************/
ViewportOverlay* OverlayCommandPage::selectedOverlay() const
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
	_newOverlayBox->setEnabled(activeViewport != nullptr && _newOverlayBox->count() > 1);
}

/******************************************************************************
* Is called when a new overlay has been selected in the list box.
******************************************************************************/
void OverlayCommandPage::onItemSelectionChanged()
{
	ViewportOverlay* overlay = selectedOverlay();
	_propertiesPanel->setEditObject(overlay);
	_deleteOverlayAction->setEnabled(overlay != nullptr);
}

/******************************************************************************
* This inserts a new overlay.
******************************************************************************/
void OverlayCommandPage::onNewOverlay(int index)
{
	if(index > 0) {
		OvitoClassPtr descriptor = _newOverlayBox->itemData(index).value<OvitoClassPtr>();
		Viewport* vp = overlayListModel()->selectedViewport();
		if(descriptor && vp) {
			int index = overlayListModel()->selectedIndex();
			if(index < 0) index = 0;
			UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Add overlay"), [this, descriptor, vp, index]() {
				// Create an instance of the overlay class.
				OORef<ViewportOverlay> overlay = static_object_cast<ViewportOverlay>(descriptor->createInstance(vp->dataset()));
				// Load user-defined default parameters.
				overlay->loadUserDefaults();
				// Make sure the new overlay gets selected in the UI.
				overlayListModel()->setNextToSelectObject(overlay);
				// Insert it.
				vp->insertOverlay(index, overlay);
				// Automatically activate preview mode to make the overlay visible.
				vp->setRenderPreviewMode(true);
			});
			_overlayListWidget->setFocus();
		}
		_newOverlayBox->setCurrentIndex(0);
	}
}

/******************************************************************************
* This deletes the currently selected overlay.
******************************************************************************/
void OverlayCommandPage::onDeleteOverlay()
{
	if(ViewportOverlay* overlay = selectedOverlay()) {
		UndoableTransaction::handleExceptions(overlay->dataset()->undoStack(), tr("Delete overlay"), [overlay]() {
			overlay->deleteReferenceObject();
		});
	}
}

/******************************************************************************
* This called when the user double clicks on an item in the overlay list.
******************************************************************************/
void OverlayCommandPage::onOverlayDoubleClicked(const QModelIndex& index)
{
	if(OverlayListItem* item = overlayListModel()->item(index.row())) {
		if(ViewportOverlay* overlay = item->overlay()) {
			// Toggle enabled state of overlay.
			UndoableTransaction::handleExceptions(overlay->dataset()->undoStack(), tr("Toggle overlay state"), [overlay]() {
				overlay->setEnabled(!overlay->isEnabled());
			});
		}
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
