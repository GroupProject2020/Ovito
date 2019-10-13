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

#include <ovito/particles/Particles.h>
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
