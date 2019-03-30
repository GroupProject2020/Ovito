///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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

/// Linked list of active script execution contexts.
ScriptEngine::ScriptExecutionContext* ScriptEngine::_activeContext = nullptr;

/// Head of linked list containing all initXXX functions.
PythonPluginRegistration* PythonPluginRegistration::linkedlist = nullptr;

// This helper class redirects Python script write calls to the sys.stdout stream to this script engine.
struct InterpreterOutputRedirector
{
	explicit InterpreterOutputRedirector(std::ostream& stream) : _stream(stream) {}

	void write(const QString& str) {
		for(ScriptEngine::ScriptExecutionContext* c = ScriptEngine::_activeContext; c != nullptr; c = c->next) {
			if(c->stdoutSlot) {
				QMetaObject::invokeMethod(c->contextObj, c->stdoutSlot, Q_ARG(QString, str));
				return;
			}
		}
		_stream << str.toStdString();
	}

	void flush() {
		for(ScriptEngine::ScriptExecutionContext* c = ScriptEngine::_activeContext; c != nullptr; c = c->next) {
			if(c->stdoutSlot)
				return;
		}
		_stream << std::flush;
	}

	std::ostream& _stream;
};

/******************************************************************************
* Initializes the embedded Python interpreter and sets up the global namespace.
******************************************************************************/
void ScriptEngine::initializeEmbeddedInterpreter(RefTarget* contextObj)
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
			py::class_<InterpreterOutputRedirector>(sys_module, "__StdStreamRedirectorHelper")
					.def("write", &InterpreterOutputRedirector::write)
					.def("flush", &InterpreterOutputRedirector::flush);
			// Replace stdout and stderr streams.
			sys_module.attr("stdout") = py::cast(new InterpreterOutputRedirector(std::cout), py::return_value_policy::take_ownership);
			sys_module.attr("stderr") = py::cast(new InterpreterOutputRedirector(std::cerr), py::return_value_policy::take_ownership);
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
	catch(Exception& ex) {
		if(!ex.context()) ex.setContext(contextObj);
		throw;
	}
	catch(py::error_already_set& ex) {
		ex.restore();
		if(PyErr_Occurred())
			PyErr_PrintEx(0);
		contextObj->throwException(DataSet::tr("Failed to initialize Python interpreter. %1").arg(ex.what()));
	}
	catch(const std::exception& ex) {
		contextObj->throwException(DataSet::tr("Failed to initialize Python interpreter: %1").arg(ex.what()));
	}
	catch(...) {
		contextObj->throwException(DataSet::tr("Unhandled exception thrown by Python interpreter."));
	}

	isInterpreterInitialized = true;
}

/******************************************************************************
* Returns the DataSet which is the current context for scripts.
******************************************************************************/
DataSet* ScriptEngine::currentDataset()
{
	OVITO_ASSERT_MSG(_activeContext != nullptr, "ScriptEngine::currentDataset()", "This method may only be called during script execution.");
	OVITO_ASSERT(_activeContext->contextObj != nullptr);
	OVITO_ASSERT(_activeContext->contextObj->dataset());

	if(!_activeContext)
		throw Exception("Invalid program state. ScriptEngine::currentDataset() was called from outside a script execution context.");

	return _activeContext->contextObj->dataset();
}

/******************************************************************************
* Blocks execution until the given future has completed.
******************************************************************************/
bool ScriptEngine::waitForFuture(const FutureBase& future)
{
	OVITO_ASSERT_MSG(_activeContext != nullptr, "ScriptEngine::waitForFuture()", "This method may only be called during script execution.");
	OVITO_ASSERT(_activeContext->task);

	if(!_activeContext)
		throw Exception("Invalid program state. ScriptEngine::waitForFuture() was called from outside a script execution context.");

	bool result = _activeContext->task->waitForFuture(future);
	return result;
}

/******************************************************************************
* Returns the asynchronous task object that represents the current script execution.
******************************************************************************/
const TaskPtr& ScriptEngine::currentTask()
{
	OVITO_ASSERT_MSG(_activeContext != nullptr, "ScriptEngine::currentTask()", "This method may only be called during script execution.");
	OVITO_ASSERT(_activeContext->task);

	if(!_activeContext)
		throw Exception("Invalid program state. ScriptEngine::currentTask() was called from outside a script execution context.");

	return _activeContext->task;
}

/******************************************************************************
* Executes the given C++ function, which in turn may invoke Python functions in the
* context of this engine, and catches possible exceptions.
******************************************************************************/
int ScriptEngine::executeSync(RefTarget* contextObj, const TaskPtr& task, const char* stdoutSlot, const std::function<void()>& func)
{
	OVITO_ASSERT(contextObj != nullptr);
	if(QCoreApplication::instance() && QThread::currentThread() != QCoreApplication::instance()->thread())
		contextObj->throwException(DataSet::tr("Python scripts can only be run from the main thread."));
	DataSet* dataset = contextObj->dataset();

	// Inform the application that a script execution has started.
	// Any OVITO objects created by a script will get initialized to their hard-coded default values.
	bool wasCalledFromScript = (Application::instance()->executionContext() == Application::ExecutionContext::Scripting);
	if(!wasCalledFromScript)
		Application::instance()->switchExecutionContext(Application::ExecutionContext::Scripting);

	// Create an information record on the stack that indicates which script execution is currently in progress.
	ScriptExecutionContext execContext(contextObj, stdoutSlot, task);

	int returnValue = 0;
	try {
		try {
			// Initialize the embedded Python interpreter if it isn't running already.
			if(!Py_IsInitialized())
				initializeEmbeddedInterpreter(contextObj);

			try {
				// Get reference to the main ovito Python module.
				py::module ovito_module = py::module::import("ovito");

				// Add an attribute to the ovito module that provides access to the active dataset.
				py::setattr(ovito_module, "scene", py::cast(dataset, py::return_value_policy::reference));

				// This is for backward compatibility with OVITO 2.9.0:
				py::setattr(ovito_module, "dataset", py::cast(dataset, py::return_value_policy::reference));

				// Invoke the supplied C++ function that executes scripting functions.
				func();
			}
			catch(py::error_already_set& ex) {
				returnValue = handlePythonException(ex);
			}
			catch(Exception& ex) {
				ex.setContext(dataset);
				throw;
			}
			catch(const std::exception& ex) {
				throw Exception(DataSet::tr("Script execution error: %1").arg(ex.what()), dataset);
			}
			catch(...) {
				throw Exception(DataSet::tr("Unhandled exception thrown by Python interpreter."), dataset);
			}
		}
		catch(Exception& ex) {
			if(!ex.context())
				ex.setContext(contextObj->dataset());
			if(stdoutSlot && !task->isCanceled())
				QMetaObject::invokeMethod(contextObj, stdoutSlot, Q_ARG(QString, ex.messages().join(QChar('\n'))));
			throw;
		}
	}
	catch(...) {
		// Inform the application that script execution has ended.
		if(!wasCalledFromScript)
			Application::instance()->switchExecutionContext(Application::ExecutionContext::Interactive);
		throw;
	}

	// Inform the application that script execution has ended.
	if(!wasCalledFromScript)
		Application::instance()->switchExecutionContext(Application::ExecutionContext::Interactive);

	return returnValue;
}

/******************************************************************************
* Executes one or more Python statements.
******************************************************************************/
int ScriptEngine::executeCommands(const QString& commands, RefTarget* contextObj, const TaskPtr& task, const char* stdoutSlot, bool modifyGlobalNamespace, const QStringList& cmdLineArguments)
{
	return executeSync(contextObj, task, stdoutSlot, [&]() {
		// Pass command line parameters to the script.
		py::list argList;
		argList.append(py::cast("-c"));
		for(const QString& a : cmdLineArguments)
			argList.append(py::cast(a));
		py::module::import("sys").attr("argv") = std::move(argList);

		py::dict global;
		if(modifyGlobalNamespace)
			global = py::globals();
		else
			global = py::globals().attr("copy")();

		global["__file__"] = py::none();
		PyObject* result = PyRun_String(commands.toUtf8().constData(), Py_file_input, global.ptr(), global.ptr());
		if(!result) throw py::error_already_set();
		Py_XDECREF(result);
	});
}

/******************************************************************************
* Executes a Python program.
******************************************************************************/
int ScriptEngine::executeFile(const QString& filename, RefTarget* contextObj, const TaskPtr& task, const char* stdoutSlot, bool modifyGlobalNamespace, const QStringList& cmdLineArguments)
{
	return executeSync(contextObj, task, stdoutSlot, [&]() {

		// Pass command line parameters to the script.
		py::list argList;
		argList.append(py::cast(filename));
		for(const QString& a : cmdLineArguments)
			argList.append(py::cast(a));
		py::module::import("sys").attr("argv") = argList;

		py::dict global;
		if(modifyGlobalNamespace)
			global = py::globals();
		else
			global = py::globals().attr("copy")();

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
		DataSet::tr("The Python script has exited with an error.") :
		DataSet::tr("The Python script '%1' has exited with an error.").arg(filename));

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

/******************************************************************************
* Executes the given C++ function in the context of an object and a scripting engine.
******************************************************************************/
Future<> ScriptEngine::executeAsync(RefTarget* context, const char* stdoutSlot, const std::function<py::object()>& func)
{
	OVITO_ASSERT(context);
	OVITO_ASSERT(func);

	// A data structure storing the state needed for a
	// continued execution of the Python generator function.
	struct {
		OvitoObjectExecutor executor;
		std::function<py::object()> func;
		const char* stdoutSlot;
		py::iterator generator;
		Promise<> promise;

		// This is to submit this structure as a work item to the executor:
		void reschedule_execution() {
			executor.createWork(std::move(*this)).post();
		}

		// This is called by the executor at a later time:
		void operator()(bool wasCanceled) {
			if(wasCanceled || promise.isCanceled()) return;

			// Get access to the context object.
			RefTarget* contextObj = static_object_cast<RefTarget>(const_cast<OvitoObject*>(executor.object()));

			// Make sure the actions performed by the script function are not recorded on the undo stack.
			UndoSuspender noUndo(contextObj);

			try {
				ScriptEngine::executeSync(contextObj, promise.task(), stdoutSlot, [this]() {
					if(func) {
						OVITO_ASSERT(!generator);
						// Run caller-provided script execution function.
						py::object functionResult = std::move(func)();
						// Throw away function object after it has been called.
						std::function<py::object()>().swap(func);

						// Check if the function is a generator function and has returned an iterator.
						if(py::isinstance<py::iterator>(functionResult)) {
							generator = py::reinterpret_borrow<py::iterator>(functionResult);
							OVITO_ASSERT(generator);
						}
						else {
							// We are done.
							promise.setFinished();
						}
					}
					else {
						OVITO_ASSERT(!func);
						QTime time;
						time.start();
						do {
							OVITO_ASSERT(generator != py::iterator::sentinel());

							// The generator may report progress.
							py::handle value = *generator;
							if(py::isinstance<py::float_>(value)) {
								double progressValue = py::cast<double>(value);
								if(progressValue >= 0.0 && progressValue <= 1.0) {
									promise.setProgressMaximum(100);
									promise.setProgressValue((qlonglong)(progressValue * 100.0));
								}
								else {
									promise.setProgressMaximum(0);
									promise.setProgressValue(0);
								}
							}
							else if(py::isinstance<py::str>(value)) {
								promise.setProgressText(py::cast<QString>(value));
							}

							// Let the Python function perform some work.
							++generator;
							// Check if the generator is exhausted.
							if(generator == py::iterator::sentinel()) {
								// We are done.
								promise.setFinished();
								break;
							}
							// Keep calling the generator object for
							// 20 milliseconds or until it becomes exhausted.
						}
						while(time.elapsed() < 20 && !promise.isCanceled());
					}
				});
			}
			catch(...) {
				promise.captureException();
				promise.setFinished();
			}

			// Continue execution at a later time.
			if(!promise.isFinished())
				reschedule_execution();
		}
	}
	func_continuation{ context->executor(), func, stdoutSlot };

	// Python has returned a generator. We have to return a Future on the
	// the final pipeline state that is still to be computed.
	func_continuation.promise = context->dataset()->taskManager().createMainThreadOperation<>(true);
	Future<> future = func_continuation.promise.future();
	func_continuation.promise.setProgressText(DataSet::tr("Script execution"));

	// Schedule an execution continuation of the Python function at some later time.
	func_continuation.reschedule_execution();

	return future;
}

/******************************************************************************
* This is called to set up an ad-hoc environment when the Ovito Python module is loaded from
* an external Python interpreter.
******************************************************************************/
void ScriptEngine::initializeExternalInterpreter(DataSet* dataset)
{
	OVITO_ASSERT(Py_IsInitialized());
	OVITO_ASSERT(dataset);

	// Inform the application that script execution is in progress (for an indefinite period).
	// Any OVITO objects created by a script will get initialized to their hard-coded default values.
	Application::instance()->switchExecutionContext(Application::ExecutionContext::Scripting);

	// Create script execution context and make it permanently active.
	static AsyncOperation scriptOperation(dataset->taskManager());
	static ScriptExecutionContext execContext(dataset, nullptr, scriptOperation.task());
}

}
