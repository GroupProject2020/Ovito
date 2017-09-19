///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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
#include <gui/properties/ModifierPropertiesEditor.h>
#include <gui/properties/PropertyReferenceParameterUI.h>
#include <core/utilities/DeferredMethodInvocation.h>

class QwtPlot;
class QwtPlotCurve;
class QwtPlotZoneItem;

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene) OVITO_BEGIN_INLINE_NAMESPACE(StdObj) OVITO_BEGIN_INLINE_NAMESPACE(Internal)
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

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

protected Q_SLOTS:

	/// Replots the histogram computed by the modifier.
	void plotHistogram();

	/// This is called when the user has clicked the "Save Data" button.
	void onSaveData();

private:

	/// The graph widget to display the histogram.
	QwtPlot* _histogramPlot;

	/// The plot item for the histogram.
    QwtPlotCurve* _plotCurve = nullptr;

	/// The plot item for indicating the seletion range.
	QwtPlotZoneItem* _selectionRange = nullptr;

	/// For deferred invocation of the plot repaint function.
	DeferredMethodInvocation<HistogramModifierEditor, &HistogramModifierEditor::plotHistogram> plotHistogramLater;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
