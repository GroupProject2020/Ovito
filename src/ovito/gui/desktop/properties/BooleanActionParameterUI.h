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
#include "ParameterUI.h"

namespace Ovito {

/******************************************************************************
* This UI allows the user to change a boolean property of the object being edited.
******************************************************************************/
class OVITO_GUI_EXPORT BooleanActionParameterUI : public PropertyParameterUI
{
	Q_OBJECT
	OVITO_CLASS(BooleanActionParameterUI)

public:

	/// Constructor for a Qt property.
	BooleanActionParameterUI(QObject* parentEditor, const char* propertyName, QAction* action);

	/// Constructor for a PropertyField property.
	BooleanActionParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField, QAction* action);

	/// This returns the action associated with this parameter UI.
	QAction* action() const { return _action; }

	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.
	virtual void resetUI() override;

	/// This method updates the displayed value of the property UI.
	virtual void updateUI() override;

	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;

public:

	Q_PROPERTY(QAction action READ action)

public Q_SLOTS:

	/// Takes the value entered by the user and stores it in the property field
	/// this property UI is bound to.
	void updatePropertyValue();

protected:

	/// The check box of the UI component.
	QPointer<QAction> _action;
};

}	// End of namespace


