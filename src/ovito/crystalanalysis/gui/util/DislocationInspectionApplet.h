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


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/DislocationNetworkObject.h>
#include <ovito/crystalanalysis/objects/DislocationVis.h>
#include <ovito/gui/desktop/mainwin/data_inspector/DataInspectionApplet.h>
#include <ovito/gui/viewport/input/ViewportInputMode.h>
#include <ovito/gui/viewport/input/ViewportInputManager.h>
#include <ovito/gui/viewport/input/ViewportGizmo.h>

namespace Ovito { namespace CrystalAnalysis {

/**
 * \brief Data inspector page for dislocation lines.
 */
class DislocationInspectionApplet : public DataInspectionApplet
{
	Q_OBJECT
	OVITO_CLASS(DislocationInspectionApplet)
	Q_CLASSINFO("DisplayName", "Dislocations");

public:

	/// Constructor.
	Q_INVOKABLE DislocationInspectionApplet() {}

	/// Returns the key value for this applet that is used for ordering the applet tabs.
	virtual int orderingKey() const override { return 50; }

	/// Determines whether the given pipeline data contains data that can be displayed by this applet.
	virtual bool appliesTo(const DataCollection& data) override;

	/// Lets the applet create the UI widget that is to be placed into the data inspector panel.
	virtual QWidget* createWidget(MainWindow* mainWindow) override;

	/// Updates the contents displayed in the inspector.
	virtual void updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode) override;

	/// This is called when the applet is no longer visible.
	virtual void deactivate(MainWindow* mainWindow) override;

private:

	/// A table model for displaying the dislocation list.
	class DislocationTableModel : public QAbstractTableModel
	{
	public:

		/// Constructor.
		DislocationTableModel(QObject* parent) : QAbstractTableModel(parent) {}

		/// Returns the number of rows.
		virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override {
			if(parent.isValid()) return 0;
			if(_dislocationObj) return _dislocationObj->segments().size();
			if(_microstructure && _microstructure->topology()) return _microstructure->topology()->faceCount();
			return 0;
		}

		/// Returns the number of columns.
		virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override {
			return parent.isValid() ? 0 : 8;
		}

		/// Returns the data stored under the given 'role' for the item referred to by the 'index'.
		virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

		/// Returns the data for the given role and section in the header with the specified orientation.
		virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
			if(orientation == Qt::Horizontal && role == Qt::DisplayRole) {
				switch(section) {
				case 0: return tr("Id");
				case 1: return tr("Burgers vector");
				case 2: return tr("Spatial Burgers vector");
				case 3: return tr("Length");
				case 4: return tr("Cluster");
				case 5: return tr("Crystal structure");
				case 6: return tr("Head vertex coordinates");
				case 7: return tr("Tail vertex coordinates");
				}
			}
			return QAbstractTableModel::headerData(section, orientation, role);
		}

		/// Replaces the contents of this data model.
		void setContents(const PipelineFlowState& state) {
			beginResetModel();
			if(!state.isEmpty()) {
				_dislocationObj = state.getObject<DislocationNetworkObject>();
				_microstructure = state.getObject<Microstructure>();
			}
			else {
				_dislocationObj.reset();
				_microstructure.reset();
			}
			endResetModel();
		}

	private:

		OORef<DislocationNetworkObject> _dislocationObj;
		OORef<Microstructure> _microstructure;
	};


	/// Viewport input mode that lets the user pick dislocations.
	class PickingMode : public ViewportInputMode, ViewportGizmo
	{
	public:

		/// Constructor.
		PickingMode(DislocationInspectionApplet* applet) : ViewportInputMode(applet), _applet(applet) {}

		/// This is called by the system after the input handler has become the active handler.
		virtual void activated(bool temporaryActivation) override {
			ViewportInputMode::activated(temporaryActivation);
			inputManager()->addViewportGizmo(this);
		}

		/// This is called by the system after the input handler is no longer the active handler.
		virtual void deactivated(bool temporary) override {
			inputManager()->removeViewportGizmo(this);
			ViewportInputMode::deactivated(temporary);
		}

		/// Handles the mouse up events for a viewport.
		virtual void mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event) override;

		/// Handles the mouse move event for the given viewport.
		virtual void mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event) override;

		/// Lets the input mode render its overlay content in a viewport.
		virtual void renderOverlay3D(Viewport* vp, ViewportSceneRenderer* renderer) override;

	private:

		/// The owner object.
		DislocationInspectionApplet* _applet;

		/// Determines the dislocation under the mouse cursor.
		int pickDislocation(ViewportWindow* vpwin, const QPoint& pos) const;
	};

private:

	/// The data display widget.
	QTableView* _tableView = nullptr;

	/// The table model.
	DislocationTableModel* _tableModel = nullptr;

	/// The viewport input mode for picking dislocations.
	PickingMode* _pickingMode;

	/// The currently selected scene node.
	QPointer<PipelineSceneNode> _sceneNode;
};

}	// End of namespace
}	// End of namespace
