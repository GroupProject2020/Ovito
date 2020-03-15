////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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

#include <ovito/gui/web/GUIWeb.h>
#include <ovito/gui/web/mainwin/MainWindow.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include "ModifierListModel.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
ModifierListModel::ModifierListModel(QObject* parent) : QAbstractListModel(parent)
{
	// Retrieve all installed modifier classes.
	for(ModifierClassPtr clazz : PluginManager::instance().metaclassMembers<Modifier>()) {
		// Sort modifiers into categories.
		QString categoryName = clazz->modifierCategory();
		if(categoryName == QStringLiteral("-")) {
			// This modifier requests to be hidden from the user.
			// Do not add it to the list of available modifiers.
		}
		else {
			_modifierClasses.push_back(clazz);
		}
	}

	// Sort modifiers.
	boost::sort(_modifierClasses, [](ModifierClassPtr a, ModifierClassPtr b) {
		return QString::compare(a->displayName(), b->displayName(), Qt::CaseInsensitive) < 0;
	});
}

/******************************************************************************
* Returns the number of rows in the model.
******************************************************************************/
int ModifierListModel::rowCount(const QModelIndex& parent) const 
{
	return _modifierClasses.size() + 1;
}

/******************************************************************************
* Returns the data associated with a list item.
******************************************************************************/
QVariant ModifierListModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::DisplayRole) {
		if(index.row() == 0) {
			return tr("Add modifier...");
		}
		else {
			return _modifierClasses[index.row() - 1]->displayName();
		}
	}
	return {};
}

/******************************************************************************
* Returns the flags for an item.
******************************************************************************/
Qt::ItemFlags ModifierListModel::flags(const QModelIndex& index) const
{
	return QAbstractListModel::flags(index);
}

/******************************************************************************
* Instantiates a modifier and inserts into the current data pipeline.
******************************************************************************/
void ModifierListModel::insertModifier(int index, PipelineListModel* pipelineModel)
{
	if(index <= 0 || index > _modifierClasses.size()) 
		return;

	// The modifier type to insert.
	ModifierClassPtr modifierClass = _modifierClasses[index - 1];
	// The current dataset.
	DataSet* dataset = static_cast<MainWindow*>(parent())->datasetContainer()->currentSet();

	// Instantiate the new modifier and insert it into the pipeline.
	UndoableTransaction::handleExceptions(dataset->undoStack(), tr("Apply modifier"), [&]() {
		// Create an instance of the modifier.
		OORef<Modifier> modifier = static_object_cast<Modifier>(modifierClass->createInstance(dataset));
		OVITO_CHECK_OBJECT_POINTER(modifier);
		// Load user-defined default parameters.
		modifier->loadUserDefaults();
		// Apply it to the data pipeline.
		pipelineModel->applyModifiers({modifier});
	});
}

}	// End of namespace
