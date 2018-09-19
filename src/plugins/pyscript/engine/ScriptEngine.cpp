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
#include <plugins/pyscript/binding/PythonBinding.h>
#include <core/app/PluginManager.h>
#include <core/app/Application.h>
#include <core/dataset/DataSetContainer.h>
#include "ScriptEngine.h"

namespace PyScript {

/// Points to the script engine that is currently active (i.e. which is executing a script).
std::shared_ptr<ScriptEngine> ScriptEngine::_activeEngine;

/// Head of linked list containing all initXXX functions. 
PythonPluginRegistration* PythonPluginRegistration::linkedlist = nullptr;

/******************************************************************************
* Initializes the scripting engine and sets up the environment.
******************************************************************************/
ScriptEngine::ScriptEngine(DataSet* dataset) : _dataset(dataset)
{
	try {
		// Initialize our embedded Python interpreter if it isn't running already.
		if(!Py_IsInitialized())
			initializeEmbeddedInterpreter();
	}
	catch(py::error_already_set& ex) {
		ex.restore();
		if(PyErr_Occurred())
			PyErr_PrintEx(0);
		throw Exception(tr("Failed to initialize Python interpreter."), dataset);
	}
	catch(Exception& ex) {
		ex.setContext(dataset);
		throw;
	}
	catch(const std::exception& ex) {
		throw Exception(tr("Failed to initialize Python interpreter. %1").arg(ex.what()), dataset);
	}
}

/******************************************************************************
* Destructor.
******************************************************************************/
ScriptEngine::~ScriptEngine()
{
}

/******************************************************************************
* Initializes the embedded Python interpreter and sets up the global namespace.
******************************************************************************/
void ScriptEngine::initializeEmbeddedInterpreter()
{
	// This is a one-time global initialization.
	static bool isInterpreterInitialized = false; 
	if(isInterpreterInitialized)
		return;	// Interpreter is already initialized.

	try {

		// Call Py_SetProgramName() because the Python interpreter uses the path of the main executable to determine the
		// location of Python standard library, which gets shipped with the static build of OVITO.
#if PY_MAJOR_VERSION >= 3
		static std::wstring programName = QDir::toNativeSeparators(QCoreApplication::applicationFilePath()).toStdWString();
		Py_SetProgramName(const_cast<wchar_t*>(programName.data()));
#else
		static QByteArray programName = QDir::toNativeSeparators(QCoreApplication::applicationFilePath()).toLocal8Bit();
		Py_SetProgramName(programName.data());
#endif

		// Make our internal script modules available by registering their initXXX functions with the Python interpreter.
		// This is required for static builds where all Ovito plugins are linked into the main executable file.
		// On Windows this is needed, because OVITO plugins have an .dll extension and the Python interpreter 
		// only looks for modules that have a .pyd extension.
		for(const PythonPluginRegistration* r = PythonPluginRegistration::linkedlist; r != nullptr; r = r->_next) {
			const char* name = r->_moduleName.c_str();
			// Note: const_cast<> is used for backward compatibility with Python 2.6.
			PyImport_AppendInittab(const_cast<char*>(name), r->_initFunc);
		}

		// Initialize the Python interpreter.
		py::initialize_interpreter();

		py::module sys_module = py::module::import("sys");

#ifdef OVITO_BUILD_MONOLITHIC
		// Let the ovito.plugins module know that it is running in a statically linked
		// interpreter.
		sys_module.attr("__OVITO_BUILD_MONOLITHIC") = py::cast(true);
#endif		

		// Install output redirection (don't do this in console mode as it interferes with the interactive interpreter).
		if(Application::instance()->guiMode()) {
			// Register the output redirector class.
			py::class_<InterpreterStdOutputRedirector>(sys_module, "__StdOutStreamRedirectorHelper")
					.def("write", &InterpreterStdOutputRedirector::write)
					.def("flush", &InterpreterStdOutputRedirector::flush);
			py::class_<InterpreterStdErrorRedirector>(sys_module, "__StdErrStreamRedirectorHelper")
					.def("write", &InterpreterStdErrorRedirector::write)
					.def("flush", &InterpreterStdErrorRedirector::flush);
			// Replace stdout and stderr streams.
			sys_module.attr("stdout") = py::cast(new InterpreterStdOutputRedirector(), py::return_value_policy::take_ownership);
			sys_module.attr("stderr") = py::cast(new InterpreterStdErrorRedirector(), py::return_value_policy::take_ownership);
		}

		// Determine path where Python source files are located.
		QDir prefixDir(QCoreApplication::applicationDirPath());
#if defined(Q_OS_WIN)
		QString pythonModulePath = prefixDir.absolutePath() + QStringLiteral("/plugins/python");
#elif defined(Q_OS_MAC)
		QString pythonModulePath = prefixDir.absolutePath() + QStringLiteral("/../Resources/python");
#else
		QString pythonModulePath = prefixDir.absolutePath() + QStringLiteral("/../lib/ovito/plugins/python");
#endif

		// Prepend directory containing OVITO's Python source files to sys.path.
		py::object sys_path = sys_module.attr("path");
		PyList_Insert(sys_path.ptr(), 0, py::cast(QDir::toNativeSeparators(pythonModulePath)).ptr());

		// Prepend current directory to sys.path.
		PyList_Insert(sys_path.ptr(), 0, py::str().ptr());
	}
	catch(const Exception&) {
		throw;
	}
	catch(py::error_already_set& ex) {
		ex.restore();
		if(PyErr_Occurred())
			PyErr_PrintEx(0);
		throw Exception(tr("Failed to initialize Python interpreter. %1").arg(ex.what()), dataset());
	}
	catch(const std::exception& ex) {
		throw Exception(tr("Failed to initialize Python interpreter: %1").arg(ex.what()), dataset());
	}
	catch(...) {
		throw Exception(tr("Unhandled exception thrown by Python interpreter."), dataset());
	}

	isInterpreterInitialized = true;
}

/******************************************************************************
* Determine the DataSet from the context the current script is running in.
******************************************************************************/
DataSet* ScriptEngine::getCurrentDataset()
{
	if(!activeEngine())
		throw Exception("Invalid interpreter state. There is no active dataset.");
	return activeEngine()->dataset();
}

/******************************************************************************
* Creates a global script engine instance. This is used when loading the 
* OVITO Python modules from an external interpreter.
******************************************************************************/
void ScriptEngine::createAdhocEngine(DataSet* dataset)
{
	OVITO_ASSERT(Py_IsInitialized());
	OVITO_ASSERT(!_activeEngine);

	// Create the global engine instance.
	_activeEngine = createEngine(dataset);

	// Inform the application that script execution is in progress.
	// Any OVITO objects created by a script will get initialized to their hard-coded default values.
	Application::instance()->scriptExecutionStarted();
}

/******************************************************************************
* Executes the given C++ function, which in turn may invoke Python functions in the
* context of this engine, and catches possible exceptions.
******************************************************************************/
int ScriptEngine::execute(const std::function<void()>& func)
{
	OVITO_ASSERT(dataset());
	OVITO_ASSERT(dataset()->container());

	if(QCoreApplication::instance() && QThread::currentThread() != QCoreApplication::instance()->thread())
		throw Exception(tr("Python scripts can only be run from the main thread."), dataset());

	// Inform the application that a script execution has started.
	// Any OVITO objects created by a script will get initialized to their hard-coded default values.
	Application::instance()->scriptExecutionStarted();

	// Keep track which script engine was active before this function was called. 
	std::shared_ptr<ScriptEngine> previousEngine = std::move(_activeEngine);
	// And mark this engine as the active one.
	_activeEngine = shared_from_this();

	int returnValue = 0;
	try {
		// Get reference to the main ovito Python module.
		py::module ovito_module = py::module::import("ovito");

		// Add an attribute to the ovito module that provides access to the active dataset.
		py::setattr(ovito_module, "scene", py::cast(dataset(), py::return_value_policy::reference));

		// This is for backward compatibility with OVITO 2.9.0:
		py::setattr(ovito_module, "dataset", py::cast(dataset(), py::return_value_policy::reference));
		
		try {
			func();
		}
		catch(py::error_already_set& ex) {
			returnValue = handlePythonException(ex);
		}
		catch(Exception& ex) {
			ex.setContext(dataset());
			throw;
		}
		catch(const std::exception& ex) {
			throw Exception(tr("Script execution error: %1").arg(ex.what()), dataset());
		}
		catch(...) {
			throw Exception(tr("Unhandled exception thrown by Python interpreter."), dataset());
		}
	}
	catch(...) {
		_activeEngine = std::move(previousEngine);
		Application::instance()->scriptExecutionStopped();
		throw;
	}
	_activeEngine = std::move(previousEngine);
	Application::instance()->scriptExecutionStopped();
	return 0;
}

/******************************************************************************
* Executes one or more Python statements.
******************************************************************************/
int ScriptEngine::executeCommands(const QString& commands, const py::object& global, const QStringList& cmdLineArguments)
{
	return execute([&]() {
		// Pass command line parameters to the script.
		py::list argList;
		argList.append(py::cast("-c"));
		for(const QString& a : cmdLineArguments)
			argList.append(py::cast(a));
		py::module::import("sys").attr("argv") = argList;

		global["__file__"] = py::none();
		PyObject* result = PyRun_String(commands.toUtf8().constData(), Py_file_input, global.ptr(), global.ptr());
		if(!result) throw py::error_already_set();
		Py_XDECREF(result);
	});
}

/******************************************************************************
* Executes a Python program.
******************************************************************************/
int ScriptEngine::executeFile(const QString& filename, const py::object& global, const QStringList& cmdLineArguments)
{
	return execute([&]() {
		// Pass command line parameters to the script.
		py::list argList;
		argList.append(py::cast(filename));
		for(const QString& a : cmdLineArguments)
			argList.append(py::cast(a));
		py::module::import("sys").attr("argv") = argList;

		py::str nativeFilename(py::cast(QDir::toNativeSeparators(filename)));
		global["__file__"] = nativeFilename;
		py::eval_file(nativeFilename, global);
	});
}

/******************************************************************************
* Handles an exception raised by the Python side.
******************************************************************************/
int ScriptEngine::handlePythonException(py::error_already_set& ex, const QString& filename)
{
	ex.restore();

	// Handle calls to sys.exit()
	if(PyErr_ExceptionMatches(PyExc_SystemExit)) {
		return handleSystemExit();
	}

	// Prepare C++ exception object.
	Exception exception(filename.isEmpty() ? 
		tr("The Python script has exited with an error.") :
		tr("The Python script '%1' has exited with an error.").arg(filename), dataset());

	// Retrieve Python error message and traceback.
	if(Application::instance()->guiMode()) {
		PyObject* extype;
		PyObject* value;
		PyObject* traceback;
		PyErr_Fetch(&extype, &value, &traceback);
		PyErr_NormalizeException(&extype, &value, &traceback);
		if(extype) {
			py::object o_extype = py::reinterpret_borrow<py::object>(extype);
			py::object o_value = py::reinterpret_borrow<py::object>(value);
			try {
				if(traceback) {
					py::object o_traceback = py::reinterpret_borrow<py::object>(traceback);
					py::object mod_traceback = py::module::import("traceback");
					bool chain = PyObject_IsInstance(value, extype) == 1;
					py::sequence lines = mod_traceback.attr("format_exception")(o_extype, o_value, o_traceback, py::none(), chain);
					if(py::isinstance<py::sequence>(lines)) {
						QString tracebackString;
						for(int i = 0; i < py::len(lines); ++i)
							tracebackString += lines[i].cast<QString>();
						exception.appendDetailMessage(tracebackString);
					}
				}
				else {
					exception.appendDetailMessage(py::str(o_value).cast<QString>());
				}
			}
			catch(py::error_already_set& ex) {
				ex.restore();
				PyErr_PrintEx(0);
			}
		}
	}
	else {
		// Print error message to the console.
		PyErr_PrintEx(0);
	}

	// Raise C++ exception.
	throw exception;
}

/******************************************************************************
* Handles a call to sys.exit() in the Python interpreter.
* Returns the program exit code.
******************************************************************************/
int ScriptEngine::handleSystemExit()
{
	PyObject *exception, *value, *tb;
	int exitcode = 0;

	PyErr_Fetch(&exception, &value, &tb);

#if PY_MAJOR_VERSION < 3
	if(Py_FlushLine())
		PyErr_Clear();
#endif

	// Interpret sys.exit() argument.
	if(value && value != Py_None) {
#ifdef PyExceptionInstance_Check
		if(PyExceptionInstance_Check(value)) {	// Python 2.6 or newer
#else
		if(PyInstance_Check(value)) {			// Python 2.4
#endif
			// The error code should be in the code attribute.
			PyObject *code = PyObject_GetAttrString(value, "code");
			if(code) {
				Py_DECREF(value);
				value = code;
				if(value == Py_None)
					goto done;
			}
			// If we failed to dig out the 'code' attribute, just let the else clause below print the error.
		}
#if PY_MAJOR_VERSION >= 3
		if(PyLong_Check(value))
			exitcode = (int)PyLong_AsLong(value);
#else
		if(PyInt_Check(value))
			exitcode = (int)PyInt_AsLong(value);
#endif
		else {
			// Send sys.exit() argument to stderr.
			py::str s(value);
	        try {
				auto write = py::module::import("sys").attr("stderr").attr("write");
    	        write(s);
				write("\n");
			} 
			catch(const py::error_already_set&) {}
			exitcode = 1;
		}
	}

done:
	PyErr_Restore(exception, value, tb);
	PyErr_Clear();
	return exitcode;
}

};
