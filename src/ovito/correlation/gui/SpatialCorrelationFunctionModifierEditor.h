////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
//  Copyright 2017 Lars Pastewka
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
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/stdobj/gui/widgets/DataTablePlotWidget.h>
#include <ovito/gui/desktop/properties/ModifierPropertiesEditor.h>
#include <ovito/core/utilities/DeferredMethodInvocation.h>

class QwtPlot;
class QwtPlotCurve;

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A properties editor for the SpatialCorrelationFunctionModifier class.
 */
class SpatialCorrelationFunctionModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(SpatialCorrelationFunctionModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE SpatialCorrelationFunctionModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// Replots one of the correlation function computed by the modifier.
	std::pair<FloatType,FloatType> plotData(const DataTable* table, DataTablePlotWidget* plotWidget, FloatType offset, FloatType fac, ConstPropertyAccess<FloatType> normalization);

protected Q_SLOTS:

	/// Replots the correlation function computed by the modifier.
	void plotAllData();

private:

	/// The plotting widget for displaying the computed real-space correlation function.
	DataTablePlotWidget* _realSpacePlot;

	/// The plotting widget for displaying the computed reciprocal-space correlation function.
	DataTablePlotWidget* _reciprocalSpacePlot;

	/// The plot item for the short-ranged part of the real-space correlation function.
    QwtPlotCurve* _neighCurve = nullptr;

	/// For deferred invocation of the plot repaint function.
	DeferredMethodInvocation<SpatialCorrelationFunctionModifierEditor, &SpatialCorrelationFunctionModifierEditor::plotAllData> plotAllDataLater;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
