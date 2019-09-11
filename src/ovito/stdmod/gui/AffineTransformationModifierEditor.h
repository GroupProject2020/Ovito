///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
