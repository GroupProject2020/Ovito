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

#pragma once


#include <plugins/pyscript/PyScript.h>
#include <core/dataset/DataSet.h>
#include <core/app/Application.h>

namespace PyScript {

using namespace Ovito;
namespace py = pybind11;

/**
 * \brief A scripting engine that provides bindings to OVITO's C++ classes.
 */
class OVITO_PYSCRIPT_EXPORT ScriptEngine : public QObject, public std::enable_shared_from_this<ScriptEngine>
{
private:

	/// \brief Initializes the scripting engine and sets up the environment.
	/// \param dataset The engine will execute scripts in the context of this dataset.
	ScriptEngine(DataSet* dataset);
	
public:

	/// \brief Creates a scripting engine and sets up the scripting environment.
	/// \param dataset The new engine will execute scripts in the context of this dataset.
	static std::shared_ptr<ScriptEngine> createEngine(DataSet* dataset) {
		return std::shared_ptr<ScriptEngine>(new ScriptEngine(dataset));
	}

	/// \brief Creates a global script engine instance. This is used when loading the OVITO Python modules from an external interpreter.
	static void createAdhocEngine(DataSet* dataset);

	/// \brief Destructor
	virtual ~ScriptEngine();

	/// \brief Returns the dataset that provides the context for the script execution.
	DataSet* dataset() const { return _dataset; }

	/// \brief Returns the script engine that is currently active (i.e. which is executing a script).
	/// \return The active script engine or NULL if no script is currently being executed.
	static const std::shared_ptr<ScriptEngine>& activeEngine() { return _activeEngine; }

	/// Returns the context DataSet the current Python script is executed in.
	static DataSet* getCurrentDataset();

	/// \brief Executes a Python script consisting of one or more statements.
	/// \param script The script commands.
	/// \param global The Python globals dictionary to use.
	/// \param scriptArguments An optional list of command line arguments that will be passed to the script via sys.argv.
	/// \return The exit code returned by the Python script.
	/// \throw Exception on error.
	int executeCommands(const QString& commands, const py::object& global, const QStringList& cmdLineArguments = QStringList());

	/// \brief Executes a Python script file.
	/// \param file The script file path.
	/// \param global The Python globals dictionary to use.
	/// \param cmdLineArguments An optional list of command line arguments that will be passed to the script via sys.argv.
	/// \return The exit code returned by the Python script.
	/// \throw Exception on error.
	int executeFile(const QString& file, const py::object& global, const QStringList& cmdLineArguments = QStringList());

	/// \brief Executes the given C++ function, which in turn may invoke Python functions, in the context of this engine.
	int execute(const std::function<void()>& func);

Q_SIGNALS:

	/// This signal is emitted when the Python script writes to the sys.stdout stream.
	void scriptOutput(const QString& outputString);

	/// This is emitted when the Python script writes to the sys.stderr stream.
	void scriptError(const QString& errorString);

private:

	/// Initializes the embedded Python interpreter and sets up the global namespace.
	void initializeEmbeddedInterpreter();

	/// Handles a call to sys.exit() in the Python interpreter.
	/// Returns the program exit code.
	int handleSystemExit();

	/// Handles an exception raised by the Python side.
	int handlePythonException(py::error_already_set& ex, const QString& filename = QString());

	/// This helper class redirects Python script write calls to the sys.stdout stream to this script engine.
	struct InterpreterStdOutputRedirector {
		void write(const QString& str) {
			if(activeEngine()) activeEngine()->scriptOutput(str);
			else std::cout << str.toStdString();
		}
		void flush() {
			if(!activeEngine()) std::cout << std::flush;
		}
	};

	/// This helper class redirects Python script write calls to the sys.stderr stream to this script engine.
	struct InterpreterStdErrorRedirector {
		void write(const QString& str) {
			if(activeEngine()) activeEngine()->scriptError(str);
			else std::cerr << str.toStdString();
		}
		void flush() {
			if(!activeEngine()) std::cerr << std::flush;
		}
	};

	/// The dataset that provides the context for the script execution.
	QPointer<DataSet> _dataset;

	/// The script engine that is currently active (i.e. which is executing a script).
	static std::shared_ptr<ScriptEngine> _activeEngine;

	Q_OBJECT
};

}	// End of namespace
