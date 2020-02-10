////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/actions/ActionManager.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/desktop/widgets/rendering/FrameBufferWindow.h>
#include <ovito/gui/desktop/utilities/concurrent/ProgressDialog.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/utilities/concurrent/AsyncOperation.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

/******************************************************************************
* Handles the ACTION_RENDER_ACTIVE_VIEWPORT command.
******************************************************************************/
void ActionManager::on_RenderActiveViewport_triggered()
{
	try {
		// Set input focus to main window.
		// This will process any pending user inputs in QLineEdit fields that haven't been processed yet.
		mainWindow()->setFocus();

		// Get the current render settings.
		RenderSettings* settings = _dataset->renderSettings();

		// Get viewport to be rendered.
		Viewport* viewport = _dataset->viewportConfig()->activeViewport();
		if(!viewport)
			throw Exception(tr("There is no active viewport to render."), _dataset);

		// Get frame buffer and window.
		FrameBufferWindow* frameBufferWindow = mainWindow()->frameBufferWindow();

		// Allocate and resize frame buffer and frame buffer window if necessary.
		std::shared_ptr<FrameBuffer> frameBuffer = frameBufferWindow->createFrameBuffer(settings->outputImageWidth(), settings->outputImageHeight());

		// Show and activate frame buffer window.
		frameBufferWindow->showAndActivateWindow();

		// Create a task object for the rendering operation.
		AsyncOperation renderingOperation(_dataset->taskManager());

		// Show progress dialog.
		ProgressDialog progressDialog(frameBufferWindow, renderingOperation.task(), tr("Rendering"));

		// Call high-level rendering function, which will take care of the rest.
		_dataset->renderScene(settings, viewport, frameBuffer.get(), std::move(renderingOperation));
	}
	catch(const Exception& ex) {
		ex.logError();
		ex.reportError();
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
