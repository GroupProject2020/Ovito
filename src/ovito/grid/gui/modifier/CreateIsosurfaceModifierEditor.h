////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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
#include <ovito/gui/properties/ModifierPropertiesEditor.h>
#include <ovito/stdobj/gui/widgets/DataTablePlotWidget.h>
#include <ovito/core/utilities/DeferredMethodInvocation.h>

class QwtPlotMarker;

namespace Ovito { namespace Grid { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief A properties editor for the CreateIsosurfaceModifier class.
 */
class CreateIsosurfaceModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(CreateIsosurfaceModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE CreateIsosurfaceModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Replots the value histogram computed by the modifier.
	void plotHistogram();

private:

	/// The graph widget to display the histogram.
	StdObj::DataTablePlotWidget* _plotWidget;

	/// The plot item for indicating the current iso level value.
	QwtPlotMarker* _isoLevelIndicator;

	/// For deferred invocation of the plot repaint function.
	DeferredMethodInvocation<CreateIsosurfaceModifierEditor, &CreateIsosurfaceModifierEditor::plotHistogram> plotHistogramLater;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
