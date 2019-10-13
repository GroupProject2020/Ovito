////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/opengl/OpenGLSceneRenderer.h>
#include "AdhocApplication.h"

namespace PyScript {

/******************************************************************************
* Initializes the application object.
******************************************************************************/
bool AdhocApplication::initialize()
{
	if(!Application::initialize())
		return false;

	// Initialize application state.
	PluginManager::initialize();

	// Create a DataSetContainer and a default DataSet.
	_datasetContainer = new DataSetContainer();
	_datasetContainer->setParent(this);
	_datasetContainer->setCurrentSet(new DataSet());
	// Do not record operations for undo.
	_datasetContainer->currentSet()->undoStack().suspend();

#if defined(Q_OS_LINUX)
	// On Unix/Linux, use headless mode if no X server is available.
	if(!qEnvironmentVariableIsEmpty("DISPLAY"))
		_headlessMode = false;
#elif defined(Q_OS_OSX) || defined(Q_OS_WIN)
	// On Windows and macOS, there is always an OpenGL implementation available for background rendering.
	_headlessMode = false;
#endif

	// Set the global default OpenGL surface format.
	// This will let Qt use core profile contexts.
	QSurfaceFormat::setDefaultFormat(OpenGLSceneRenderer::getDefaultSurfaceFormat());

	return true;
}

}	// End of namespace
