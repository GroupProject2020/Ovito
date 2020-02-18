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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/mainwin/data_inspector/DataInspectionApplet.h>
#include <ovito/core/dataset/data/AttributeDataObject.h>

namespace Ovito {

/**
 * \brief Data inspector page for global attribute values.
 */
class GlobalAttributesInspectionApplet : public DataInspectionApplet
{
	Q_OBJECT
	OVITO_CLASS(GlobalAttributesInspectionApplet)
	Q_CLASSINFO("DisplayName", "Global Attributes");

public:

	/// Constructor.
	Q_INVOKABLE GlobalAttributesInspectionApplet() {}

	/// Returns the key value for this applet that is used for ordering the applet tabs.
	virtual int orderingKey() const override { return 100; }

	/// Determines whether the given pipeline flow state contains data that can be displayed by this applet.
	virtual bool appliesTo(const DataCollection& data) override;

	/// Lets the applet create the UI widget that is to be placed into the data inspector panel.
	virtual QWidget* createWidget(MainWindow* mainWindow) override;

	/// Updates the contents displayed in the inspector.
	virtual void updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode) override;

	/// Selects a specific data object in this applet.
	virtual bool selectDataObject(PipelineObject* dataSource, const QString& objectIdentifierHint, const QVariant& modeHint) override;

private Q_SLOTS:

	/// Action handler.
	void exportToFile();

private:

	/// A table model for displaying global attributes.
	class AttributeTableModel : public QAbstractTableModel
	{
	public:

		/// Inherit constructor.
		using QAbstractTableModel::QAbstractTableModel;

		/// Returns the number of rows.
		virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override {
			return parent.isValid() ? 0 : _attributes.size();
		}

		/// Returns the number of columns.
		virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override {
			return parent.isValid() ? 0 : 2;
		}

		/// Returns the data stored under the given 'role' for the item referred to by the 'index'.
		virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
			if(role == Qt::DisplayRole) {
				if(index.column() == 0) return _attributes[index.row()]->identifier();
				else {
					const QVariant& v = _attributes[index.row()]->value();
					if(v.type() == QVariant::Double)
						return QString::number(v.toDouble());
					return v;
				}
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
		void setContents(const DataCollection* dataCollection) {
			beginResetModel();
			_attributes.clear();
			if(dataCollection) {
				for(const DataObject* obj : dataCollection->objects()) {
					if(const AttributeDataObject* attribute = dynamic_object_cast<AttributeDataObject>(obj)) {
						_attributes.push_back(attribute);
					}
				}
				boost::sort(_attributes, [](const auto& a, const auto& b) {
					return a->identifier() < b->identifier();
				});
			}
			endResetModel();
		}

		/// Returns the current list of attributes.
		const std::vector<OORef<AttributeDataObject>>& attributes() const { return _attributes; }

	private:

		/// The list of attributes.
		std::vector<OORef<AttributeDataObject>> _attributes;
	};

private:

	/// The data display widget.
	QTableView* _tableView = nullptr;

	/// The table model.
	AttributeTableModel* _tableModel = nullptr;

	/// The parent window.
	MainWindow* _mainWindow;

	/// The currently selected scene node.
	QPointer<PipelineSceneNode> _sceneNode;
};

}	// End of namespace
