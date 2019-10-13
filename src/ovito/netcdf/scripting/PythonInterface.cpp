////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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

#include <ovito/pyscript/PyScript.h>
#include <ovito/pyscript/binding/PythonBinding.h>
#include <ovito/netcdf/AMBERNetCDFImporter.h>
#include <ovito/netcdf/AMBERNetCDFExporter.h>
#include <ovito/particles/scripting/PythonBinding.h>
#include <ovito/core/app/PluginManager.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace PyScript;

PYBIND11_MODULE(NetCDFPluginPython, m)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();

	py::options options;
	options.disable_function_signatures();

	ovito_class<AMBERNetCDFImporter, ParticleImporter>(m)
		.def_property("custom_column_mapping", &AMBERNetCDFImporter::customColumnMapping, &AMBERNetCDFImporter::setCustomColumnMapping)
		.def_property("use_custom_column_mapping", &AMBERNetCDFImporter::useCustomColumnMapping, &AMBERNetCDFImporter::setUseCustomColumnMapping)
	;

	ovito_class<AMBERNetCDFExporter, FileColumnParticleExporter>{m}
	;
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(NetCDFPluginPython);

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
