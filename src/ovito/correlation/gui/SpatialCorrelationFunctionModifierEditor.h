///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
//  Copyright (2017) Lars Pastewka
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


#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/stdobj/gui/widgets/DataSeriesPlotWidget.h>
#include <ovito/gui/properties/ModifierPropertiesEditor.h>
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
	std::pair<FloatType,FloatType> plotData(const DataSeriesObject* series, DataSeriesPlotWidget* plotWidget, FloatType offset, FloatType fac, const ConstPropertyPtr& normalization);

protected Q_SLOTS:

	/// Replots the correlation function computed by the modifier.
	void plotAllData();

private:

	/// The plotting widget for displaying the computed real-space correlation function.
	DataSeriesPlotWidget* _realSpacePlot;

	/// The plotting widget for displaying the computed reciprocal-space correlation function.
	DataSeriesPlotWidget* _reciprocalSpacePlot;

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
