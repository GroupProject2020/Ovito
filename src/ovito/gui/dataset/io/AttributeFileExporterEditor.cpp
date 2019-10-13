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

#include <ovito/gui/GUI.h>
#include <ovito/core/dataset/io/AttributeFileExporter.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/gui/utilities/concurrent/ProgressDialog.h>
#include "AttributeFileExporterEditor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

IMPLEMENT_OVITO_CLASS(AttributeFileExporterEditor);
SET_OVITO_OBJECT_EDITOR(AttributeFileExporter, AttributeFileExporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void AttributeFileExporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Attributes to export"), rolloutParams);
	QGridLayout* columnsGroupBoxLayout = new QGridLayout(rollout);

	_columnMappingWidget = new QListWidget();
	columnsGroupBoxLayout->addWidget(_columnMappingWidget, 0, 0, 5, 1);
	columnsGroupBoxLayout->setRowStretch(2, 1);

	QPushButton* moveUpButton = new QPushButton(tr("Move up"), rollout);
	QPushButton* moveDownButton = new QPushButton(tr("Move down"), rollout);
	QPushButton* selectAllButton = new QPushButton(tr("Select all"), rollout);
	QPushButton* selectNoneButton = new QPushButton(tr("Unselect all"), rollout);
	columnsGroupBoxLayout->addWidget(moveUpButton, 0, 1, 1, 1);
	columnsGroupBoxLayout->addWidget(moveDownButton, 1, 1, 1, 1);
	columnsGroupBoxLayout->addWidget(selectAllButton, 3, 1, 1, 1);
	columnsGroupBoxLayout->addWidget(selectNoneButton, 4, 1, 1, 1);
	moveUpButton->setEnabled(_columnMappingWidget->currentRow() >= 1);
	moveDownButton->setEnabled(_columnMappingWidget->currentRow() >= 0 && _columnMappingWidget->currentRow() < _columnMappingWidget->count() - 1);

	connect(_columnMappingWidget, &QListWidget::itemSelectionChanged, [moveUpButton, moveDownButton, this]() {
		moveUpButton->setEnabled(_columnMappingWidget->currentRow() >= 1);
		moveDownButton->setEnabled(_columnMappingWidget->currentRow() >= 0 && _columnMappingWidget->currentRow() < _columnMappingWidget->count() - 1);
	});

	connect(moveUpButton, &QPushButton::clicked, [this]() {
		int currentIndex = _columnMappingWidget->currentRow();
		QListWidgetItem* currentItem = _columnMappingWidget->takeItem(currentIndex);
		_columnMappingWidget->insertItem(currentIndex - 1, currentItem);
		_columnMappingWidget->setCurrentRow(currentIndex - 1);
		onAttributeChanged();
	});

	connect(moveDownButton, &QPushButton::clicked, [this]() {
		int currentIndex = _columnMappingWidget->currentRow();
		QListWidgetItem* currentItem = _columnMappingWidget->takeItem(currentIndex);
		_columnMappingWidget->insertItem(currentIndex + 1, currentItem);
		_columnMappingWidget->setCurrentRow(currentIndex + 1);
		onAttributeChanged();
	});

	connect(selectAllButton, &QPushButton::clicked, [this]() {
		for(int index = 0; index < _columnMappingWidget->count(); index++)
			_columnMappingWidget->item(index)->setCheckState(Qt::Checked);
	});

	connect(selectNoneButton, &QPushButton::clicked, [this]() {
		for(int index = 0; index < _columnMappingWidget->count(); index++)
			_columnMappingWidget->item(index)->setCheckState(Qt::Unchecked);
	});

	connect(this, &PropertiesEditor::contentsReplaced, this, &AttributeFileExporterEditor::updateAttributesList);
	connect(_columnMappingWidget, &QListWidget::itemChanged, this, &AttributeFileExporterEditor::onAttributeChanged);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool AttributeFileExporterEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject() && event.type() == ReferenceEvent::ReferenceChanged) {
		if(static_cast<const ReferenceFieldEvent&>(event).field() == &PROPERTY_FIELD(FileExporter::nodeToExport)) {
			updateAttributesList();
		}
	}
	return PropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Rebuilds the displayed list of exportable attributes.
******************************************************************************/
void AttributeFileExporterEditor::updateAttributesList()
{
	_columnMappingWidget->clear();

	// Retrieve the data to be exported.
	AttributeFileExporter* exporter = static_object_cast<AttributeFileExporter>(editObject());
	if(!exporter || !exporter->nodeToExport()) return;

	try {
		QVariantMap attrMap;
		ProgressDialog progressDialog(container(), exporter->dataset()->taskManager());
		AsyncOperation asyncOperation(progressDialog.taskManager());
		if(!exporter->getAttributesMap(exporter->dataset()->animationSettings()->time(), attrMap, asyncOperation))
			throw Exception(tr("Operation has been canceled by the user."));
		for(const QString& attrName : attrMap.keys())
			insertAttributeItem(attrName, exporter->attributesToExport());
	}
	catch(const Exception& ex) {
		// Ignore errors, but display a message in the UI widget to inform user.
		_columnMappingWidget->addItems(ex.messages());
	}

	onAttributeChanged();
}

/******************************************************************************
* Populates the list box with an entry.
******************************************************************************/
void AttributeFileExporterEditor::insertAttributeItem(const QString& displayName, const QStringList& selectedAttributeList)
{
	QListWidgetItem* item = new QListWidgetItem(displayName);
	item->setFlags(Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren));
	item->setCheckState(Qt::Unchecked);
	int sortKey = selectedAttributeList.size();

	for(int c = 0; c < selectedAttributeList.size(); c++) {
		if(selectedAttributeList[c] == displayName) {
			item->setCheckState(Qt::Checked);
			sortKey = c;
			break;
		}
	}

	item->setData(Qt::InitialSortOrderRole, sortKey);
	if(sortKey < selectedAttributeList.size()) {
		int insertIndex = 0;
		for(; insertIndex < _columnMappingWidget->count(); insertIndex++) {
			int k = _columnMappingWidget->item(insertIndex)->data(Qt::InitialSortOrderRole).value<int>();
			if(sortKey < k)
				break;
		}
		_columnMappingWidget->insertItem(insertIndex, item);
	}
	else {
		_columnMappingWidget->addItem(item);
	}
}

/******************************************************************************
* Is called when the user checked/unchecked an item in the attributes list.
******************************************************************************/
void AttributeFileExporterEditor::onAttributeChanged()
{
	AttributeFileExporter* exporter = dynamic_object_cast<AttributeFileExporter>(editObject());
	if(!exporter) return;

	QStringList newAttributeList;
	for(int index = 0; index < _columnMappingWidget->count(); index++) {
		if(_columnMappingWidget->item(index)->checkState() == Qt::Checked) {
			newAttributeList.push_back(_columnMappingWidget->item(index)->text());
		}
	}
	exporter->setAttributesToExport(newAttributeList);

	// Remember the selection for next time.
	QSettings settings;
	settings.beginGroup("exporter/attributes/");
	settings.setValue("attrlist", newAttributeList);
	settings.endGroup();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
