///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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


#include <ovito/particles/Particles.h>
#include "../ParticleExporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

/**
 * \brief Exporter that outputs the particles to a FHI-aims file.
 */
class OVITO_PARTICLES_EXPORT FHIAimsExporter : public ParticleExporter
{
	/// Defines a metaclass specialization for this exporter type.
	class OOMetaClass : public ParticleExporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using ParticleExporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the extension of files written by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("FHI-aims File"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(FHIAimsExporter, OOMetaClass)

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE FHIAimsExporter(DataSet* dataset) : ParticleExporter(dataset) {}

protected:

	/// \brief Writes the particles of one animation frame to the current output file.
	virtual bool exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation) override;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


