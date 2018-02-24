///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2017) Alexander Stukowski
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


#include <gui/GUI.h>
#include "ParameterUI.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

/**
 * \brief UI component that allows the user to select the delegate for a DelegatingModifier.
 */
class OVITO_GUI_EXPORT ModifierDelegateParameterUI : public ParameterUI
{
	Q_OBJECT
	OVITO_CLASS(ModifierDelegateParameterUI)
	
public:

	/// Constructor.
	ModifierDelegateParameterUI(QObject* parent, const OvitoClass& delegateType);

	/// Destructor.
	virtual ~ModifierDelegateParameterUI();
	
	/// This returns the combo box managed by this ParameterUI.
	QComboBox* comboBox() const { return _comboBox; }
	
	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;
	
	/// This method updates the displayed value of the parameter UI.
	virtual void updateUI() override;

	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.  
	virtual void resetUI() override;

	/// Sets the tooltip text for the combo box widget.
	void setToolTip(const QString& text) const { 
		if(comboBox()) comboBox()->setToolTip(text); 
	}
	
	/// Sets the What's This helper text for the combo box.
	void setWhatsThis(const QString& text) const { 
		if(comboBox()) comboBox()->setWhatsThis(text); 
	}

public:
	
	Q_PROPERTY(QComboBox comboBox READ comboBox);

public Q_SLOTS:
	
	/// Takes the value entered by the user and stores it in the property field 
	/// this property UI is bound to. 
	void updatePropertyValue();
	
protected:

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;
	
	/// The combo box of the UI component.
	QPointer<QComboBox> _comboBox;

	/// The type of modifier delegates, which the user can choose from.
	const OvitoClass& _delegateType;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


