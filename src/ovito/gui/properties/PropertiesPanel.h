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


#include <ovito/gui/GUI.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/gui/properties/PropertiesEditor.h>
#include <ovito/gui/widgets/general/RolloutContainer.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Widgets)

/******************************************************************************
* This panel lets the user edit the properties of some RefTarget derived object.
******************************************************************************/
class OVITO_GUI_EXPORT PropertiesPanel : public RolloutContainer
{
	Q_OBJECT

public:

	/// Constructs the panel.
	PropertiesPanel(QWidget* parent, MainWindow* mainWindow);

	/// Destructs the panel.
	virtual ~PropertiesPanel();

	/// Returns the target object being edited in the panel.
	RefTarget* editObject() const;

	/// Sets the target object being edited in the panel.
	void setEditObject(RefTarget* editObject);

	/// Returns the editor that is responsible for the object being edited.
	PropertiesEditor* editor() const { return _editor; }

protected:

	/// The editor for the current object.
	OORef<PropertiesEditor> _editor;

	/// The main window this properties panel is associated with.
	MainWindow* _mainWindow;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


