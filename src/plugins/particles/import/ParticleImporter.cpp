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

#include <plugins/particles/Particles.h>
#include "ParticleImporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import)

IMPLEMENT_OVITO_CLASS(ParticleImporter);	
DEFINE_PROPERTY_FIELD(ParticleImporter, isMultiTimestepFile);
DEFINE_PROPERTY_FIELD(ParticleImporter, sortParticles);
SET_PROPERTY_FIELD_LABEL(ParticleImporter, isMultiTimestepFile, "File contains multiple timesteps");
SET_PROPERTY_FIELD_LABEL(ParticleImporter, sortParticles, "Sort particles by ID");

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ParticleImporter::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(isMultiTimestepFile)) {
		// Automatically rescan input file for animation frames when this option has been changed.
		requestFramesUpdate();
	}
	else if(field == PROPERTY_FIELD(sortParticles)) {
		// Automatically reload input file when this option has been changed.
		requestReload();
	}
	FileSourceImporter::propertyChanged(field);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
