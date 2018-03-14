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

#include <plugins/stdobj/gui/StdObjGui.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include <plugins/stdobj/properties/PropertyExpressionEvaluator.h>
#include <gui/widgets/general/AutocompleteLineEdit.h>
#include <gui/mainwin/MainWindow.h>
#include "PropertyInspectionApplet.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(PropertyInspectionApplet);

/******************************************************************************
* Determines whether the given pipeline flow state contains data that can be 
* displayed by this applet.
******************************************************************************/
bool PropertyInspectionApplet::appliesTo(const PipelineFlowState& state)
{
	return _propertyClass.isDataPresent(state);
}

/******************************************************************************
* Lets the applet create the UI widgets that are to be placed into the data 
* inspector panel. 
******************************************************************************/
void PropertyInspectionApplet::createBaseWidgets()
{
	_filterExpressionEdit = new AutocompleteLineEdit();
	_filterExpressionEdit->setPlaceholderText(tr("Filter..."));
	_cleanupHandler.add(_filterExpressionEdit);
	_resetFilterAction = new QAction(QIcon(":/stdobj/icons/reset_filter.svg"), tr("Reset filter"), this);
	_cleanupHandler.add(_resetFilterAction);
	connect(_resetFilterAction, &QAction::triggered, _filterExpressionEdit, &QLineEdit::clear);
	connect(_resetFilterAction, &QAction::triggered, _filterExpressionEdit, &AutocompleteLineEdit::editingFinished);
	connect(_filterExpressionEdit, &AutocompleteLineEdit::editingFinished, this, &PropertyInspectionApplet::onFilterExpressionEntered);

	_tableView = new QTableView();
	_tableView->setWordWrap(false);
	_tableModel = new PropertyTableModel(this, _tableView);
	_filterModel = new PropertyFilterModel(this, _tableView);
	_filterModel->setSourceModel(_tableModel);
	_tableView->setModel(_filterModel);
	_cleanupHandler.add(_tableView);
}

/******************************************************************************
* Updates the contents displayed in the inspector.
******************************************************************************/
void PropertyInspectionApplet::updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode)
{
	// Clear filter expression when a different scene node has been selected.
	if(sceneNode != currentSceneNode()) {
		_resetFilterAction->trigger();
	}
	
	_sceneNode = sceneNode;
	_tableModel->setContents(state);
	_filterModel->setContentsBegin(state);
	_filterModel->setContentsEnd();

	// Update the list of variables that can be referenced in the filter expression.
	try {
		auto evaluator = createExpressionEvaluator();
		evaluator->initialize(QStringList(), state, _propertyClass);
		_filterExpressionEdit->setWordList(evaluator->inputVariableNames());
	}
	catch(const Exception&) {}
}

/******************************************************************************
* Replaces the contents of this data model.
******************************************************************************/
void PropertyInspectionApplet::PropertyTableModel::setContents(const PipelineFlowState& state) 
{
	// Generate the new list of properties.
	std::vector<OORef<PropertyObject>> newProperties;
	for(DataObject* o : state.objects()) {
		if(PropertyObject* prop = dynamic_object_cast<PropertyObject>(o))
			if(_applet->_propertyClass.isMember(prop))
				newProperties.push_back(prop);
	}
	int oldRowCount = rowCount();
	int newRowCount = 0;
	if(!newProperties.empty())
		newRowCount = (int)std::min(newProperties.front()->size(), (size_t)std::numeric_limits<int>::max());	

	// Try to preserve the columns of the model as far as possible.
	auto iter_pair = std::mismatch(_properties.begin(), _properties.end(), newProperties.begin(), newProperties.end(), 
		[](PropertyObject* prop1, PropertyObject* prop2) { 
			if(prop1->type() == PropertyStorage::GenericUserProperty)
				return prop1->name() == prop2->name();
			else
				return prop1->type() == prop2->type();
		});

	if(iter_pair.first != _properties.end()) {
		beginRemoveColumns(QModelIndex(), iter_pair.first - _properties.begin(), _properties.size()-1);
		_properties.erase(iter_pair.first, _properties.end());
		endRemoveColumns();
	}

	OVITO_ASSERT(_properties.size() <= newProperties.size());
	if(!_properties.empty()) {
		if(oldRowCount > newRowCount) {
			beginRemoveRows(QModelIndex(), newRowCount, oldRowCount-1);
			std::move(newProperties.begin(), newProperties.begin() + _properties.size(), _properties.begin());
			endRemoveRows();
		}
		else if(newRowCount > oldRowCount) {
			beginInsertRows(QModelIndex(), oldRowCount, newRowCount-1);
			std::move(newProperties.begin(), newProperties.begin() + _properties.size(), _properties.begin());
			endInsertRows();
		}
		else {
			std::move(newProperties.begin(), newProperties.begin() + _properties.size(), _properties.begin());
		}
		int changedRows = std::min(oldRowCount, newRowCount);
		if(changedRows) {
			dataChanged(index(0, 0), index(changedRows-1, _properties.size()-1));
		}

		if(newProperties.size() > _properties.size()) {
			beginInsertColumns(QModelIndex(), _properties.size(), newProperties.size()-1);
			_properties.insert(_properties.end(), std::make_move_iterator(newProperties.begin() + _properties.size()), std::make_move_iterator(newProperties.end()));
			endInsertColumns();
		}
	}
	else {
		beginResetModel();
		_properties = std::move(newProperties);
		endResetModel();
	}

	OVITO_ASSERT(rowCount() == newRowCount);
}

/******************************************************************************
* Replaces the contents of this data model.
******************************************************************************/
void PropertyInspectionApplet::PropertyFilterModel::setContentsBegin(const PipelineFlowState& state) 
{
	if(_filterExpression.isEmpty() == false)
		beginResetModel();
	_data = state;
	setupEvaluator();
}

/******************************************************************************
* Initializes the expression evaluator.
******************************************************************************/
void PropertyInspectionApplet::PropertyFilterModel::setupEvaluator()
{
	_evaluatorWorker.reset();
	_evaluator.reset();
	if(_filterExpression.isEmpty() == false) {
		try {
			// Check if expression contain an assignment ('=' operator).
			// This should be considered an error, because the user is probably referring the comparison operator '=='.
			if(_filterExpression.contains(QRegExp("[^=!><]=(?!=)")))
				throw Exception(tr("The entered expression contains the assignment operator '='. Please use the comparison operator '==' instead."));

			_evaluator = _applet->createExpressionEvaluator();
			_evaluator->initialize(QStringList(_filterExpression), _data, _applet->_propertyClass);
			_evaluatorWorker = std::make_unique<PropertyExpressionEvaluator::Worker>(*_evaluator);
		}
		catch(const Exception& ex) {
			_applet->onFilterStatusChanged(ex.messages().join("\n"));
			_evaluatorWorker.reset();
			_evaluator.reset();
			return;
		}
	}
	_applet->onFilterStatusChanged(QString());
}

/******************************************************************************
* Returns the data stored under the given 'role' for the item referred to by 
* the 'index'.
******************************************************************************/
QVariant PropertyInspectionApplet::PropertyTableModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::DisplayRole) {
		OVITO_ASSERT(index.column() >= 0 && index.column() < _properties.size());
		size_t elementIndex = index.row();
		PropertyObject* property = _properties[index.column()];
		if(elementIndex < property->size()) {
			QString str;
			for(size_t component = 0; component < property->componentCount(); component++) {
				if(component != 0) str += QStringLiteral(" ");
				if(property->dataType() == PropertyStorage::Int) {
					str += QString::number(property->getIntComponent(elementIndex, component));
					if(property->elementTypes().empty() == false) {
						if(ElementType* ptype = property->elementType(property->getIntComponent(elementIndex, component)))
							str += QStringLiteral(" (%1)").arg(ptype->name());
					}
				}
				else if(property->dataType() == PropertyStorage::Int64) {
					str += QString::number(property->getInt64Component(elementIndex, component));
				}
				else if(property->dataType() == PropertyStorage::Float) {
					str += QString::number(property->getFloatComponent(elementIndex, component));
				}
			}
			return str;
		}
	}
	else if(role == Qt::DecorationRole) {
		OVITO_ASSERT(index.column() >= 0 && index.column() < _properties.size());
		PropertyObject* property = _properties[index.column()];
		size_t elementIndex = index.row();
		if(elementIndex < property->size()) {
			if(_applet->isColorProperty(property)) {
				return (QColor)property->getColor(elementIndex);
			}
			else if(property->dataType() == PropertyStorage::Int && property->componentCount() == 1 && property->elementTypes().empty() == false) {
				if(ElementType* ptype = property->elementType(property->getInt(elementIndex)))
					return (QColor)ptype->color();
			}
		}
	}
	return {};
}

/******************************************************************************
* Is called when the uer has changed the filter expression.
******************************************************************************/
void PropertyInspectionApplet::onFilterExpressionEntered()
{
	_filterModel->setFilterExpression(_filterExpressionEdit->text());
	Q_EMIT filterChanged();
}

/******************************************************************************
* Sets the filter expression.
******************************************************************************/
void PropertyInspectionApplet::setFilterExpression(const QString& expression)
{
	_filterExpressionEdit->setText(expression);
	_filterModel->setFilterExpression(expression);
	Q_EMIT filterChanged();
}

/******************************************************************************
* Is called when an error during filter evaluation ocurred.
******************************************************************************/
void PropertyInspectionApplet::onFilterStatusChanged(const QString& msgText)
{
	if(msgText.isEmpty() == false) {
		_filterStatusString = msgText;
		QToolTip::showText(_filterExpressionEdit->mapToGlobal(_filterExpressionEdit->rect().bottomLeft()), msgText, 
			_filterExpressionEdit, QRect());
	}
	else if(!_filterStatusString.isEmpty()) {
		QToolTip::hideText();
		_filterStatusString.clear();
	}
}

/******************************************************************************
* Performs the filtering of data rows.
******************************************************************************/
bool PropertyInspectionApplet::PropertyFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
	if(_evaluatorWorker && (size_t)source_row < _evaluator->elementCount()) {
		try {
			return _evaluatorWorker->evaluate(source_row, 0);
		}
		catch(const Exception& ex) {
			_applet->onFilterStatusChanged(ex.messages().join("\n"));
			_evaluatorWorker.reset();
			_evaluator.reset();
		}
	}
	return true;
}

}	// End of namespace
}	// End of namespace
