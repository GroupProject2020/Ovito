////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#include <ovito/pyscript/PyScript.h>
#include <ovito/gui/widgets/rendering/FrameBufferWindow.h>
#include <ovito/gui/mainwin/MainWindow.h>
#include <ovito/gui/dataset/GuiDataSetContainer.h>
#include <ovito/gui/viewport/ViewportWindow.h>
#include <ovito/gui/viewport/input/ViewportInputManager.h>
#include <ovito/pyscript/binding/PythonBinding.h>
#include <ovito/core/app/PluginManager.h>

namespace PyScript {

using namespace Ovito;

PYBIND11_MODULE(PyScriptGui, m)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();

	py::options options;
	options.disable_function_signatures();

	py::class_<MainWindow>(m, "MainWindow")
		.def_property_readonly("frame_buffer_window", &MainWindow::frameBufferWindow, py::return_value_policy::reference)
	;

	py::class_<GuiDataSetContainer, DataSetContainer>(m, "GuiDataSetContainer")
		//.def("fileNew", &GuiDataSetContainer::fileNew)
		//.def("fileLoad", &GuiDataSetContainer::fileLoad)
		//.def("fileSave", &GuiDataSetContainer::fileSave)
		//.def("fileSaveAs", &GuiDataSetContainer::fileSaveAs)
		//.def("askForSaveChanges", &GuiDataSetContainer::askForSaveChanges)
		.def_property_readonly("window", &GuiDataSetContainer::mainWindow, py::return_value_policy::reference)
	;

	py::class_<FrameBufferWindow>(m, "FrameBufferWindow")
		.def_property_readonly("frame_buffer", &FrameBufferWindow::frameBuffer)
		.def("create_frame_buffer", &FrameBufferWindow::createFrameBuffer)
		.def("show_and_activate", &FrameBufferWindow::showAndActivateWindow)
	;

	py::class_<ViewportWindow>(m, "ViewportWindow")
		// This helper function is part of the implementation of the Viewport.create_widget() method.
		// It creates a ViewportWindow for the given Viewport and return its C++ address.
		// On the Python side, the Viewport.create_widget() method will wrap it in a QWidget SIP object.
		.def_static("_create", [](Viewport* vp, std::uintptr_t parentPtr) {
			if(vp->window())
				vp->throwException("Viewport is already associated with a viewport widget. Cannot create more than one widget for the same viewport.");
			QWidget* parentWidget = reinterpret_cast<QWidget*>(parentPtr);
			// Also create a ViewportInputManager for the viewport window, which will manage handling of
			// mouse input events for the viewport.
			ViewportInputManager* inputManager = new ViewportInputManager(nullptr, *vp->dataset()->container());
			ViewportWindow* vpWin = new ViewportWindow(vp, inputManager, parentWidget);
			inputManager->setParent(vpWin);
			// Active the default mouse input mode.
			inputManager->reset();
			// The viewport should also be registered with the dataset's ViewportConfiguration object
			// in order to take part in the viewport update mechanism.
			if(!vp->dataset()->viewportConfig()->viewports().contains(vp))
				vp->dataset()->viewportConfig()->addViewport(vp);
			return reinterpret_cast<std::uintptr_t>(vpWin);
		})
	;
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(PyScriptGui);

}
