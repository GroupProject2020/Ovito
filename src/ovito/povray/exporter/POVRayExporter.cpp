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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/utilities/concurrent/Promise.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include "POVRayExporter.h"

namespace Ovito { namespace POVRay {

IMPLEMENT_OVITO_CLASS(POVRayExporter);

/******************************************************************************
* Constructs a new instance of the class.
******************************************************************************/
POVRayExporter::POVRayExporter(DataSet* dataset) : FileExporter(dataset)
{
}

/******************************************************************************
 * This is called once for every output file to be written and before
 * exportData() is called.
 *****************************************************************************/
bool POVRayExporter::openOutputFile(const QString& filePath, int numberOfFrames, AsyncOperation& operation)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	OVITO_ASSERT(!_renderer);

	_outputFile.setFileName(filePath);
	if(!_outputFile.open(QIODevice::WriteOnly | QIODevice::Text))
		throwException(tr("Failed to open output file '%1' for writing: %2").arg(filePath).arg(_outputFile.errorString()));

	// Take the already-existing POV-Ray renderer from the dataset if there is one.
	// Otherwise, create a temporary POV-Ray renderer for streaming the scene objects to the output file.
	POVRayRenderer* sceneRenderer = dynamic_object_cast<POVRayRenderer>(dataset()->renderSettings()->renderer());
	if(sceneRenderer)
		_renderer.reset(sceneRenderer);
	else
		_renderer.reset(new POVRayRenderer(dataset()));

	_renderer->setScriptOutputDevice(&_outputFile);
	if(!_renderer->startRender(dataset(), dataset()->renderSettings()))
		return false;

	return true;
}

/******************************************************************************
 * This is called once for every output file written after exportData()
 * has been called.
 *****************************************************************************/
void POVRayExporter::closeOutputFile(bool exportCompleted)
{
	if(_renderer) {
		_renderer->endRender();
		_renderer.reset();
	}
	if(_outputFile.isOpen())
		_outputFile.close();

	if(!exportCompleted)
		_outputFile.remove();
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool POVRayExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation)
{
	Viewport* vp = dataset()->viewportConfig()->activeViewport();
	if(!vp) throwException(tr("POV-Ray exporter requires an active viewport."));

	operation.setProgressText(tr("Writing data to POV-Ray file"));

	OVITO_ASSERT(_renderer);
	// Use a dummy bounding box to set up view projection.
	Box3 boundingBox(Point3::Origin(), 1);
	ViewProjectionParameters projParams = vp->computeProjectionParameters(time, _renderer->renderSettings()->outputImageAspectRatio(), boundingBox);
	try {
		_renderer->_exportOperation = operation.task();
		_renderer->beginFrame(time, projParams, vp);
		if(nodeToExport())
			_renderer->renderNode(nodeToExport(), operation);
	}
	catch(...) {
		_renderer->endFrame(false);
		throw;
	}
	_renderer->endFrame(!operation.isCanceled());
	return !operation.isCanceled();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
