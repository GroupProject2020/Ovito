////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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
