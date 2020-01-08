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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/export/FileColumnParticleExporter.h>
#include <ovito/stdobj/io/PropertyOutputWriter.h>
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
	void insertPropertyItem(ParticlePropertyReference propRef, const QString& displayName, const ParticlesOutputColumnMapping& columnMapping);

	/// This writes the settings made in the UI back to the exporter.
	void saveChanges(FileColumnParticleExporter* particleExporter);

	QListWidget* _columnMappingWidget;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
