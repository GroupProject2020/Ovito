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


#include <gui/GUI.h>
#include <gui/mainwin/data_inspector/DataInspectionApplet.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

/**
 * \brief Data inspector page for global attribute values.
 */
class GlobalAttributesInspectionApplet : public DataInspectionApplet
{
	Q_OBJECT
	OVITO_CLASS(GlobalAttributesInspectionApplet)
	Q_CLASSINFO("DisplayName", "Attributes");

public:

	/// Constructor.
	Q_INVOKABLE GlobalAttributesInspectionApplet() {}

	/// Returns the key value for this applet that is used for ordering the applet tabs.
	virtual int orderingKey() const override { return 100; }

	/// Determines whether the given pipeline flow state contains data that can be displayed by this applet.
	virtual bool appliesTo(const PipelineFlowState& state) override;

	/// Lets the applet create the UI widget that is to be placed into the data inspector panel. 
	virtual QWidget* createWidget(MainWindow* mainWindow) override;

	/// Updates the contents displayed in the inspector.
	virtual void updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode) override;

private:

	/// A table model for displaying global attributes.
	class AttributeTableModel : public QAbstractTableModel
	{
	public:

		/// Constructor.
		AttributeTableModel(QObject* parent) : QAbstractTableModel(parent) {}

		/// Returns the number of rows.
		virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override {
			return parent.isValid() ? 0 : _keys.size();
		}

		/// Returns the number of columns.
		virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override {
			return parent.isValid() ? 0 : 2;
		}

		/// Returns the data stored under the given 'role' for the item referred to by the 'index'.
		virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
			if(role == Qt::DisplayRole) {
				if(index.column() == 0) return _keys[index.row()];
				else return _values[index.row()];
			}
			return {};
		}

		/// Returns the data for the given role and section in the header with the specified orientation.
		virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
			if(orientation == Qt::Horizontal && role == Qt::DisplayRole) {
				if(section == 0) return tr("Attribute");
				else return tr("Value");
			}
			return QAbstractTableModel::headerData(section, orientation, role);
		}

		/// Replaces the contents of this data model.
		void setContents(const PipelineFlowState& state) {
			beginResetModel();
			_keys = state.attributes().keys();
			_values = state.attributes().values();
			endResetModel();
		}

	private:

		QList<QString> _keys;
		QList<QVariant> _values;
	};

private:

	/// The data display widget.
	QTableView* _tableView = nullptr;

	/// The table model.
	AttributeTableModel* _tableModel = nullptr;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace