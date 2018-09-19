///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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

#include <plugins/pyscript/PyScript.h>
#include <plugins/pyscript/engine/AdhocApplication.h>
#include <plugins/pyscript/engine/ScriptEngine.h>
#include <core/dataset/DataSetContainer.h>
#include "PythonBinding.h"

namespace PyScript {

void defineAppSubmodule(py::module parentModule);	// Defined in AppBinding.cpp
void defineSceneSubmodule(py::module parentModule);	// Defined in SceneBinding.cpp
void defineIOSubmodule(py::module parentModule);	// Defined in FileIOBinding.cpp
void defineViewportSubmodule(py::module parentModule);	// Defined in ViewportBinding.cpp
void defineAnimationSubmodule(py::module parentModule);	// Defined in AnimationBinding.cpp
void defineRenderingSubmodule(py::module parentModule);	// Defined in RenderingBinding.cpp

using namespace Ovito;

PYBIND11_MODULE(PyScript, m)
{
	py::options options;
	options.disable_function_signatures();

	// Register Ovito-to-Python exception translator.
	py::register_exception_translator([](std::exception_ptr p) {
		try {
			if(p) std::rethrow_exception(p);
		}
		catch(const Exception& ex) {
			PyErr_SetString(PyExc_RuntimeError, ex.messages().join(QChar('\n')).toUtf8().constData());
		}
	});

	// Initialize an ad-hoc environment when this module has been imported by an external Python interpreter and is not running as a standalone app. 
	// Otherwise an environment is already provided by the StandaloneApplication class.
	if(!Application::instance()) {
		try {
			AdhocApplication* app = new AdhocApplication(); // This will leak, but it doesn't matter because this Python module will never be unloaded.
			if(!app->initialize())
				throw Exception("OVITO application object could not be initialized.");
			OVITO_ASSERT(Application::instance() == app);

			// Create a global QCoreApplication object if there isn't one already.
			// This application object is needed for event processing (e.g. QEventLoop).
			if(!QCoreApplication::instance()) {
				static int argc = 1;
				static char* argv[] = { const_cast<char*>("") };
				app->createQtApplication(argc, argv);
			}

			// Create the global ScriptEngine instance.
			ScriptEngine::createAdhocEngine(app->datasetContainer()->currentSet());
		}
		catch(const Exception& ex) {
			ex.logError();
			throw std::runtime_error("Error during OVITO runtime environment initialization.");
		}
	}
	OVITO_ASSERT(QCoreApplication::instance() != nullptr);

	// Register submodules.
	defineAppSubmodule(m);
	defineSceneSubmodule(m);
	defineAnimationSubmodule(m);
	defineIOSubmodule(m);
	defineViewportSubmodule(m);
	defineRenderingSubmodule(m);

	// Make Ovito program version number available to script.
	m.attr("version") = py::make_tuple(Application::applicationVersionMajor(), Application::applicationVersionMinor(), Application::applicationVersionRevision());
	m.attr("version_string") = py::cast(QCoreApplication::applicationVersion());

	// Make environment information available to the script.
	m.attr("gui_mode") = py::cast(Application::instance()->guiMode());
	m.attr("headless_mode") = py::cast(Application::instance()->headlessMode());

	// Set up the ad-hoc environment, which consist of the global DataSet and a TaskManager.
	// The enviroment may get updated later on by the ScriptEngine::execute() method.

	// Add an attribute to the ovito module that provides access to the active dataset.
	DataSet* activeDataset = Application::instance()->datasetContainer()->currentSet();
	m.attr("scene") = py::cast(activeDataset, py::return_value_policy::reference);

	// This is for backward compatibility with OVITO 2.9.0:
	m.attr("dataset") = py::cast(activeDataset, py::return_value_policy::reference);
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(PyScript);

/// Helper method that checks if the given data object is safe to modify without unwanted side effects.
/// If it is not, an exception is raised to inform the user that a mutable version of the data object
/// should be explicitly requested.
void ensureDataObjectIsMutable(DataObject& obj)
{
	if(!obj.isSafeToModify()) {
		QString className = py::cast<QString>(py::str(py::cast(obj).attr("__class__").attr("__name__")));
		obj.throwException(QStringLiteral("You tried to modify a %1 object that is currently shared by multiple owners. "
			"Please explicitly request a mutable version of the data object by using the '_' notation.").arg(className));
	}
}

} // End of namespace
