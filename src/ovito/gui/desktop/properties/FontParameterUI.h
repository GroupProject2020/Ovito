////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Alexander Stukowski
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
* Allows the user to pick a font.
******************************************************************************/
class OVITO_GUI_EXPORT FontParameterUI : public PropertyParameterUI
{
	Q_OBJECT
	OVITO_CLASS(FontParameterUI)

public:

	/// Constructor.
	FontParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField);

	/// Destructor.
	virtual ~FontParameterUI();

	/// This returns the font picker widget managed by this parameter UI.
	QPushButton* fontPicker() const { return _fontPicker; }

	/// This returns a label for the widget managed by this FontParameterUI.
	/// The text of the label widget is taken from the description text stored along
	/// with the property field.
	QLabel* label() const { return _label; }

	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.
	virtual void resetUI() override;

	/// This method updates the displayed value of the property UI.
	virtual void updateUI() override;

	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;

	/// Sets the What's This helper text for the label and the color picker.
	void setWhatsThis(const QString& text) const {
		if(label()) label()->setWhatsThis(text);
		if(fontPicker()) fontPicker()->setWhatsThis(text);
	}

public Q_SLOTS:

	/// Is called when the user has pressed the font picker button.
	void onButtonClicked();

protected:

	/// The font picker widget of the UI component.
	QPointer<QPushButton> _fontPicker;

	/// The label of the UI component.
	QPointer<QLabel> _label;
};

}	// End of namespace


