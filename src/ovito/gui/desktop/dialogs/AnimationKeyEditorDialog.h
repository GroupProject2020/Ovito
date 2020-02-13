////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Alexander Stukowski
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
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/oo/RefTargetListener.h>
#include <ovito/gui/desktop/properties/ParameterUI.h>

namespace Ovito {

/**
 * This dialog box allows to edit the animation keys of an animatable parameter.
 */
class AnimationKeyEditorDialog : public QDialog, private UndoableTransaction
{
	Q_OBJECT

public:

	/// Constructor.
	AnimationKeyEditorDialog(KeyframeController* ctrl, const PropertyFieldDescriptor* propertyField, QWidget* parent, MainWindow* mainWindow);

	/// Returns the animation controller being edited.
	KeyframeController* ctrl() const { return _ctrl.target(); }

private Q_SLOTS:

	/// Event handler for the Ok button.
	void onOk();

	/// Handles the 'Add key' button.
	void onAddKey();

	/// Handles the 'Delete key' button.
	void onDeleteKey();

private:

	QTableView* _tableWidget;
	QAbstractTableModel* _model;
	QAction* _addKeyAction;
	QAction* _deleteKeyAction;
	RefTargetListener<KeyframeController> _ctrl;
	PropertiesPanel* _keyPropPanel;
};

}	// End of namespace


