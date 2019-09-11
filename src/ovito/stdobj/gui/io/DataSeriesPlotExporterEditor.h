///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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


#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/gui/properties/PropertiesEditor.h>

namespace Ovito { namespace StdObj {

/**
 * \brief User interface component for the DataSeriesPlotExporter class.
 */
class DataSeriesPlotExporterEditor : public PropertiesEditor
{
	OVITO_CLASS(DataSeriesPlotExporterEditor)
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE DataSeriesPlotExporterEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;
};

}	// End of namespace
}	// End of namespace
