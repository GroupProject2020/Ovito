///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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


#include <plugins/particles/Particles.h>
#include "../FileColumnParticleExporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

/**
 * \brief Exporter that writes the particles to a LAMMPS data file.
 */
class OVITO_PARTICLES_EXPORT XYZExporter : public FileColumnParticleExporter
{
	/// Defines a metaclass specialization for this exporter type.
	class OOMetaClass : public FileColumnParticleExporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using FileColumnParticleExporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the extension of files written by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*"); }
	
		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("XYZ File"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(XYZExporter, OOMetaClass)

public:

	/// \brief The supported XYZ sub-formats.
	enum XYZSubFormat {
		ParcasFormat,
		ExtendedFormat
	};
	Q_ENUMS(XYZSubFormat);

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE XYZExporter(DataSet* dataset) : FileColumnParticleExporter(dataset), _subFormat(ExtendedFormat) {}

	/// \brief Indicates whether this file exporter can write more than one animation frame into a single output file.
	virtual bool supportsMultiFrameFiles() const override { return true; }

protected:

	/// \brief Writes the particles of one animation frame to the current output file.
	virtual bool exportObject(SceneNode* sceneNode, int frameNumber, TimePoint time, const QString& filePath, TaskManager& taskManager) override;

private:

	/// Selects the kind of XYZ file to write.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(XYZSubFormat, subFormat, setSubFormat, PROPERTY_FIELD_MEMORIZE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::XYZExporter::XYZSubFormat);
Q_DECLARE_TYPEINFO(Ovito::Particles::XYZExporter::XYZSubFormat, Q_PRIMITIVE_TYPE);


