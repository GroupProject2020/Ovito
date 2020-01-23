////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/gui/desktop/properties/ModifierPropertiesEditor.h>
#include <ovito/stdobj/gui/widgets/DataTablePlotWidget.h>
#include <ovito/core/utilities/DeferredMethodInvocation.h>

class QwtPlotZoneItem;

namespace Ovito { namespace StdMod {

/**
 * A properties editor for the ScatterPlotModifier class.
 */
class ScatterPlotModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(ScatterPlotModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE ScatterPlotModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Replots the scatter plot.
	void plotScatterPlot();

private:

	/// The graph widget to display the plot.
	DataTablePlotWidget* _plotWidget;

	/// Marks the range of selected points in the X direction.
	QwtPlotZoneItem* _selectionRangeIndicatorX;

	/// Marks the range of selected points in the Y direction.
	QwtPlotZoneItem* _selectionRangeIndicatorY;

	/// For deferred invocation of the plot repaint function.
	DeferredMethodInvocation<ScatterPlotModifierEditor, &ScatterPlotModifierEditor::plotScatterPlot> plotLater;
};

}	// End of namespace
}	// End of namespace
