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

#include <ovito/particles/Particles.h>
#include <ovito/pyscript/binding/PythonBinding.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/particles/export/ParticleExporter.h>
#include <ovito/particles/export/imd/IMDExporter.h>
#include <ovito/particles/export/vasp/POSCARExporter.h>
#include <ovito/particles/export/xyz/XYZExporter.h>
#include <ovito/particles/export/lammps/LAMMPSDumpExporter.h>
#include <ovito/particles/export/lammps/LAMMPSDataExporter.h>
#include <ovito/particles/export/fhi_aims/FHIAimsExporter.h>
#include <ovito/particles/export/gsd/GSDExporter.h>
#include "PythonBinding.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace PyScript;

void defineExportersSubmodule(py::module m)
{
	ovito_abstract_class<ParticleExporter, FileExporter>{m}
	;

	ovito_abstract_class<FileColumnParticleExporter, ParticleExporter>(m)
		.def_property("columns", &FileColumnParticleExporter::columnMapping, &FileColumnParticleExporter::setColumnMapping)
	;

	ovito_class<IMDExporter, FileColumnParticleExporter>{m}
	;

	ovito_class<POSCARExporter, ParticleExporter>{m}
		.def_property("reduced", &POSCARExporter::writeReducedCoordinates, &POSCARExporter::setWriteReducedCoordinates)
	;

	ovito_class<LAMMPSDataExporter, ParticleExporter>{m}
		.def_property("_atom_style", &LAMMPSDataExporter::atomStyle, &LAMMPSDataExporter::setAtomStyle)
	;

	ovito_class<LAMMPSDumpExporter, FileColumnParticleExporter>{m}
	;

	auto XYZExporter_py = ovito_class<XYZExporter, FileColumnParticleExporter>{m}
		.def_property("sub_format", &XYZExporter::subFormat, &XYZExporter::setSubFormat)
	;

	ovito_enum<XYZExporter::XYZSubFormat>(XYZExporter_py, "XYZSubFormat")
		.value("Parcas", XYZExporter::ParcasFormat)
		.value("Extended", XYZExporter::ExtendedFormat)
	;

	ovito_class<FHIAimsExporter, ParticleExporter>{m}
	;

	ovito_class<GSDExporter, ParticleExporter>{m}
	;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
