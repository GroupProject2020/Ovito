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
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief Exporter that exports dislocation lines to a Crystal Analysis Tool (CA) file.
 */
class OVITO_CRYSTALANALYSIS_EXPORT CAExporter : public ParticleExporter
{
	/// Defines a metaclass specialization for this exporter type.
	class OOMetaClass : public ParticleExporter::OOMetaClass
	{
	public:

		/// Inherit standard constructor from base meta class.
		using ParticleExporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the extension of files written by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*.ca"); }
	
		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("Crystal Analysis File"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(CAExporter, OOMetaClass)

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE CAExporter(DataSet* dataset) : ParticleExporter(dataset) {}

	/// Returns whether the DXA defect mesh is exported (in addition to the dislocation lines).
	bool meshExportEnabled() const { return _meshExportEnabled; }

	/// Sets whether the DXA defect mesh is exported (in addition to the dislocation lines).
	void setMeshExportEnabled(bool enable) { _meshExportEnabled = enable; }

	/// \brief Returns the type of data objects that this exporter service can export.
	virtual const DataObject::OOMetaClass* exportableDataObjectClass() const override {
		return &DislocationNetworkObject::OOClass();
	}

protected:

	/// \brief Writes the particles of one animation frame to the current output file.
	virtual bool exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation) override;
	
private:

	/// Controls whether the DXA defect mesh is exported (in addition to the dislocation lines).
	bool _meshExportEnabled = true;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


