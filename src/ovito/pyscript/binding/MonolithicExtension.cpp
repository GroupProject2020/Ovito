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
#include "PythonBinding.h"

namespace PyScript {

PYBIND11_MODULE(ovito_bindings, m)
{
	// Build a list of all modules that have been compiled into the extension library.
	std::vector<const PythonPluginRegistration*> pluginList;
	for(const PythonPluginRegistration* r = PythonPluginRegistration::linkedlist; r != nullptr; r = r->_next)
		pluginList.push_back(r);

	// Initialize the modules in reverse order to obey the correct interdependecies
	// and register them in the sys.modules dictionary.
	py::module ovito_plugins_module = py::module::import("ovito.plugins");
	py::module sys_module = py::module::import("sys");
	py::object sys_modules_dict = sys_module.attr("modules");
	for(auto r = pluginList.crbegin(); r != pluginList.crend(); ++r) {
		const std::string& fullModuleName = (*r)->_moduleName;
		py::module mod = py::reinterpret_steal<py::module>((*r)->_initFunc());
		sys_modules_dict[py::cast(fullModuleName)] = mod;
		// Also add the sub-module to the ovito.plugins parent module.
		py::object moduleName = py::cast(fullModuleName.substr(fullModuleName.rfind('.')+1));
		ovito_plugins_module.attr(moduleName) = mod;
	}
}


} // End of namespace
