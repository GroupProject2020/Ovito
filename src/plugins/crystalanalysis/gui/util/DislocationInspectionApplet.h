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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationVis.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <gui/mainwin/data_inspector/DataInspectionApplet.h>
#include <gui/viewport/input/ViewportInputMode.h>
#include <gui/viewport/input/ViewportInputManager.h>
#include <gui/viewport/input/ViewportGizmo.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

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
			return parent.isValid() ? 0 : (_dislocationObj ? _dislocationObj->segments().size() : 0);
		}

		/// Returns the number of columns.
		virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override {
			return parent.isValid() ? 0 : 8;
		}

		/// Returns the data stored under the given 'role' for the item referred to by the 'index'.
		virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
			if(role == Qt::DisplayRole) {
				DislocationSegment* segment = _dislocationObj->segments()[index.row()];
				switch(index.column()) {
				case 0: return _dislocationObj->segments()[index.row()]->id;
				case 1: return DislocationVis::formatBurgersVector(segment->burgersVector.localVec(), _patternCatalog ? _patternCatalog->structureById(segment->burgersVector.cluster()->structure) : nullptr);
				case 2: { Vector3 b = segment->burgersVector.toSpatialVector(); 
						return QStringLiteral("%1 %2 %3")
									.arg(QLocale::c().toString(b.x(), 'f', 4), 7)
									.arg(QLocale::c().toString(b.y(), 'f', 4), 7)
									.arg(QLocale::c().toString(b.z(), 'f', 4), 7); }
				case 3: return QLocale::c().toString(segment->calculateLength());
				case 4: return segment->burgersVector.cluster()->id;
				case 5: if(_patternCatalog) 
							return _patternCatalog->structureById(segment->burgersVector.cluster()->structure)->name(); 
						else
							break;
				case 6: { Point3 headLocation = segment->backwardNode().position();
							if(_dislocationObj->domain()) headLocation = _dislocationObj->domain()->data().wrapPoint(headLocation);
							return QStringLiteral("%1 %2 %3")
								.arg(QLocale::c().toString(headLocation.x(), 'f', 4), 7)
								.arg(QLocale::c().toString(headLocation.y(), 'f', 4), 7)
								.arg(QLocale::c().toString(headLocation.z(), 'f', 4), 7); }
				case 7: { Point3 tailLocation = segment->forwardNode().position();
							if(_dislocationObj->domain()) tailLocation = _dislocationObj->domain()->data().wrapPoint(tailLocation);
							return QStringLiteral("%1 %2 %3")
								.arg(QLocale::c().toString(tailLocation.x(), 'f', 4), 7)
								.arg(QLocale::c().toString(tailLocation.y(), 'f', 4), 7)
								.arg(QLocale::c().toString(tailLocation.z(), 'f', 4), 7); }
				}
			}
			else if(role == Qt::DecorationRole && index.column() == 1 && _patternCatalog) {
				DislocationSegment* segment = _dislocationObj->segments()[index.row()];
				StructurePattern* pattern = _patternCatalog->structureById(segment->burgersVector.cluster()->structure);
				BurgersVectorFamily* family = pattern->defaultBurgersVectorFamily();
				for(BurgersVectorFamily* f : pattern->burgersVectorFamilies()) {
					if(f->isMember(segment->burgersVector.localVec(), pattern)) {
						family = f;
						break;
					}
				}
				if(family) return (QColor)family->color();
			}
			return {};
		}

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
				case 6: return tr("Head vertex");
				case 7: return tr("Tail vertex");
				}
			}
			return QAbstractTableModel::headerData(section, orientation, role);
		}

		/// Replaces the contents of this data model.
		void setContents(const PipelineFlowState& state) {
			beginResetModel();
			_dislocationObj = state.getObject<DislocationNetworkObject>();
			_patternCatalog = state.getObject<PatternCatalog>();
			endResetModel();
		}

	private:

		OORef<DislocationNetworkObject> _dislocationObj;
		OORef<PatternCatalog> _patternCatalog;
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

		/// Determines the dislocation segment under the mouse cursor.
		int pickDislocationSegment(ViewportWindow* vpwin, const QPoint& pos) const;
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
}	// End of namespace
