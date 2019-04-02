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

#pragma once


#include <plugins/pyscript/PyScript.h>

namespace PyScript {

using namespace Ovito;
namespace py = pybind11;

/**
 * \brief A static class that provides functions for executing Python scripts and commands.
 */
class OVITO_PYSCRIPT_EXPORT ScriptEngine
{
public:

	/// \brief Executes a Python script consisting of one or more statements.
	/// \param script The Python statements to execute.
	/// \param contextObj An object (e.g. a DataSet) that serves as context of the script execution.
	/// \param task The asynchronous task object that serves as context of the script execution.
	/// \param stdoutSlot The optional name of the Qt slot of the context object that is called whenever the script produces console output.
	/// \param modifyGlobalNamespace Controls whether the script affects the Python global namespace. If false, the script will be executed in a private copy of the global namespace.
	/// \param scriptArguments An optional list of command line arguments that will be passed to the script via sys.argv.
	/// \return The exit code returned by the Python script.
	/// \throw Exception on error.
	static int executeCommands(const QString& commands, RefTarget* contextObj, const TaskPtr& task, const char* stdoutSlot, bool modifyGlobalNamespace, const QStringList& cmdLineArguments = QStringList());

	/// \brief Executes a Python script file.
	/// \param file The script file path.
	/// \param contextObj An object (e.g. a DataSet) that serves as context of the script execution.
	/// \param task The asynchronous task object that serves as context of the script execution.
	/// \param stdoutSlot The optional name of the Qt slot of the context object that is called whenever the script produces console output.
	/// \param modifyGlobalNamespace Controls whether the script affects the Python global namespace. If false, the script will be executed in a private copy of the global namespace.
	/// \param cmdLineArguments An optional list of command line arguments that will be passed to the script via sys.argv.
	/// \return The exit code returned by the Python script.
	/// \throw Exception on error.
	static int executeFile(const QString& file, RefTarget* contextObj, const TaskPtr& task, const char* stdoutSlot, bool modifyGlobalNamespace, const QStringList& cmdLineArguments = QStringList());

	/// \brief Executes the given C++ function, which in turn may invoke Python code, in the context of this engine.
	/// \param contextObj An object (e.g. a DataSet) that serves as context of the script execution.
	/// \param task The asynchronous task object that serves as context of the script execution.
	/// \param stdoutSlot The optional name of the Qt slot of the context object that is called whenever the script produces console output.
	static int executeSync(RefTarget* contextObj, const TaskPtr& task, const char* stdoutSlot, const std::function<void()>& func);

	/// \brief Executes the given C++ function in the context of an object and a scripting engine.
	/// \param contextObj An object (e.g. a DataSet) that serves as context of the script execution.
	/// \param stdoutSlot The optional name of the Qt slot of the context object that is called whenever the script produces console output.
	static Future<> executeAsync(RefTarget* context, const char* stdoutSlot, const std::function<py::object()>& func);

	/// \brief Blocks execution until the given future has completed.
	/// \note This function may only be called from within a script execution context. Typically it is used by Python binding layer.
	/// \return False if the operation has been canceled by the user.
	static bool waitForFuture(const FutureBase& future);

	/// \brief Returns the DataSet which is the current context for scripts.
	/// \note This function may only be called from within a script execution context. Typically it is used by Python binding layer.
	static DataSet* currentDataset();

	/// \brief Returns the asynchronous task object that represents the current script execution.
	/// \note This function may only be called from within a script execution context. Typically it is used by Python binding layer.
	static const TaskPtr& currentTask();

	/// \brief This is called to set up an ad-hoc environment when the Ovito Python module is loaded from
	///        an external Python interpreter.
	static void initializeExternalInterpreter(DataSet* dataset);

private:

	/// Initializes the embedded Python interpreter and sets up the global namespace.
	static void initializeEmbeddedInterpreter(RefTarget* contextObj);

	/// Handles a call to sys.exit() in the Python interpreter.
	/// Returns the program exit code.
	static int handleSystemExit();

	/// Handles an exception raised by the Python side.
	static int handlePythonException(py::error_already_set& ex, const QString& filename = QString());

	/// Information record that represents a script execution that is currently in progress.
	struct ScriptExecutionContext {

		ScriptExecutionContext(ScriptExecutionContext&&) = delete;
		ScriptExecutionContext(RefTarget* c, const char* s, TaskPtr t) : contextObj(c), stdoutSlot(s), task(std::move(t)) {
			// Insert myself into linked list of objects.
			next = _activeContext;
			_activeContext = this;
		}
		~ScriptExecutionContext() {
			// Remove myself from linked list of objects.
			OVITO_ASSERT(_activeContext == this);
			_activeContext = next;
		}

		RefTarget* contextObj;
		const char* stdoutSlot;
		TaskPtr task;
		ScriptExecutionContext* next;
	};

	/// Stack of currently active script execution contexts.
	static ScriptExecutionContext* _activeContext;

	friend struct InterpreterOutputRedirector;
};

}	// End of namespace
