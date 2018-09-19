///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2018) Alexander Stukowski
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
#include <core/viewport/Viewport.h>
#include <core/rendering/RenderSettings.h>
#include <core/dataset/DataSetContainer.h>
#include <core/app/Application.h>
#include "PythonViewportOverlay.h"

namespace PyScript {

IMPLEMENT_OVITO_CLASS(PythonViewportOverlay);
DEFINE_PROPERTY_FIELD(PythonViewportOverlay, script);
SET_PROPERTY_FIELD_LABEL(PythonViewportOverlay, script, "script");

/******************************************************************************
* Constructor.
******************************************************************************/
PythonViewportOverlay::PythonViewportOverlay(DataSet* dataset) : ViewportOverlay(dataset)
{
}

/******************************************************************************
* Loads the default values of this object's parameter fields.
******************************************************************************/
void PythonViewportOverlay::loadUserDefaults()
{
	ViewportOverlay::loadUserDefaults();

	// Load default demo script.
	setScript(
			"# This user-defined function is called by OVITO to let it draw arbitrary graphics on top of the viewport.\n"
			"def render(args):\n"
			"    \n"
			"    # This demo code prints the current animation frame into the upper left corner of the viewport.\n"
			"    text1 = \"Frame {}\".format(args.frame)\n"
			"    args.painter.drawText(10, 10 + args.painter.fontMetrics().ascent(), text1)\n"
			"    \n"
			"    # Also print the current number of particles into the lower left corner of the viewport.\n"
			"    pipeline = args.scene.selected_pipeline\n"
			"    if pipeline:\n"
			"        data = pipeline.compute()\n"
			"        num_particles = data.particles.count\n"
			"        text2 = \"{} particles\".format(num_particles)\n"
			"        args.painter.drawText(10, args.painter.window().height() - 10, text2)\n");	
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void PythonViewportOverlay::propertyChanged(const PropertyFieldDescriptor& field)
{
	ViewportOverlay::propertyChanged(field);

	// Throw away compiled script function whenever script source code changes and recompile.
	if(field == PROPERTY_FIELD(script)) {
		_overlayScriptFunction = py::object();
		compileScript();
	}
}

/******************************************************************************
* Prepares the script engine, which is needed for script execution.
******************************************************************************/
ScriptEngine* PythonViewportOverlay::getScriptEngine()
{
	// Initialize a private script engine if there is no active global engine.
	const std::shared_ptr<ScriptEngine>& engine = ScriptEngine::activeEngine();
	if(!engine) {
		if(!_scriptEngine) {
			_scriptEngine = ScriptEngine::createEngine(dataset());
			connect(_scriptEngine.get(), &ScriptEngine::scriptOutput, this, &PythonViewportOverlay::onScriptOutput);
			connect(_scriptEngine.get(), &ScriptEngine::scriptError, this, &PythonViewportOverlay::onScriptOutput);
		}
		return _scriptEngine.get();
	}
	else {
		if(!_scriptEngine) _scriptEngine = engine;
		return engine.get();
	}
}

/******************************************************************************
* Compiles the script entered by the user.
******************************************************************************/
void PythonViewportOverlay::compileScript()
{
	// Cannot execute scripts during file loading.
	if(isBeingLoaded()) return;

	_overlayScriptFunction = py::function();
	_scriptCompilationOutput.clear();
	_scriptRenderingOutput.clear();
	try {
		// Make sure the actions of the script function are not recorded on the undo stack.
		UndoSuspender noUndo(dataset());
			
		// Initialize a local script engine.
		ScriptEngine* engine = getScriptEngine();

		// Run script code within a private namespace.
		py::dict localNamespace = py::globals().attr("copy")();
		engine->executeCommands(script(), localNamespace);

		// Extract the render() function defined by the script.
		engine->execute([&]() {
			try {
				_overlayScriptFunction = py::function(localNamespace["render"]);
				if(!py::isinstance<py::function>(_overlayScriptFunction)) {
					_overlayScriptFunction = py::function();
					throwException(tr("Invalid Python script. It does not define a callable function named render()."));
				}
			}
			catch(const py::error_already_set&) {
				throwException(tr("Invalid Python script. It does not define the function named render()."));
			}
		});
	}
	catch(const Exception& ex) {
		_scriptCompilationOutput += ex.messages().join(QChar('\n'));
	}

	// Update status, because log output has changed.
	notifyDependents(ReferenceEvent::ObjectStatusChanged);
}

/******************************************************************************
* Is called when the script generates some output.
******************************************************************************/
void PythonViewportOverlay::onScriptOutput(const QString& text)
{
	if(!_overlayScriptFunction)
		_scriptCompilationOutput += text;
	else
		_scriptRenderingOutput += text;
}

/******************************************************************************
* This method paints the overlay contents on the given canvas.
******************************************************************************/
void PythonViewportOverlay::renderImplementation(const Viewport* viewport, TimePoint time, QPainter& painter, 
							const ViewProjectionParameters& projParams, const RenderSettings* renderSettings)
{
	// Compile script source if needed.
	if(!_overlayScriptFunction) {
		compileScript();
	}

	// Check if executable script function is available.
	if(!_overlayScriptFunction)
		return;

	// This object instance may be deleted while this method is being executed.
	// This is to detect this situation.
	QPointer<PythonViewportOverlay> thisPointer(this);
	
	_scriptRenderingOutput.clear();
	try {
		// Make sure the actions of the script function are not recorded on the undo stack.
		UndoSuspender noUndo(dataset());
			
		// Enable antialiasing for the QPainter by default.
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::TextAntialiasing);

		// Get a local script engine.
		ScriptEngine* engine = getScriptEngine();
		engine->execute([this,engine,viewport,&painter,&projParams,renderSettings,time]() {

			// Pass viewport, QPainter, and other information to the Python script function.
			// The QPainter pointer has to be converted to the representation used by PyQt5.
			py::module numpy_module = py::module::import("numpy");
			py::module sip_module = py::module::import("sip");
			py::module qtgui_module = py::module::import("PyQt5.QtGui");
			py::object painter_ptr = py::cast(reinterpret_cast<std::uintptr_t>(&painter));
			py::object qpainter_class = qtgui_module.attr("QPainter");
			py::object sip_painter = sip_module.attr("wrapinstance")(painter_ptr, qpainter_class);

			py::tuple arguments = py::make_tuple(py::cast(
				ViewportOverlayArguments(time, viewport, projParams, renderSettings, std::move(sip_painter), painter), 
				py::return_value_policy::move));

			// Execute render() script function.
			engine->execute([&]() {
				_overlayScriptFunction(*std::move(arguments));
			});
		});
	}
	catch(const Exception& ex) {
		_scriptRenderingOutput +=  ex.messages().join(QChar('\n'));
		// Interrupt rendering process in console mode.
		if(Application::instance()->consoleMode())
			throw;
	}

	if(thisPointer)
		thisPointer->notifyDependents(ReferenceEvent::ObjectStatusChanged);
}

/******************************************************************************
* Projects a point from world space to window space.
******************************************************************************/
boost::optional<Point2> ViewportOverlayArguments::projectPoint(const Point3& world_pos) const
{
	// Transform to view space:
	Point3 view_pos = projParams().viewMatrix * world_pos;
	// Project to screen space:
	Vector4 screen_pos = projParams().projectionMatrix * Vector4(view_pos.x(), view_pos.y(), view_pos.z(), 1);
	// Check if point is behind the viewer. If yes, stop here.
	if((projParams().isPerspective && view_pos.z() >= 0) || screen_pos.w() == 0) 
		return {};
	screen_pos.x() /= screen_pos.w();
	screen_pos.y() /= screen_pos.w();
	// Translate to window coordinates.
	const auto& win_rect = _painter.window();
	FloatType x = win_rect.left() + win_rect.width() * (screen_pos.x() + 1) / 2;
	FloatType y = win_rect.bottom() - win_rect.height() * (screen_pos.y() + 1) / 2 + 1;
	return { Point2(x, y) };
}

/******************************************************************************
* Projects a size from 3d world space to 2d window space.
******************************************************************************/
FloatType ViewportOverlayArguments::projectSize(const Point3& world_pos, FloatType radius3d) const
{
	if(projParams().isPerspective) {
		// Transform to view space.
		Point3 view_pos = projParams().viewMatrix * world_pos;
		// Project to screen space.
		Point3 screen_pos1 = projParams().projectionMatrix * view_pos;
		view_pos.y() += radius3d;
		Point3 screen_pos2 = projParams().projectionMatrix * view_pos;
		return (screen_pos1 - screen_pos2).length() * _painter.window().height() / 2;
	}
	else {
		return radius3d / projParams().fieldOfView * _painter.window().height() / 2;
	}
}

}	// End of namespace
