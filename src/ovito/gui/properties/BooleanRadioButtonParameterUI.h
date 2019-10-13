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
#include "ParameterUI.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

/******************************************************************************
* This UI allows the user to change a boolean-value property of the object being edited
* using two radio buttons.
******************************************************************************/
class OVITO_GUI_EXPORT BooleanRadioButtonParameterUI : public PropertyParameterUI
{
	Q_OBJECT
	OVITO_CLASS(BooleanRadioButtonParameterUI)

public:

	/// Constructor for a Qt property.
	BooleanRadioButtonParameterUI(QObject* parentEditor, const char* propertyName);

	/// Constructor for a PropertyField property.
	BooleanRadioButtonParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField);

	/// Destructor.
	virtual ~BooleanRadioButtonParameterUI();

	/// This returns the radio button group managed by this ParameterUI.
	QButtonGroup* buttonGroup() const { return _buttonGroup; }

	/// This returns the radio button for the "False" state.
	QRadioButton* buttonFalse() const { return buttonGroup() ? (QRadioButton*)buttonGroup()->button(0) : nullptr; }

	/// This returns the radio button for the "True" state.
	QRadioButton* buttonTrue() const { return buttonGroup() ? (QRadioButton*)buttonGroup()->button(1) : nullptr; }

	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.
	virtual void resetUI() override;

	/// This method updates the displayed value of the property UI.
	virtual void updateUI() override;

	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;

public:

	Q_PROPERTY(QButtonGroup buttonGroup READ buttonGroup)

public Q_SLOTS:

	/// Takes the value entered by the user and stores it in the property field
	/// this property UI is bound to.
	void updatePropertyValue();

protected:

	/// The radio button group.
	QPointer<QButtonGroup> _buttonGroup;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


