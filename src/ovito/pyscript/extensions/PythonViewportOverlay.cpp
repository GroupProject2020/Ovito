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

#include <ovito/pyscript/PyScript.h>
#include <ovito/pyscript/binding/PythonBinding.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/app/Application.h>
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
			"        data = pipeline.compute(args.frame)\n"
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

	// Throw away compiled script function whenever script text changes.
	if(field == PROPERTY_FIELD(script)) {
		_scriptCompilationFuture.reset();
		_scriptCompilationOutput = tr("<Script compilation pending>\n");
		_scriptRenderingOutput.clear();
	}
}

/******************************************************************************
* Compiles the script entered by the user.
******************************************************************************/
SharedFuture<py::function> PythonViewportOverlay::compileScriptFunction()
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
				*scriptFunction = py::function(localNamespace["render"]);
				if(!py::isinstance<py::function>(*scriptFunction)) {
					throw Exception(tr("Invalid Python overlay script. It does not define a callable function named render()."));
				}
			}
			catch(const py::error_already_set&) {
				throw Exception(tr("Invalid Python overlay script. There is no function with the name 'render()'."));
			}

			return py::none();
		});

		// Update modifier status after compilation, to display Python log output.
		execFuture.finally(executor(), [this]() {
			notifyDependents(ReferenceEvent::ObjectStatusChanged);
		});

		// Make sure the Python function gets returned to the caller.
		_scriptCompilationFuture = execFuture.then([scriptFunction]() {
//		setStatus(PipelineStatus(PipelineStatus::Error, ex.message()));
			return std::move(*scriptFunction);
		});
	}
	return _scriptCompilationFuture;
}

/******************************************************************************
* Is called whenever the script generates some output during the compilation phase.
******************************************************************************/
void PythonViewportOverlay::appendCompilationOutput(const QString& text)
{
	// This is to collect script output during the compilation phase.
	_scriptCompilationOutput += text;
	notifyDependents(ReferenceEvent::ObjectStatusChanged);
}

/******************************************************************************
* Is called whenever the script generates some output during the rendering phase.
******************************************************************************/
void PythonViewportOverlay::appendRenderingOutput(const QString& text)
{
	// This is to collect script output during the rendering phase.
	_scriptRenderingOutput += text;
	notifyDependents(ReferenceEvent::ObjectStatusChanged);
}

/******************************************************************************
* This method asks the overlay to paint its contents over the rendered image.
******************************************************************************/
void PythonViewportOverlay::render(const Viewport* viewport, TimePoint time, FrameBuffer* frameBuffer, const ViewProjectionParameters& projParams, const RenderSettings* renderSettings, AsyncOperation& operation)
{
	// First compile the script function.
	SharedFuture<py::function> scriptFunctionFuture = compileScriptFunction();
	if(!operation.waitForFuture(scriptFunctionFuture))
		return;
	py::function scriptFunction = scriptFunctionFuture.result();

	// Reset log output.
	_scriptRenderingOutput.clear();

	try {
		// Run the following code within the context of a script engine.
		Future<> execFuture = ScriptEngine::executeAsync(this, "appendRenderingOutput", [scriptFunction,viewport,frameBuffer,projParams,renderSettings,time]() {

			// Create a QPainter for the frame buffer.
			QPainter painter(&frameBuffer->image());

			// Enable antialiasing for the QPainter by default.
			painter.setRenderHint(QPainter::Antialiasing);
			painter.setRenderHint(QPainter::TextAntialiasing);

			// Pass viewport, QPainter, and other information to the Python script function.
			// The QPainter pointer has to be converted to the representation used by PyQt5.
			py::module numpy_module = py::module::import("numpy");
			py::module qtgui_module = py::module::import("PyQt5.QtGui");
			py::module sip_module = py::module::import("sip");
			py::object painter_ptr = py::cast(reinterpret_cast<std::uintptr_t>(&painter));
			py::object qpainter_class = qtgui_module.attr("QPainter");
			py::object sip_painter = sip_module.attr("wrapinstance")(painter_ptr, qpainter_class);

			py::tuple arguments = py::make_tuple(py::cast(
				ViewportOverlayArguments(time, viewport, projParams, renderSettings, std::move(sip_painter), painter),
				py::return_value_policy::move));

			// Execute script render function.
			return scriptFunction(*std::move(arguments));
		});

		if(!operation.waitForFuture(execFuture))
			return;
	}
	catch(const Exception& ex) {
		// Interrupt rendering process in console mode; swallow exception in GUI mode.
		if(Application::instance()->consoleMode()) {
			throw;
		}
	}
}

/******************************************************************************
* This method paints the overlay contents on the given canvas.
******************************************************************************/
void PythonViewportOverlay::renderInteractive(const Viewport* viewport, TimePoint time, QPainter& painter, const ViewProjectionParameters& projParams, const RenderSettings* renderSettings, AsyncOperation& operation)
{
	// Reset log output.
	_scriptRenderingOutput.clear();

	// Request the compilation of the script function.
	SharedFuture<py::function> funcFuture = compileScriptFunction();

	// This object instance may be deleted while this method is being executed.
	// This is to detect this situation.
	QPointer<PythonViewportOverlay> thisPointer(this);
	PipelineStatus status = PipelineStatus::Success;

	try {
		// Check if an executable script function is already available.
		// In an interactive context, we cannot wait for it to complete.
		if(!funcFuture.isFinished()) {
			// However, make sure the viewport gets redrawn once compilation has completed.
			funcFuture.finally(viewport->executor(), [viewport]() {
				const_cast<Viewport*>(viewport)->updateViewport();
			});
			return;
		}

		py::function scriptFunction = funcFuture.result();

		// Make sure the actions of the script function are not recorded on the undo stack.
		UndoSuspender noUndo(dataset());

		// Enable antialiasing for the QPainter by default.
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::TextAntialiasing);

		// Run the following operation within the context of a scripting engine.
		ScriptEngine::executeSync(this, operation.task(), "appendRenderingOutput", [scriptFunction,viewport,&painter,&projParams,renderSettings,time]() {

			// Pass viewport, QPainter, and other information to the Python script function.
			// The QPainter pointer has to be converted to the representation used by PyQt5.
			py::module numpy_module = py::module::import("numpy");
			py::module qtgui_module = py::module::import("PyQt5.QtGui");
			py::module sip_module = py::module::import("sip");
			py::object painter_ptr = py::cast(reinterpret_cast<std::uintptr_t>(&painter));
			py::object qpainter_class = qtgui_module.attr("QPainter");
			py::object sip_painter = sip_module.attr("wrapinstance")(painter_ptr, qpainter_class);

			py::tuple arguments = py::make_tuple(py::cast(
				ViewportOverlayArguments(time, viewport, projParams, renderSettings, std::move(sip_painter), painter),
				py::return_value_policy::move));

			// Execute script render function.
			scriptFunction(*std::move(arguments));
		});
	}
	catch(const Exception& ex) {
		status = PipelineStatus(PipelineStatus::Error, ex.message());
		// Interrupt rendering process in console mode; swallow exceptions in GUI mode.
		if(Application::instance()->consoleMode()) {
			setStatus(status);
			throw;
		}
	}

	if(thisPointer) {
		setStatus(status);
		notifyDependents(ReferenceEvent::ObjectStatusChanged);
	}
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
