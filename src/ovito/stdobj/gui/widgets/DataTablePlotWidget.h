////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/stdobj/table/DataTable.h>
#include <ovito/core/oo/RefTargetListener.h>

#include <qwt/qwt_plot.h>
#include <qwt/qwt_scale_draw.h>

class QwtPlotCurve;
class QwtPlotLegendItem;
class QwtPlotBarChart;
class QwtPlotSpectroCurve;

namespace Ovito { namespace StdObj {

/**
 * \brief A widget that plots the data of a DataTable.
 */
class OVITO_STDOBJGUI_EXPORT DataTablePlotWidget : public QwtPlot
{
	Q_OBJECT

public:

	/// Constructor.
	DataTablePlotWidget(QWidget* parent = nullptr);

	/// Returns the data table object currently being plotted.
	const DataTable* table() const { return _table; }

	/// Sets the data table to be plotted.
	void setTable(const DataTable* table);

	/// Resets the plot.
	void reset() {
		if(_table) {
			_table.reset();
			updateDataPlot();
		}
	}

private Q_SLOTS:

	/// Regenerates the plot.
	/// This function is called whenever a new data table has been loaded into widget or if the current table data changes.
	void updateDataPlot();

private:

	/// A custom scale draw implementation for drawing the axis labels of a bar chart.
	class BarChartScaleDraw : public QwtScaleDraw
	{
	public:

		/// Constructor.
		using QwtScaleDraw::QwtScaleDraw;

		/// Sets the texts of the labels.
		void setLabels(QStringList labels) {
			_labels = std::move(labels);
			invalidateCache();
		}

		/// Returns the label text for the given axis position.
		virtual QwtText label(double value) const override {
			QwtText lbl;
			int index = qRound(value);
			if(index >= 0 && index < _labels.size())
				lbl = _labels[index];
			return lbl;
		}

	private:

		QStringList _labels;
	};

private:

	/// Reference to the current data table shown in the plot widget.
	OORef<DataTable> _table;

	/// The plot item(s) for standard line charts.
    std::vector<QwtPlotCurve*> _curves;

	/// The plot item(s) for scatter plots.
    std::vector<QwtPlotSpectroCurve*> _spectroCurves;

	/// The plot item for bar charts.
	QwtPlotBarChart* _barChart = nullptr;

	/// The scale draw used when plotting a bar chart.
	BarChartScaleDraw* _barChartScaleDraw = nullptr;

	/// The plot legend.
	QwtPlotLegendItem* _legend = nullptr;
};

}	// End of namespace
}	// End of namespace
