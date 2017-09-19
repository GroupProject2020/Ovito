///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/export/ParticleExporter.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief Exporter that exports dislocation lines to a Crystal Analysis Tool (CA) file.
 */
class OVITO_CRYSTALANALYSIS_EXPORT CAExporter : public ParticleExporter
{
	Q_OBJECT
	OVITO_CLASS(CAExporter)
	
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE CAExporter(DataSet* dataset) : ParticleExporter(dataset) {}

	/// \brief Returns the file filter that specifies the files that can be exported by this service.
	virtual QString fileFilter() override { 
#ifndef Q_OS_WIN
		return QStringLiteral("*.ca");
#else 
		// Workaround for bug in Windows file selection dialog (https://bugreports.qt.io/browse/QTBUG-45759)
		return QStringLiteral("*");
#endif
	}

	/// \brief Returns the filter description that is displayed in the drop-down box of the file dialog.
	virtual QString fileFilterDescription() override { return tr("Crystal Analysis File"); }

	/// Returns whether the DXA defect mesh is exported (in addition to the dislocation lines).
	bool meshExportEnabled() const { return _meshExportEnabled; }

	/// Sets whether the DXA defect mesh is exported (in addition to the dislocation lines).
	void setMeshExportEnabled(bool enable) { _meshExportEnabled = enable; }

protected:

	/// \brief Writes the particles of one animation frame to the current output file.
	virtual bool exportObject(SceneNode* sceneNode, int frameNumber, TimePoint time, const QString& filePath, TaskManager& taskManager) override;
	
private:

	/// Controls whether the DXA defect mesh is exported (in addition to the dislocation lines).
	bool _meshExportEnabled = true;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


