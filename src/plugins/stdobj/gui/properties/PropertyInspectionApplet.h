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


#include <plugins/stdobj/gui/StdObjGui.h>
#include <plugins/stdobj/properties/PropertyExpressionEvaluator.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include <gui/mainwin/data_inspector/DataInspectionApplet.h>
#include <core/dataset/scene/PipelineSceneNode.h>

namespace Ovito { namespace StdObj {

/**
 * \brief Data inspector page for property-based data.
 */
class OVITO_STDOBJGUI_EXPORT PropertyInspectionApplet : public DataInspectionApplet
{
	Q_OBJECT
	OVITO_CLASS(PropertyInspectionApplet)

public:

	/// Determines whether the given pipeline flow state contains data that can be displayed by this applet.
	virtual bool appliesTo(const PipelineFlowState& state) override;

	/// Lets the applet update the contents displayed in the inspector.
	virtual void updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode) override;

	/// Returns the data display widget.
	QTableView* tableView() const { return _tableView; }

	/// Returns the list widget displaying the list of data bundles.
	QListWidget* bundleSelectionWidget() const { return _bundleSelectionWidget; }

	/// Returns the input widget for the filter expression.
	AutocompleteLineEdit* filterExpressionEdit() const { return _filterExpressionEdit; }

	/// Return The UI action that resets the filter expression.
	QAction* resetFilterAction() const { return _resetFilterAction; }

	/// Returns the currently selected scene node.
	PipelineSceneNode* currentSceneNode() const { return _sceneNode.data(); }

	/// Returns the current dataset.
	const PipelineFlowState& currentData() const { return _data; }

	/// Returns the number of currently displayed elements.
	int visibleElementCount() const { return _filterModel->rowCount(); }

	/// Returns the index of the i-th element currently shown in the table.
	size_t visibleElementAt(int index) const { return _filterModel->mapToSource(_filterModel->index(index, 0)).row(); }

	/// Returns the identifier of the data bundle that is currently selected.
	QString selectedBundleId() const;

	/// Returns the data object representing the data bundle that is currently selected.
	DataObject* selectedBundleObject() const;

protected:

	/// Constructor.
	PropertyInspectionApplet(const PropertyClass& propertyClass) : _propertyClass(propertyClass) {}

	/// Lets the applet create the UI widgets that are to be placed into the data inspector panel. 
	void createBaseWidgets();

	/// Creates the evaluator object for filter expressions.
	virtual std::unique_ptr<PropertyExpressionEvaluator> createExpressionEvaluator() = 0;

	/// Determines whether the given property represents a color.
	virtual bool isColorProperty(PropertyObject* property) const { return false; }

	/// Updates the list of data bundles displayed in the inspector.
	void updateBundleList();

Q_SIGNALS:

	/// This signal is emitted whenever the filter expression has changed.
	void filterChanged();

public Q_SLOTS:

	/// Sets the filter expression.
	void setFilterExpression(const QString& expression);

protected Q_SLOTS:

	/// Is called when the user selects a different bundle in the list.
	virtual void currentBundleChanged();

private Q_SLOTS:

	/// Is called when the uer has changed the filter expression.
	void onFilterExpressionEntered();

	/// Is called when an error during filter evaluation ocurred.
	void onFilterStatusChanged(const QString& msgText);

private:

	/// A table model for displaying the property data.
	class PropertyTableModel : public QAbstractTableModel
	{
	public:

		/// Constructor.
		PropertyTableModel(PropertyInspectionApplet* applet, QObject* parent) : QAbstractTableModel(parent), _applet(applet) {}

		/// Returns the number of rows.
		virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override {
			if(parent.isValid() || _properties.empty()) return 0;
			return (int)std::min(_properties.front()->size(), (size_t)std::numeric_limits<int>::max());
		}

		/// Returns the number of columns.
		virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override {
			return parent.isValid() ? 0 : _properties.size();
		}

		/// Returns the data stored under the given 'role' for the item referred to by the 'index'.
		virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

		/// Returns the data for the given role and section in the header with the specified orientation.
		virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
			if(orientation == Qt::Horizontal && role == Qt::DisplayRole) {
				OVITO_ASSERT(section >= 0 && section < _properties.size());
				return _properties[section]->name();
			}
			else if(orientation == Qt::Vertical && role == Qt::DisplayRole) {
				return section;
			}
			return QAbstractTableModel::headerData(section, orientation, role);
		}

		/// Replaces the contents of this data model.
		void setContents(const PipelineFlowState& state, const QString& bundleName);

	private:

		/// The owner of the model.
		PropertyInspectionApplet* _applet;

		/// The list of properties.
		std::vector<OORef<PropertyObject>> _properties;
	};

	/// A proxy model for filtering the property list.
	class PropertyFilterModel : public QSortFilterProxyModel
	{
	public:

		/// Constructor.
		PropertyFilterModel(PropertyInspectionApplet* applet, QObject* parent) : QSortFilterProxyModel(parent), _applet(applet) {}

		/// Replaces the contents of this data model.
		void setContentsBegin();

		/// Replaces the contents of this data model.
		void setContentsEnd() { 
			if(_filterExpression.isEmpty() == false)
				endResetModel();
		}

		/// Sets the filter expression.
		void setFilterExpression(const QString& expression) {
			if(_filterExpression != expression) {
				beginResetModel();
				_filterExpression = expression;
				setupEvaluator();
				endResetModel();
			}
		}

	protected:

		/// Performs the filtering of data rows.
		virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

		/// Initializes the expression evaluator.
		void setupEvaluator();

	private:

		/// The owner of the model.
		PropertyInspectionApplet* _applet;

		/// The filtering expression.
		QString _filterExpression;

		/// The filter expression evaluator. 
		mutable std::unique_ptr<PropertyExpressionEvaluator> _evaluator;

		/// The filter expression evaluator worker. 
		mutable std::unique_ptr<PropertyExpressionEvaluator::Worker> _evaluatorWorker;

		friend PropertyInspectionApplet;
	};

private:

	/// The type of properties displayed in this applet.
	const PropertyClass& _propertyClass;

	/// The data display widget.
	QTableView* _tableView = nullptr;

	/// The table model.
	PropertyTableModel* _tableModel = nullptr;

	/// The filter model.
	PropertyFilterModel* _filterModel;

	/// Input widget for the filter expression.
	AutocompleteLineEdit* _filterExpressionEdit;

	/// The UI action that resets the filter expression.
	QAction* _resetFilterAction;

	/// The current filter status.
	QString _filterStatusString;

	/// For cleaning up widgets.
	QObjectCleanupHandler _cleanupHandler;

	/// The currently selected scene node.
	QPointer<PipelineSceneNode> _sceneNode;

	/// The widget for selecting the current data bundle.
	QListWidget* _bundleSelectionWidget = nullptr;

	/// The input data.
	PipelineFlowState _data;
};

}	// End of namespace
}	// End of namespace
