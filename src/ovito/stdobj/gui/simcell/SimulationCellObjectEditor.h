////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
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


#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/gui/desktop/properties/PropertiesEditor.h>

namespace Ovito { namespace StdObj {

/**
 * \brief A properties editor for the SimulationCellObject class.
 */
class SimulationCellEditor : public PropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(SimulationCellEditor)

public:

	/// Default constructor.
	Q_INVOKABLE SimulationCellEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Is called when a spinner's value has changed.
	void onSizeSpinnerValueChanged(int dim);

	/// Is called when the user begins dragging a spinner interactively.
	void onSizeSpinnerDragStart(int dim);

	/// Is called when the user stops dragging a spinner interactively.
	void onSizeSpinnerDragStop(int dim);

	/// Is called when the user aborts dragging a spinner interactively.
	void onSizeSpinnerDragAbort(int dim);

	/// After the simulation cell size has changed, updates the UI controls.
	void updateSimulationBoxSize();

private:

	/// After the user has changed a spinner's value, this method changes the
	/// simulation cell geometry.
	void changeSimulationBoxSize(int dim);

	SpinnerWidget* simCellSizeSpinners[3];
	BooleanParameterUI* pbczPUI;
	AffineTransformationParameterUI* zvectorPUI[3];
	AffineTransformationParameterUI* zoriginPUI;
};

}	// End of namespace
}	// End of namespace
