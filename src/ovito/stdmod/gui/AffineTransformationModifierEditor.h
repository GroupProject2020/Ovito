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


#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/gui/properties/ModifierPropertiesEditor.h>

namespace Ovito { namespace StdMod {

/**
 * A properties editor for the AffineTransformationModifier class.
 */
class AffineTransformationModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(AffineTransformationModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE AffineTransformationModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private Q_SLOTS:

	/// Is called when the spinner value has changed.
	void onSpinnerValueChanged();

	/// Is called when the user begins dragging the spinner interactively.
	void onSpinnerDragStart();

	/// Is called when the user stops dragging the spinner interactively.
	void onSpinnerDragStop();

	/// Is called when the user aborts dragging the spinner interactively.
	void onSpinnerDragAbort();

	/// This method updates the displayed matrix values.
	void updateUI();

	/// Is called when the user presses the 'Enter rotation' button.
	void onEnterRotation();

private:

	/// Takes the value entered by the user and stores it in transformation controller.
	void updateParameterValue();

	SpinnerWidget* elementSpinners[3][4];
};

}	// End of namespace
}	// End of namespace
