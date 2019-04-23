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
#include <core/dataset/animation/AnimationSettings.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/UndoStack.h>
#include <core/utilities/concurrent/TaskManager.h>
#include <core/utilities/concurrent/Promise.h>
#include "PythonScriptModifier.h"

namespace PyScript {

IMPLEMENT_OVITO_CLASS(PythonScriptModifier);
IMPLEMENT_OVITO_CLASS(PythonScriptModifierApplication);
DEFINE_PROPERTY_FIELD(PythonScriptModifier, script);
SET_PROPERTY_FIELD_LABEL(PythonScriptModifier, script, "script");
SET_MODIFIER_APPLICATION_TYPE(PythonScriptModifier, PythonScriptModifierApplication);

/******************************************************************************
* Constructor.
******************************************************************************/
PythonScriptModifier::PythonScriptModifier(DataSet* dataset) : Modifier(dataset)
{
}

/******************************************************************************
* Loads the default values of this object's parameter fields.
******************************************************************************/
void PythonScriptModifier::loadUserDefaults()
{
	Modifier::loadUserDefaults();

	// Load example script.
	setScript("from ovito.data import *\n\n"
			"def modify(frame, data):\n"
			"    \n"
			"    # This user-defined modifier function gets automatically called by OVITO whenever the data pipeline is newly computed.\n"
			"    # It receives two arguments from the pipeline system:\n"
			"    # \n"
			"    #    frame - The current animation frame number at which the pipeline is being evaluated.\n"
			"    #    data   - The DataCollection passed in from the pipeline system. \n"
			"    #                The function may modify the data stored in this DataCollection as needed.\n"
			"    # \n"
			"    # What follows is an example code snippet doing nothing except printing the current \n"
			"    # list of particle properties to the log window. Use it as a starting point for developing \n"
			"    # your own data modification or analysis functions. \n"
			"    \n"
			"    if data.particles != None:\n"
			"        print(\"There are %i particles with the following properties:\" % data.particles.count)\n"
			"        for property_name in data.particles.keys():\n"
			"            print(\"  '%s'\" % property_name)\n");
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void PythonScriptModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	Modifier::propertyChanged(field);

	// Throw away compiled script function whenever script text changes.
	if(field == PROPERTY_FIELD(script)) {
		_scriptCompilationFuture.reset();
		_scriptCompilationOutput = tr("<Script compilation pending>\n");
	}
}

/******************************************************************************
* Compiles the script entered by the user.
******************************************************************************/
SharedFuture<py::function> PythonScriptModifier::compileScriptFunction()
{
	// Use the existing future object if the compilation is currently in progress or already completed.
	if(!_scriptCompilationFuture.isValid()) {

		_scriptCompilationOutput.clear();
		auto scriptFunction = std::make_shared<py::function>();

		// Run the following code within the context of a script engine.
		Future<> execFuture = ScriptEngine::executeAsync(this, "appendCompilationOutput", [this, scriptFunction]() {

			// Run script code within a fresh and private namespace.
			py::dict localNamespace = py::globals().attr("copy")();

			// Run the script code once.
			localNamespace["__file__"] = py::none();
			PyObject* result = PyRun_String(script().toUtf8().constData(), Py_file_input, localNamespace.ptr(), localNamespace.ptr());
			if(!result) throw py::error_already_set();
			Py_XDECREF(result); // We have no interest in the result object.

			// Extract the modify() function defined within the script.
			try {
				*scriptFunction = py::function(localNamespace["modify"]);
				if(!py::isinstance<py::function>(*scriptFunction)) {
					throw Exception(tr("Invalid Python modifier script. It does not define a callable function named modify()."));
				}
			}
			catch(const py::error_already_set&) {
				throw Exception(tr("Invalid Python modifier script. There is no function with the name 'modify()'."));
			}

			return py::none();
		});

		// Update modifier status after compilation, to display Python log output.
		execFuture.finally(executor(), [this]() {
			notifyDependents(ReferenceEvent::ObjectStatusChanged);
		});

		// Make sure the Python function gets returned to the caller.
		_scriptCompilationFuture = execFuture.then([scriptFunction]() {
			return std::move(*scriptFunction);
		});
	}
	return _scriptCompilationFuture;
}

/******************************************************************************
* This modifies the input data.
******************************************************************************/
Future<PipelineFlowState> PythonScriptModifier::evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	if(input.isEmpty())
		throwException(tr("Modifier input is empty."));

	// We now enter the modifier evaluation phase.
	PythonScriptModifierApplication* pmodApp = dynamic_object_cast<PythonScriptModifierApplication>(modApp);
	if(!pmodApp)
		throwException(tr("PythonScriptModifier instance is not associated with a PythonScriptModifierApplication instance."));

	// Reset script log output.
	pmodApp->clearLogOutput();

	// First compile the script function.
	SharedFuture<py::function> scriptFunctionFuture = compileScriptFunction();

	// Prepare the pipeline output state.
	std::shared_ptr<PipelineFlowState> output = std::make_shared<PipelineFlowState>(input);

	// Limit validity interval of the pipeline output state to the current frame by default,
	// because we don't know if the user script produces time-dependent results or not.
	output->intersectStateValidity(time);

	// Now that a compiled script function is available, execute it.
	return scriptFunctionFuture.then(pmodApp->executor(), [output = std::move(output), time, pmodApp](const py::function& scriptFunction) mutable {

		// Run the following code within the context of a script engine.
		Future<> execFuture = ScriptEngine::executeAsync(pmodApp, "appendLogOutput", [pmodApp, time, output, scriptFunction]() {

			// Call the user-defined modifier function.
			try {
				int animationFrame = pmodApp->dataset()->animationSettings()->timeToFrame(time);
				py::tuple arguments = py::make_tuple(animationFrame, output->mutableData());
				return scriptFunction(*arguments);
			}
			catch(const py::error_already_set& ex) {
				if(!ex.matches(PyExc_TypeError))
					throw;

				// The following code is for backward compatibility with OVITO 2.9.0:
				//
				// Retry to call the user-defined modifier function with a separate input and an output data collection.
				// but first check if the function has the expected signature.
				py::object inspect_module = py::module::import("inspect");
#if PY_MAJOR_VERSION >= 3
				py::object argsSpec = inspect_module.attr("getfullargspec")(scriptFunction);
#else
				py::object argsSpec = inspect_module.attr("getargspec")(scriptFunction);
#endif
				if(py::len(argsSpec.attr("args")) != 3)
					throw;

				// Invoke the user-defined modifier function, this time with an input and an output data collection.
				int animationFrame = pmodApp->dataset()->animationSettings()->timeToFrame(time);
				PipelineFlowState input = *output;
				py::tuple arguments = py::make_tuple(animationFrame, input.data(), output->mutableData());
				return scriptFunction(*arguments);
			}
		});

		// Make sure the pipeline flow state is returned to the caller.
		return execFuture.then([output]() {
			return std::move(*output);
		});
	});
}

/******************************************************************************
* Is called whenever the script generates some output during the compilation phase.
******************************************************************************/
void PythonScriptModifier::appendCompilationOutput(const QString& text)
{
	// This is to collect script output during the compilation phase.
	_scriptCompilationOutput += text;
	notifyDependents(ReferenceEvent::ObjectStatusChanged);
}

/******************************************************************************
* Is called whenever the script generates some output during the compilation phase.
******************************************************************************/
void PythonScriptModifierApplication::appendLogOutput(const QString& text)
{
	// This is to collect script output during the evaluation phase.
	_scriptLogOutput += text;
	notifyDependents(ReferenceEvent::ObjectStatusChanged);
	modifier()->notifyDependents(ReferenceEvent::ObjectStatusChanged);
}

}	// End of namespace
