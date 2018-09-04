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


#include <gui/GUI.h>
#include <plugins/stdobj/gui/widgets/DataSeriesPlotWidget.h>
#include <gui/properties/ModifierPropertiesEditor.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <core/utilities/DeferredMethodInvocation.h>

class QwtPlotSpectrogram;
class QwtMatrixRasterData;

namespace Ovito { namespace Grid { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace Ovito::StdObj;

/**
 * A properties editor for the SpatialBinningModifier class.
 */
class SpatialBinningModifierEditor : public ModifierPropertiesEditor
{
	OVITO_CLASS(SpatialBinningModifierEditor)
	Q_OBJECT
	
public:

	/// Default constructor.
	Q_INVOKABLE SpatialBinningModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Plots the data computed by the modifier.
	void plotData();

    /// Enable/disable the editor for number of y-bins and the first derivative button.
    void updateWidgets();

	/// This is called when the user has clicked the "Save Data" button.
	void onSaveData();

private:

    /// Widget controlling computation of the first derivative.
    BooleanParameterUI* _firstDerivativePUI;

    /// Widget controlling the number of y-bins.
    IntegerParameterUI* _numBinsYPUI;

    /// Widget controlling the number of z-bins.
    IntegerParameterUI* _numBinsZPUI;

	/// The graph widget to display the data.
	DataSeriesPlotWidget* _plot;

	/// The plot item for the 2D color plot.
	QwtPlotSpectrogram* _plotRaster = nullptr;

	/// The data storage for the 2D color plot.
	QwtMatrixRasterData* _rasterData = nullptr;

	/// For deferred invocation of the plot repaint function.
	DeferredMethodInvocation<SpatialBinningModifierEditor, &SpatialBinningModifierEditor::plotData> plotLater;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
