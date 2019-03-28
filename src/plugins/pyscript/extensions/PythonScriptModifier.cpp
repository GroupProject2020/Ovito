///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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

	// Throw away compiled script function whenever script source code changes.
	if(field == PROPERTY_FIELD(script)) {
		_modifyScriptFunction = py::object();
	}
}

/******************************************************************************
* Compiles the script entered by the user.
******************************************************************************/
void PythonScriptModifier::compileScript(ScriptEngine& engine)
{
	_modifyScriptFunction = py::function();
	_scriptCompilationOutput.clear();
	try {
		try {
			// Make sure the actions of the script function are not recorded on the undo stack.
			UndoSuspender noUndo(dataset());

			// Run script code within a fresh and private namespace.
			py::dict localNamespace = py::globals().attr("copy")();
			engine.executeCommands(script(), localNamespace);

			// Extract the modify() function defined by the script.
			engine.execute([&]() {
				try {
					_modifyScriptFunction = py::function(localNamespace["modify"]);
					if(!py::isinstance<py::function>(_modifyScriptFunction)) {
						_modifyScriptFunction = py::function();
						throwException(tr("Invalid Python script. It does not define a callable function named modify()."));
					}
				}
				catch(const py::error_already_set&) {
					throwException(tr("Invalid Python script. It does not define the function named modify()."));
				}
			});

			// Update status, because log output stored by the modifier has changed.
			notifyDependents(ReferenceEvent::ObjectStatusChanged);
		}
		catch(const Exception& ex) {
			_scriptCompilationOutput += ex.messages().join(QChar('\n'));
			throw;
		}
	}
	catch(...) {
		// Update status, because log output stored by the modifier has changed.
		notifyDependents(ReferenceEvent::ObjectStatusChanged);
		throw;
	}
}

/******************************************************************************
* Prepares the script engine, which is needed for script execution.
******************************************************************************/
ScriptEngine* PythonScriptModifier::getScriptEngine()
{
	// Initialize a private script engine if there is no active global engine.
	const std::shared_ptr<ScriptEngine>& engine = ScriptEngine::activeEngine();
	if(!engine) {
		if(!_scriptEngine) {
			_scriptEngine = ScriptEngine::createEngine(dataset());
			connect(_scriptEngine.get(), &ScriptEngine::scriptOutput, this, &PythonScriptModifier::onScriptOutput);
			connect(_scriptEngine.get(), &ScriptEngine::scriptError, this, &PythonScriptModifier::onScriptOutput);
		}
		return _scriptEngine.get();
	}
	else {
		if(!_scriptEngine) _scriptEngine = engine;
		return engine.get();
	}
}

/******************************************************************************
* This modifies the input data.
******************************************************************************/
Future<PipelineFlowState> PythonScriptModifier::evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Make sure this modifier evaluation has not been triggered from within an ongoing pipeline evaluation.
	OVITO_ASSERT(!_activeModApp);
	if(_activeModApp)
		throwException(tr("Python script modifier is not reentrant. It cannot be evaluated while another evaluation is already in progress."));

	// Initialize the script engine if there is no active global engine.
	ScriptEngine* engine = getScriptEngine();

	// We now enter the modifier evaluation phase.
	_activeModApp = dynamic_object_cast<PythonScriptModifierApplication>(modApp);
	if(!_activeModApp)
		throwException(tr("PythonScriptModifier instance is not associated with a PythonScriptModifierApplication instance."));

	if(input.isEmpty())
		throwException(tr("Modifier input is empty."));

	try {

		// Reset script log output.
		_activeModApp->clearLogOutput();

		// Compile script source if needed.
		if(!_modifyScriptFunction) {
			compileScript(*engine);
		}
		else {
			_scriptCompilationOutput.clear();
		}

		// Check if script function has been set.
		if(!_modifyScriptFunction)
			throwException(tr("PythonScriptModifier has not been assigned a Python function."));

		try {
			// Make sure the actions of the script function are not recorded on the undo stack.
			UndoSuspender noUndo(dataset());

			// Prepare arguments to be passed to the script function.
			PipelineFlowState output = input;

			// Limit validity interval of the output to the current frame,
			// because we don't know if the user script produces time-dependent results or not.
			output.intersectStateValidity(time);

			// Call the user-defined modifier function.
			py::object functionResult;
			try {
				engine->execute([&]() {
					int animationFrame = dataset()->animationSettings()->timeToFrame(time);
					py::tuple arguments = py::make_tuple(animationFrame, output.mutableData());
					functionResult = _modifyScriptFunction(*arguments);
				});
			}
			catch(const Exception& ex) {

				// The following code is for backward compatibility with OVITO 2.9.0:
				//
				// Retry to call the user-defined modifier function with a separate input and an output data collection.
				// but first check if the function has the expected signature.
				py::object inspect_module = py::module::import("inspect");
#if PY_MAJOR_VERSION >= 3
				py::object argsSpec = inspect_module.attr("getfullargspec")(_modifyScriptFunction);
#else
				py::object argsSpec = inspect_module.attr("getargspec")(_modifyScriptFunction);
#endif
				if(py::len(argsSpec.attr("args")) != 3)
					throw;

				// Invoke the user-defined modifier function, this time with an input and an output data collection.
				engine->execute([&]() {
					int animationFrame = dataset()->animationSettings()->timeToFrame(time);
					py::tuple arguments = py::make_tuple(animationFrame, input.data(), output.mutableData());
					functionResult = _modifyScriptFunction(*arguments);
				});
			}

			// Exit the modifier evaluation phase.
			_activeModApp = nullptr;
			notifyDependents(ReferenceEvent::ObjectStatusChanged);
			modApp->notifyDependents(ReferenceEvent::ObjectStatusChanged);

			// Check if the function is a generator function.
			if(py::isinstance<py::iterator>(functionResult)) {

				// A data structure storing the information needed for a
				// continued execution of the Python generator function.
				struct {
					OvitoObjectExecutor executor;
					py::iterator generator;
					PipelineFlowState output;
					Promise<PipelineFlowState> promise;

					// This is to submit this structure as a work item to the executor:
					void reschedule_execution() {
						executor.createWork(std::move(*this)).post();
					}

					// This is called by the executor at a later time:
					void operator()(bool wasCanceled) {
						if(wasCanceled || promise.isCanceled()) return;

						// Get access to the modifier and its modifier application.
						PythonScriptModifierApplication* modApp = static_object_cast<PythonScriptModifierApplication>(const_cast<OvitoObject*>(executor.object()));
						PythonScriptModifier* modifier = dynamic_object_cast<PythonScriptModifier>(modApp->modifier());
						if(!modifier) return;

						// Enter the modifier evaluation phase.
						modifier->_activeModApp = modApp;
						// Make sure the actions of the script function are not recorded on the undo stack.
						UndoSuspender noUndo(modifier);

						// Get the script engine to use.
						ScriptEngine* engine = modifier->getScriptEngine();

						try {
							try {
								engine->execute([this]() {
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
											// We are done. Return pipeline results.
											promise.setResults(std::move(output));
											promise.setFinished();
											break;
										}
										// Keep calling the generator object for
										// 20 milliseconds or until it becomes exhausted.
									}
									while(time.elapsed() < 20 && !promise.isCanceled());
								});
							}
							catch(const Exception& ex) {
								modApp->appendLogOutput(ex.messages().join(QChar('\n')));
								throw;
							}
						}
						catch(...) {
							promise.captureException();
							promise.setFinished();
						}

						// Exit the modifier evaluation phase.
						modifier->_activeModApp = nullptr;
						modifier->notifyDependents(ReferenceEvent::ObjectStatusChanged);
						modApp->notifyDependents(ReferenceEvent::ObjectStatusChanged);

						// Continue execution at a later time.
						if(!promise.isFinished())
							reschedule_execution();
					}
				} func_continuation{ modApp->executor() };

				func_continuation.output = std::move(output);
				func_continuation.generator = py::reinterpret_borrow<py::iterator>(functionResult);
				OVITO_ASSERT(func_continuation.generator);

				// Python has returned a generator. We have to return a Future on the
				// the final pipeline state that is still to be computed.
				func_continuation.promise = dataset()->taskManager().createMainThreadOperation<PipelineFlowState>(true);
				Future<PipelineFlowState> future = func_continuation.promise.future();
				func_continuation.promise.setProgressText(tr("Executing user-defined modifier function"));

				// Schedule an execution continuation of the Python function at a later time.
				func_continuation.reschedule_execution();

				return future;
			}
			else {
				// Return final output pipeline state.
				return std::move(output);
			}
		}
		catch(const std::runtime_error& ex) {
			qWarning() << ex.what();
			throwException(tr("Internal Python interface error: %1").arg(ex.what()));
		}
		catch(const Exception& ex) {
			_activeModApp->appendLogOutput(ex.messages().join(QChar('\n')));
			throw;
		}
	}
	catch(...) {
		// Exit the modifier evaluation phase.
		_activeModApp = nullptr;
		notifyDependents(ReferenceEvent::ObjectStatusChanged);
		modApp->notifyDependents(ReferenceEvent::ObjectStatusChanged);
		throw;
	}
}

/******************************************************************************
* Is called when the script generates some output.
******************************************************************************/
void PythonScriptModifier::onScriptOutput(const QString& text)
{
	if(!_activeModApp) {
		// This is to collect script output during the compilation phase.
		_scriptCompilationOutput += text;
	}
	else {
		// This is to collect script output during the modifier evaluation phase.
		_activeModApp->appendLogOutput(text);
	}
}

}	// End of namespace
