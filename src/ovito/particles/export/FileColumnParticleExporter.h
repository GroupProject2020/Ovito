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


#include <ovito/particles/Particles.h>
#include <ovito/stdobj/io/PropertyOutputWriter.h>
#include "ParticleExporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export)

using ParticlesOutputColumnMapping = TypedOutputColumnMapping<ParticlesObject>;

/**
 * \brief Abstract base class for export services that can export an arbitrary list of particle properties.
 */
class OVITO_PARTICLES_EXPORT FileColumnParticleExporter : public ParticleExporter
{
	Q_OBJECT
	OVITO_CLASS(FileColumnParticleExporter)

protected:

	/// \brief Constructs a new instance of this class.
	FileColumnParticleExporter(DataSet* dataset) : ParticleExporter(dataset) {}

public:

	/// \brief Returns the mapping of particle properties to output file columns.
	const ParticlesOutputColumnMapping& columnMapping() const { return _columnMapping; }

	/// \brief Sets the mapping of particle properties to output file columns.
	void setColumnMapping(const ParticlesOutputColumnMapping& mapping) { _columnMapping = mapping; }

	/// \brief Loads the user-defined default values of this object's parameter fields from the
	///        application's settings store.
	virtual void loadUserDefaults() override;

public:

	Q_PROPERTY(Ovito::Particles::ParticlesOutputColumnMapping columnMapping READ columnMapping WRITE setColumnMapping);

private:

	/// The mapping of particle properties to output file columns.
	ParticlesOutputColumnMapping _columnMapping;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::ParticlesOutputColumnMapping);
