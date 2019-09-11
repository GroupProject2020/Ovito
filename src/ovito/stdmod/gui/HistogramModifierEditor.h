///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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
#include <ovito/stdobj/gui/widgets/PropertyReferenceParameterUI.h>
#include <ovito/stdobj/gui/widgets/DataSeriesPlotWidget.h>
#include <ovito/core/utilities/DeferredMethodInvocation.h>

class QwtPlotZoneItem;

namespace Ovito { namespace StdMod {
/**
 * A properties editor for the HistogramModifier class.
 */
class HistogramModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(HistogramModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE HistogramModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Replots the histogram computed by the modifier.
	void plotHistogram();

private:

	/// The graph widget to display the histogram.
	DataSeriesPlotWidget* _plotWidget;

	/// The plot item for indicating the seletion range.
	QwtPlotZoneItem* _selectionRangeIndicator;

	/// For deferred invocation of the plot repaint function.
	DeferredMethodInvocation<HistogramModifierEditor, &HistogramModifierEditor::plotHistogram> plotHistogramLater;
};

}	// End of namespace
}	// End of namespace
