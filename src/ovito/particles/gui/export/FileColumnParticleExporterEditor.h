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


#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/export/OutputColumnMapping.h>
#include <ovito/particles/export/FileColumnParticleExporter.h>
#include <ovito/gui/properties/PropertiesEditor.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export)

/**
 * \brief User interface component for the FileColumnParticleExporter class.
 */
class FileColumnParticleExporterEditor : public PropertiesEditor
{
	OVITO_CLASS(FileColumnParticleExporterEditor)
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE FileColumnParticleExporterEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

private Q_SLOTS:

	/// Updates the displayed list of particle properties that are available for export.
	void updateParticlePropertiesList();

	/// Is called when the user checked/unchecked an item in the particle property list.
	void onParticlePropertyItemChanged();

private:

	/// Populates the column mapping list box with an entry.
	void insertPropertyItem(ParticlePropertyReference propRef, const QString& displayName, const OutputColumnMapping& columnMapping);

	/// This writes the settings made in the UI back to the exporter.
	void saveChanges(FileColumnParticleExporter* particleExporter);

	QListWidget* _columnMappingWidget;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


