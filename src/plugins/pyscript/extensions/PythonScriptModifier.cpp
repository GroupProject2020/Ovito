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
			"def modify(frame, input, output):\n"
			"\tprint(\"Input particle properties:\")\n"
			"\tfor name in input.particle_properties.keys():\n"
			"\t\tprint(name)\n");
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
	// Reset Python environment when using a private script engine.
	if(_mainNamespacePrototype)
		engine.mainNamespace() = _mainNamespacePrototype.attr("copy")();

	_modifyScriptFunction = py::function();
	_scriptCompilationOutput.clear();
	try {
		try {
			// Make sure the actions of the script function are not recorded on the undo stack.
			UndoSuspender noUndo(dataset());
			
			// Run script once.
			engine.executeCommands(script());
			// Extract the modify() function defined by the script.
			engine.execute([this,&engine]() {
				try {
					_modifyScriptFunction = py::function(engine.mainNamespace()["modify"]);
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
			_scriptEngine = std::make_shared<ScriptEngine>(dataset(), dataset()->container()->taskManager(), true);
			connect(_scriptEngine.get(), &ScriptEngine::scriptOutput, this, &PythonScriptModifier::onScriptOutput);
			connect(_scriptEngine.get(), &ScriptEngine::scriptError, this, &PythonScriptModifier::onScriptOutput);
			_mainNamespacePrototype = _scriptEngine->mainNamespace();
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
	// Make sure the pipeline evaluation is not treiggered from within an
	// ongoing pipeline evaluation.
	OVITO_ASSERT(!_activeModApp);
	if(_activeModApp)
		throwException(tr("Python script modifier is not reentrant. It cannot be evaluated while another evaluation is already in progress."));

	// Initialize the script engine if there is no active global engine.
	ScriptEngine* engine = getScriptEngine();

	// We now enter the modifier evaluation phase.
	_activeModApp = dynamic_object_cast<PythonScriptModifierApplication>(modApp);
	if(!_activeModApp)
		throwException(tr("PythonScriptModifier instance is not associated with a PythonScriptModifierApplication instance."));

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
			throwException(tr("Modifier function of PythonScriptModifier instance has not been set."));
		
		try {
			// Get animation frame at which the modifier is evaluated.
			int animationFrame = dataset()->animationSettings()->timeToFrame(time);

			// Make sure the actions of the script function are not recorded on the undo stack.
			UndoSuspender noUndo(dataset());

			// Prepare arguments to be passed to the script function.
			py::object input_py = py::cast(input, py::return_value_policy::copy);
			py::object output_py = py::cast(input, py::return_value_policy::copy);
			py::tuple arguments = py::make_tuple(animationFrame, std::move(input_py), output_py);

			// Limit validity interval of the output to the current frame,
			// because we don't know if the user script produces time-dependent results.
			py::cast<PipelineFlowState&>(output_py).intersectStateValidity(time);				

			// Call the user-defined Python function.
			py::object functionResult = engine->callObject(_modifyScriptFunction, arguments);
			
			// Exit the modifier evaluation phase.
			_activeModApp = nullptr;
			modApp->notifyDependents(ReferenceEvent::ObjectStatusChanged);				
			 
			// Check if the function is a generator function.
			if(py::isinstance<py::iterator>(functionResult)) {
				
				// A data structure storing the information needed for a
				// continued execution of the Python generator function.
				struct {
					OvitoObjectExecutor executor;
					py::iterator generator;
					py::object output_py;
					Promise<PipelineFlowState> promise;
					
					// This is to submit this structure as a work item to the executor:
					void reschedule_execution() {
						executor.createWork(std::move(*this)).post();
					}

					// This is called by the executor at a later time:
					void operator()(bool wasCanceled) {
						if(wasCanceled || promise.isCanceled()) return;
						
						// Get access to the modifier and its modifier application.
						PythonScriptModifierApplication* modApp = static_object_cast<PythonScriptModifierApplication>(executor.object());
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
												promise.setProgressValue((int)(progressValue * 100.0));
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
											promise.setResults(py::cast<PipelineFlowState>(std::move(output_py)));
											promise.setFinished();
											break;
										}
										// Keep calling the generator object until
										// 20 milliseconds have passed or until it is exhausted.
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
						modApp->notifyDependents(ReferenceEvent::ObjectStatusChanged);
						
						// Continue execution at a later time.
						if(!promise.isFinished())
							reschedule_execution();
					}
				} func_continuation{ modApp->executor() };

				func_continuation.output_py = std::move(output_py);
				func_continuation.generator = py::reinterpret_borrow<py::iterator>(functionResult);
				OVITO_ASSERT(func_continuation.generator);
				
				// Python has returned a generator. We have to return a Future on the 
				// the final pipeline state that is till to be computed.
				func_continuation.promise = Promise<PipelineFlowState>::createSynchronous(nullptr, true, false);\
				dataset()->container()->taskManager().registerTask(func_continuation.promise.sharedState());
				Future<PipelineFlowState> future = func_continuation.promise.future();
				func_continuation.promise.setProgressText(tr("Executing user-defined modifier function"));				

				// Schedule an execution continuation of the Python function at a later time.
				func_continuation.reschedule_execution();
				
				return future;
			}
			else {
				// Extract final output pipeline state.
				return py::cast<PipelineFlowState>(std::move(output_py));
			}
		}
		catch(const Exception& ex) {
			_activeModApp->appendLogOutput(ex.messages().join(QChar('\n')));
			throw;
		}
	}
	catch(...) {
		// Exit the modifier evaluation phase.
		_activeModApp = nullptr;
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
