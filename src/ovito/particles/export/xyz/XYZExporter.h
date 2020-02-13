////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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


#include <ovito/particles/Particles.h>
#include "../FileColumnParticleExporter.h"

namespace Ovito { namespace Particles {

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
	virtual bool exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation) override;

private:

	/// Selects the kind of XYZ file to write.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(XYZSubFormat, subFormat, setSubFormat, PROPERTY_FIELD_MEMORIZE);
};

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::XYZExporter::XYZSubFormat);
Q_DECLARE_TYPEINFO(Ovito::Particles::XYZExporter::XYZSubFormat, Q_PRIMITIVE_TYPE);


