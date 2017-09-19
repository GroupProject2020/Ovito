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

PYBIND11_PLUGIN(PyScript)
{
	py::options options;
	options.disable_function_signatures();

	py::module m("PyScript");

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

	// Add an attribute to the ovito module that provides access to the active dataset.
	DataSet* activeDataset = Application::instance()->datasetContainer()->currentSet();
	m.attr("dataset") = py::cast(activeDataset, py::return_value_policy::reference);

	// Add an attribute to the ovito module that provides access to the global task manager.
	TaskManager* activeTaskManager = &Application::instance()->datasetContainer()->taskManager();
	m.attr("task_manager") = py::cast(activeTaskManager, py::return_value_policy::reference);

	return m.ptr();
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(PyScript);

/// Helper function that converts a Python string to a C++ PropertyReference instance.
/// The function requires a property class to look up the property name string.
PropertyReference convertPythonPropertyReference(py::object src, const PropertyClass* propertyClass)
{
	if(src.is_none())
		return {};
	if(!propertyClass)
		throw Exception(QStringLiteral("Cannot set property field without an active property class."));

	try {
		int ptype = src.cast<int>();
		if(ptype == 0)
			throw Exception(QStringLiteral("User-defined property without a name is not acceptable."));
		if(propertyClass->standardProperties().contains(ptype) == false)
			throw Exception(QStringLiteral("%1 is not a valid standard property type ID.").arg(ptype));
		return PropertyReference(propertyClass, ptype);
	}
	catch(const py::cast_error&) {}

	QString str;
	try {
		str = src.cast<QString>();
	}
	catch(const py::cast_error&) {
		throw Exception(QStringLiteral("Invalid property name. Expected a string."));
	}

	QStringList parts = str.split(QChar('.'));
	if(parts.length() > 2)
		throw Exception(QStringLiteral("Too many dots in property name string."));
	else if(parts.length() == 0 || parts[0].isEmpty())
		throw Exception(QStringLiteral("Invalid property name. String is empty."));

	// Determine property type.
	QString name = parts[0];
	int type = propertyClass->standardPropertyIds().value(name, 0);

	// Determine vector component.
	int component = -1;
	if(parts.length() == 2) {
		// First try to convert component to integer.
		bool ok;
		component = parts[1].toInt(&ok);
		if(!ok) {
			if(type != 0) {
				// Perhaps the standard property's component name was used instead of an integer.
				const QString componentName = parts[1].toUpper();
				QStringList standardNames = propertyClass->standardPropertyComponentNames(type);
				component = standardNames.indexOf(componentName);
				if(component < 0)
					throw Exception(QStringLiteral("Component name '%1' is not defined for property '%2'. Possible components are: %3").arg(parts[1]).arg(parts[0]).arg(standardNames.join(',')));
			}
			else {
				// Assume user-defined properties cannot be vectors.
				component = -1;
				name = parts.join(QChar('.'));
			}
		}
	}
	if(type == 0)
		return PropertyReference(propertyClass, name, component);
	else
		return PropertyReference(propertyClass, type, component);
}

};
