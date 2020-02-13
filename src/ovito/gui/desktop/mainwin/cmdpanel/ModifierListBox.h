////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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
#include <ovito/core/oo/OvitoClass.h>

namespace Ovito {

class PipelineListModel;

/**
 * A combo-box widget that lets the user insert new modifiers into the modification pipeline.
 */
class ModifierListBox : public QComboBox
{
public:

	/// Initializes the widget.
	ModifierListBox(QWidget* parent, PipelineListModel* pipelineList);

	/// Is called just before the drop-down box is activated.
	virtual void showPopup() override {
		updateApplicableModifiersList();
		_filterModel->invalidate();
		setMaxVisibleItems(_model->rowCount());
		_showAllModifiers = false;
		QComboBox::showPopup();
	}

	/// Indicates whether the complete list of modifiers should be shown.
	bool showAllModifiers() const {
		return (_showAllModifiers || _mostRecentlyUsedModifiers.size() < 4);
	}

private Q_SLOTS:

	/// Updates the list box of modifier classes that can be applied to the currently selected
	/// item in the modification list.
	void updateApplicableModifiersList();

	/// Updates the MRU list after the user has selected a modifier.
	void updateMRUList(const QString& selectedModifierName);

private:

	/// Filters the full list of modifiers to show only most recently used ones.
	bool filterAcceptsRow(int source_row, const QModelIndex& source_parent);

	/// Determines the sort order of the modifier list.
	bool filterSortLessThan(const QModelIndex& source_left, const QModelIndex& source_right);

	/// The modification list model.
	PipelineListModel* _pipelineList;

	/// The list items representing modifier types.
	QVector<QStandardItem*> _modifierItems;

	/// The item model containing all entries of the combo box.
	QStandardItemModel* _model;

	/// The item model used for filtering/storting the displayed list of modifiers.
	QSortFilterProxyModel* _filterModel;

	/// This flag asks updateApplicableModifiersList() to list all modifiers, not just the most recently used ones.
	bool _showAllModifiers = false;

	/// The number of modifier templates in the list.
	int _numModifierTemplates = 0;

	/// MRU list of modifiers.
	QStringList _mostRecentlyUsedModifiers;

	/// Maximum number of modifiers shown in the MRU list.
	int _maxMRUSize = 8;

	Q_OBJECT
};

}	// End of namespace
