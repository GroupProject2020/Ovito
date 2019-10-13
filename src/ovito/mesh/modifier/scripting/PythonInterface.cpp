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

#include <ovito/mesh/Mesh.h>
#include <ovito/pyscript/binding/PythonBinding.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/mesh/modifier/SurfaceMeshAffineTransformationModifierDelegate.h>
#include <ovito/mesh/modifier/SurfaceMeshReplicateModifierDelegate.h>
#include <ovito/mesh/modifier/SurfaceMeshSliceModifierDelegate.h>

namespace Ovito { namespace Mesh {

using namespace PyScript;

PYBIND11_MODULE(MeshModPython, m)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();

	py::options options;
	options.disable_function_signatures();

	ovito_class<SurfaceMeshAffineTransformationModifierDelegate, AffineTransformationModifierDelegate>{m};
	ovito_class<SurfaceMeshReplicateModifierDelegate, ReplicateModifierDelegate>{m};
	ovito_class<SurfaceMeshSliceModifierDelegate, SliceModifierDelegate>{m};
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(MeshModPython);

}	// End of namespace
}	// End of namespace
