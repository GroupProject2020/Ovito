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

#pragma once


#include <ovito/gui/GUI.h>
#include <ovito/gui/properties/PropertiesPanel.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/overlays/ViewportOverlay.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

class OverlayListModel;	// defined in OverlayListModel.h
class OverlayListItem;	// defined in OverlayListItem.h

/**
 * The command panel tab lets the user edit the viewport overlays.
 */
class OVITO_GUI_EXPORT OverlayCommandPage : public QWidget
{
	Q_OBJECT

public:

	/// Initializes the modify page.
    OverlayCommandPage(MainWindow* mainWindow, QWidget* parent);

	/// Returns the list model that encapsulates the list of overlays of the active viewport.
	OverlayListModel* overlayListModel() const { return _overlayListModel; }

protected Q_SLOTS:

	/// This is called whenever the current viewport configuration of current dataset has been replaced by a new one.
	void onViewportConfigReplaced(ViewportConfiguration* newViewportConfiguration);

	/// This is called when another viewport became active.
	void onActiveViewportChanged(Viewport* activeViewport);

	/// Is called when a new layer has been selected in the list box.
	void onItemSelectionChanged();

	/// This inserts a new viewport layer.
	void onNewLayer(int index);

	/// This deletes the selected viewport layer.
	void onDeleteLayer();

	/// This called when the user double clicks an item in the list.
	void onLayerDoubleClicked(const QModelIndex& index);

	/// Action handler moving the selected layer up in the stack.
	void onLayerMoveUp();

	/// Action handler moving the selected layer down in the stack.
	void onLayerMoveDown();

private:

	/// Returns the selected viewport layer.
	ViewportOverlay* selectedLayer() const;

	/// The container of the current dataset being edited.
	DataSetContainer& _datasetContainer;

	/// Contains the list of available layer types.
	QComboBox* _newLayerBox;

	/// The Qt model for the list of overlays of the active viewport.
	OverlayListModel* _overlayListModel;

	/// This list box shows the overlays of the active viewport.
	QListView* _overlayListWidget;

	/// This panel shows the properties of the selected overlay.
	PropertiesPanel* _propertiesPanel;

	/// Signal connection for detecting active viewport changes.
	QMetaObject::Connection _activeViewportChangedConnection;

	/// The GUI action that deletes the currently selected viewport layer.
	QAction* _deleteLayerAction;

	/// The GUI action that moves the currently selected viewport layer up in the stack.
	QAction* _moveLayerUpAction;

	/// The GUI action that moves the currently selected viewport layer down in the stack.
	QAction* _moveLayerDownAction;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
