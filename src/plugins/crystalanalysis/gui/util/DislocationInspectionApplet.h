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

	/// Determines whether the given pipeline flow state contains data that can be displayed by this applet.
	virtual bool appliesTo(const PipelineFlowState& state) override;

	/// Lets the applet create the UI widget that is to be placed into the data inspector panel. 
	virtual QWidget* createWidget(MainWindow* mainWindow) override;

	/// Updates the contents displayed in the inspector.
	virtual void updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode) override;

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
			return parent.isValid() ? 0 : 6;
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
				}
			}
			return QAbstractTableModel::headerData(section, orientation, role);
		}

		/// Replaces the contents of this data model.
		void setContents(const PipelineFlowState& state) {
			beginResetModel();
			_dislocationObj = state.findObject<DislocationNetworkObject>();
			_patternCatalog = state.findObject<PatternCatalog>();
			endResetModel();
		}

	private:

		OORef<DislocationNetworkObject> _dislocationObj;
		OORef<PatternCatalog> _patternCatalog;
	};

private:

	/// The data display widget.
	QTableView* _tableView = nullptr;

	/// The table model.
	DislocationTableModel* _tableModel = nullptr;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace
