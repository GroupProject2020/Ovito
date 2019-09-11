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

	/// Is called when a new overlay has been selected in the list box.
	void onItemSelectionChanged();

	/// This inserts a new overlay.
	void onNewOverlay(int index);

	/// This deletes the selected overlay.
	void onDeleteOverlay();

	/// This called when the user double clicks on an item in the overlay list.
	void onOverlayDoubleClicked(const QModelIndex& index);

private:

	/// Returns the selected overlay.
	ViewportOverlay* selectedOverlay() const;

	/// The container of the current dataset being edited.
	DataSetContainer& _datasetContainer;

	/// Contains the list of available overlay types.
	QComboBox* _newOverlayBox;

	/// The Qt model for the list of overlays of the active viewport.
	OverlayListModel* _overlayListModel;

	/// This list box shows the overlays of the active viewport.
	QListView* _overlayListWidget;

	/// This panel shows the properties of the selected overlay.
	PropertiesPanel* _propertiesPanel;

	/// Signal connection for detecting active viewport changes.
	QMetaObject::Connection _activeViewportChangedConnection;

	/// The GUI action that deletes the currently selected viewport overlay.
	QAction* _deleteOverlayAction;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
