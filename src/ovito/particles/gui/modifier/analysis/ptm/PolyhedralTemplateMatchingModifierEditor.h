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


#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/stdobj/gui/widgets/DataTablePlotWidget.h>
#include <ovito/gui/desktop/properties/ModifierPropertiesEditor.h>
#include <ovito/core/utilities/DeferredMethodInvocation.h>

class QwtPlotZoneItem;

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief A properties editor for the PolyhedralTemplateMatchingModifier class.
 */
class PolyhedralTemplateMatchingModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(PolyhedralTemplateMatchingModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE PolyhedralTemplateMatchingModifierEditor() {}

protected Q_SLOTS:

	/// Replots the histogram computed by the modifier.
	void plotHistogram();

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

private:

	/// The graph widget to display the RMSD histogram.
	DataTablePlotWidget* _rmsdPlotWidget;

	/// Marks the RMSD cutoff in the histogram plot.
	QwtPlotZoneItem* _rmsdRangeIndicator;

	/// For deferred invocation of the plot repaint function.
	DeferredMethodInvocation<PolyhedralTemplateMatchingModifierEditor, &PolyhedralTemplateMatchingModifierEditor::plotHistogram> plotHistogramLater;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
