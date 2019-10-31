////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Alexander Stukowski
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
#include <ovito/pyscript/engine/AdhocApplication.h>
#include <ovito/pyscript/engine/ScriptEngine.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/utilities/concurrent/AsyncOperation.h>
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

			// Create an operation object that represents the script execution.
			static AsyncOperation scriptOperation(app->datasetContainer()->taskManager());
			scriptOperation.setProgressText(QStringLiteral("Script execution in progress"));
			scriptOperation.setStarted();

			// Install exit handler that deletes the application object on interpreter shutdown.
			Py_AtExit([]() {
				scriptOperation.setFinished();
				delete AdhocApplication::instance();
			});

			// Set up script execution environment.
			ScriptEngine::initializeExternalInterpreter(app->datasetContainer()->currentSet(), scriptOperation.task());
		}
		catch(const Exception& ex) {
			throw std::runtime_error(qPrintable(AdhocApplication::tr("%1 module initialization failed: %2")
				.arg(AdhocApplication::applicationName())
				.arg(ex.messages().join(QStringLiteral(" - ")))));
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
	m.attr("version_string") = py::cast(Application::applicationVersionString());

	// Make environment information available to the script.
	m.attr("gui_mode") = py::cast(Application::instance()->guiMode());
	m.attr("headless_mode") = py::cast(Application::instance()->headlessMode());

	// Set up the ad-hoc environment, which consist of the global DataSet and a TaskManager.

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
