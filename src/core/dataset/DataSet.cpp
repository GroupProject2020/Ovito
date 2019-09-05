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

#include <core/Core.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/DataSetContainer.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/dataset/scene/RootSceneNode.h>
#include <core/dataset/scene/SelectionSet.h>
#include <core/rendering/RenderSettings.h>
#include <core/rendering/FrameBuffer.h>
#include <core/rendering/SceneRenderer.h>
#include <core/app/Application.h>
#include <core/app/StandaloneApplication.h>
#ifdef OVITO_VIDEO_OUTPUT_SUPPORT
	#include <core/utilities/io/video/VideoEncoder.h>
#endif

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

IMPLEMENT_OVITO_CLASS(DataSet);
DEFINE_REFERENCE_FIELD(DataSet, viewportConfig);
DEFINE_REFERENCE_FIELD(DataSet, animationSettings);
DEFINE_REFERENCE_FIELD(DataSet, sceneRoot);
DEFINE_REFERENCE_FIELD(DataSet, selection);
DEFINE_REFERENCE_FIELD(DataSet, renderSettings);
DEFINE_REFERENCE_FIELD(DataSet, globalObjects);
SET_PROPERTY_FIELD_LABEL(DataSet, viewportConfig, "Viewport Configuration");
SET_PROPERTY_FIELD_LABEL(DataSet, animationSettings, "Animation Settings");
SET_PROPERTY_FIELD_LABEL(DataSet, sceneRoot, "Scene");
SET_PROPERTY_FIELD_LABEL(DataSet, selection, "Selection");
SET_PROPERTY_FIELD_LABEL(DataSet, renderSettings, "Render Settings");
SET_PROPERTY_FIELD_LABEL(DataSet, globalObjects, "Global objects");

/******************************************************************************
* Constructor.
******************************************************************************/
DataSet::DataSet(DataSet* self) : RefTarget(this), _unitsManager(this)
{
	setViewportConfig(createDefaultViewportConfiguration());
	setAnimationSettings(new AnimationSettings(this));
	setSceneRoot(new RootSceneNode(this));
	setSelection(new SelectionSet(this));
	setRenderSettings(new RenderSettings(this));

	connect(&_pipelineEvaluationWatcher, &TaskWatcher::finished, this, &DataSet::pipelineEvaluationFinished);
}

/******************************************************************************
* Destructor.
******************************************************************************/
DataSet::~DataSet()
{
	// Stop pipeline evaluation, which might still be in progress.
	_pipelineEvaluationWatcher.reset();
	if(_pipelineEvaluationFuture.isValid()) {
		_pipelineEvaluationFuture.cancelRequest();
	}
}

/******************************************************************************
* Returns the TaskManager responsible for this DataSet.
******************************************************************************/
TaskManager& DataSet::taskManager() const
{
	return container()->taskManager();
}

/******************************************************************************
* Returns a viewport configuration that is used as template for new scenes.
******************************************************************************/
OORef<ViewportConfiguration> DataSet::createDefaultViewportConfiguration()
{
	UndoSuspender noUndo(undoStack());

	OORef<ViewportConfiguration> defaultViewportConfig = new ViewportConfiguration(this);

	if(!StandaloneApplication::instance() || !StandaloneApplication::instance()->cmdLineParser().isSet("noviewports")) {
		OORef<Viewport> topView = new Viewport(this);
		topView->setViewType(Viewport::VIEW_TOP);
		defaultViewportConfig->addViewport(topView);

		OORef<Viewport> frontView = new Viewport(this);
		frontView->setViewType(Viewport::VIEW_FRONT);
		defaultViewportConfig->addViewport(frontView);

		OORef<Viewport> leftView = new Viewport(this);
		leftView->setViewType(Viewport::VIEW_LEFT);
		defaultViewportConfig->addViewport(leftView);

		OORef<Viewport> perspectiveView = new Viewport(this);
		perspectiveView->setViewType(Viewport::VIEW_PERSPECTIVE);
		perspectiveView->setCameraTransformation(ViewportSettings::getSettings().coordinateSystemOrientation() * AffineTransformation::lookAlong({90, -120, 100}, {-90, 120, -100}, {0,0,1}).inverse());
		defaultViewportConfig->addViewport(perspectiveView);

		defaultViewportConfig->setActiveViewport(perspectiveView);

		Viewport::ViewType maximizedViewportType = static_cast<Viewport::ViewType>(ViewportSettings::getSettings().defaultMaximizedViewportType());
		if(maximizedViewportType != Viewport::VIEW_NONE) {
			for(Viewport* vp : defaultViewportConfig->viewports()) {
				if(vp->viewType() == maximizedViewportType) {
					defaultViewportConfig->setActiveViewport(vp);
					defaultViewportConfig->setMaximizedViewport(vp);
					break;
				}
			}
			if(!defaultViewportConfig->maximizedViewport()) {
				defaultViewportConfig->setMaximizedViewport(defaultViewportConfig->activeViewport());
				if(maximizedViewportType > Viewport::VIEW_NONE && maximizedViewportType <= Viewport::VIEW_PERSPECTIVE)
					defaultViewportConfig->maximizedViewport()->setViewType(maximizedViewportType);
			}
		}
		else defaultViewportConfig->setMaximizedViewport(nullptr);
	}

	return defaultViewportConfig;
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool DataSet::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	OVITO_ASSERT_MSG(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread(), "DataSet::referenceEvent", "Reference events may only be processed in the main thread.");

	if(event.type() == ReferenceEvent::TargetChanged) {

		if(source == sceneRoot()) {

			// If any of the scene nodes change, the scene ready state needs to be reset (unless it's still unfulfilled).
			if(_sceneReadyFuture.isValid() && _sceneReadyFuture.isFinished()) {
				_sceneReadyFuture.reset();
				_sceneReadyPromise.reset();
				OVITO_ASSERT(!_pipelineEvaluationFuture.isValid());
				OVITO_ASSERT(!_currentEvaluationNode);
			}

			// If any of the scene nodes change, we should interrupt the pipeline evaluation that is in progress.
			// Ignore messages from display objects, because they usually don't require a pipeline re-evaluation.
			if(_pipelineEvaluationFuture.isValid() && dynamic_object_cast<DataVis>(event.sender()) == nullptr) {
				// Restart pipeline evaluation immediately:
				makeSceneReady(true);
			}
		}
		else if(source == animationSettings()) {
			// If the animation time changes, we should interrupt any pipeline evaluation that is in progress.
			if(_pipelineEvaluationFuture.isValid() && _pipelineEvaluationTime != animationSettings()->time()) {
				_pipelineEvaluationWatcher.reset();
				_currentEvaluationNode.clear();
				_pipelineEvaluationFuture.cancelRequest();
				// Restart pipeline evaluation immediately:
				makeSceneReady(false);
			}
		}

		// Propagate event only from certain sources to the DataSetContainer:
		return (source == sceneRoot() || source == selection() || source == renderSettings());
	}
	else if(event.type() == ReferenceEvent::AnimationFramesChanged && source == sceneRoot() && !isBeingLoaded()) {
		// Automatically adjust scene's animation interval to length of loaded source animations.
		if(animationSettings()->autoAdjustInterval()) {
			UndoSuspender noUndo(this);
			animationSettings()->adjustAnimationInterval();
		}
	}
	return RefTarget::referenceEvent(source, event);
}

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void DataSet::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(viewportConfig)) {
		Q_EMIT viewportConfigReplaced(viewportConfig());

		// Whenever viewport updates are resumed, we also resume evaluation of the scene's data pipelines.
		if(oldTarget) disconnect(static_cast<ViewportConfiguration*>(oldTarget), &ViewportConfiguration::viewportUpdateResumed, this, &DataSet::onViewportUpdatesResumed);
		if(newTarget) connect(static_cast<ViewportConfiguration*>(newTarget), &ViewportConfiguration::viewportUpdateResumed, this, &DataSet::onViewportUpdatesResumed);
	}
	else if(field == PROPERTY_FIELD(animationSettings)) {
		// Stop animation playback when animation settings are being replaced.
		if(AnimationSettings* oldAnimSettings = static_object_cast<AnimationSettings>(oldTarget))
			oldAnimSettings->stopAnimationPlayback();

		Q_EMIT animationSettingsReplaced(animationSettings());
	}
	else if(field == PROPERTY_FIELD(renderSettings)) {
		Q_EMIT renderSettingsReplaced(renderSettings());
	}
	else if(field == PROPERTY_FIELD(selection)) {
		Q_EMIT selectionSetReplaced(selection());
	}

	// Install a signal/slot connection that updates the viewports every time the animation time has changed.
	if(field == PROPERTY_FIELD(viewportConfig) || field == PROPERTY_FIELD(animationSettings)) {
		disconnect(_updateViewportOnTimeChangeConnection);
		if(animationSettings() && viewportConfig()) {
			_updateViewportOnTimeChangeConnection = connect(animationSettings(), &AnimationSettings::timeChangeComplete, viewportConfig(), &ViewportConfiguration::updateViewports);
			viewportConfig()->updateViewports();
		}
	}

	RefTarget::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* Returns the container to which this dataset belongs.
******************************************************************************/
DataSetContainer* DataSet::container() const
{
	OVITO_ASSERT_MSG(!_container.isNull(), "DataSet::container()", "DataSet is not in a DataSetContainer.");
	return _container.data();
}

/******************************************************************************
* Deletes all nodes from the scene.
******************************************************************************/
void DataSet::clearScene()
{
	while(!sceneRoot()->children().empty())
		sceneRoot()->children().back()->deleteNode();
}

/******************************************************************************
* Rescales the animation keys of all controllers in the scene.
******************************************************************************/
void DataSet::rescaleTime(const TimeInterval& oldAnimationInterval, const TimeInterval& newAnimationInterval)
{
	// Iterate over all controllers in the scene.
	for(RefTarget* reftarget : getAllDependencies()) {
		if(Controller* ctrl = dynamic_object_cast<Controller>(reftarget)) {
			ctrl->rescaleTime(oldAnimationInterval, newAnimationInterval);
		}
	}
}

/******************************************************************************
* Returns a future that is triggered once all data pipelines in the scene
* have been completely evaluated at the current animation time.
******************************************************************************/
SharedFuture<> DataSet::whenSceneReady()
{
	OVITO_CHECK_OBJECT_POINTER(sceneRoot());
	OVITO_CHECK_OBJECT_POINTER(animationSettings());
	OVITO_CHECK_OBJECT_POINTER(viewportConfig());
	OVITO_ASSERT(!viewportConfig()->isRendering());
	OVITO_ASSERT(_sceneReadyPromise.isValid() == _sceneReadyFuture.isValid());

	if(_sceneReadyFuture.isValid() && _sceneReadyFuture.isFinished() && _sceneReadyTime != animationSettings()->time()) {
		_sceneReadyFuture.reset();
		_sceneReadyPromise.reset();
	}

	if(!_sceneReadyFuture.isValid()) {
		_sceneReadyPromise = SignalPromise::create(true);
		_sceneReadyFuture = _sceneReadyPromise.future();
		_sceneReadyTime = animationSettings()->time();
		makeSceneReady(false);
	}

	OVITO_ASSERT(!_sceneReadyFuture.isCanceled());
	return _sceneReadyFuture;
}

/******************************************************************************
* Requests the (re-)evaluation of all data pipelines in the current scene.
******************************************************************************/
void DataSet::makeSceneReady(bool forceReevaluation)
{
	OVITO_ASSERT(_sceneReadyPromise.isValid() == _sceneReadyFuture.isValid());

	// Make sure whenSceneReady() was called before.
	if(!_sceneReadyFuture.isValid()) {
		OVITO_ASSERT(!_currentEvaluationNode);
		OVITO_ASSERT(!_pipelineEvaluationFuture.isValid());
		return;
	}

	OVITO_ASSERT(!_sceneReadyFuture.isCanceled());

	// If scene is already ready, we are done.
	if(_sceneReadyFuture.isFinished() && _pipelineEvaluationTime == animationSettings()->time()) {
		return;
	}

	// Is there already a pipeline evaluation in progress?
	if(_pipelineEvaluationFuture.isValid()) {
		// Keep waiting for the current pipeline evaluation to finish unless we are at the different animation time now.
		// Or unless the node has been deleted from the scene in the meantime.
		if(!forceReevaluation && _pipelineEvaluationTime == animationSettings()->time() && _currentEvaluationNode && _currentEvaluationNode->isChildOf(sceneRoot())) {
			return;
		}
	}

	// If viewport updates are suspended, we simply wait until they are resumed.
	if(viewportConfig()->isSuspended()) {
		return;
	}

	// Request result of the data pipeline of each scene node.
	// If at least one of them is not immediately available, we'll have to
	// wait until its pipeline results become available.
	_pipelineEvaluationTime = animationSettings()->time();
	_currentEvaluationNode.clear();
	_pipelineEvaluationWatcher.reset();

	SharedFuture<PipelineFlowState> newPipelineEvaluationFuture;
	sceneRoot()->visitObjectNodes([this,&newPipelineEvaluationFuture](PipelineSceneNode* node) {
		// Request display objects as well.
		SharedFuture<PipelineFlowState> stateFuture = node->evaluateRenderingPipeline(_pipelineEvaluationTime);
		if(!stateFuture.isFinished()) {
			// Wait for this state to become available and return a pending future.
			_currentEvaluationNode = node;
			_pipelineEvaluationWatcher.watch(stateFuture.task());
			newPipelineEvaluationFuture = std::move(stateFuture);
			return false;
		}
		else if(!stateFuture.isCanceled()) {
			try { stateFuture.results(); }
			catch(...) {
				qWarning() << "DataSet::makeSceneReady(): An exception was thrown in a data pipeline. This should never happen.";
				OVITO_ASSERT(false);
			}
		}
		return true;
	});

	if(_pipelineEvaluationFuture.isValid()) {
		_pipelineEvaluationFuture.cancelRequest();
	}
	_pipelineEvaluationFuture = std::move(newPipelineEvaluationFuture);

	// If all pipelines are already complete, we are done.
	if(!_currentEvaluationNode) {
		_sceneReadyPromise.setFinished();
		OVITO_ASSERT(_sceneReadyFuture.isFinished());
	}
}

/******************************************************************************
* Is called whenver viewport updates are resumed.
******************************************************************************/
void DataSet::onViewportUpdatesResumed()
{
	makeSceneReady(true);
}

/******************************************************************************
* Is called when the pipeline evaluation of a scene node has finished.
******************************************************************************/
void DataSet::pipelineEvaluationFinished()
{
	OVITO_ASSERT(_sceneReadyFuture.isValid());
	OVITO_ASSERT(_sceneReadyPromise.isValid() == _sceneReadyFuture.isValid());
	OVITO_ASSERT(!_sceneReadyFuture.isCanceled());
	OVITO_ASSERT(_currentEvaluationNode);
	OVITO_ASSERT(_pipelineEvaluationFuture.isValid());
	OVITO_ASSERT(_pipelineEvaluationFuture.isFinished());

	// Query results of the pipeline evaluation to see if an exception has been thrown.
	if(!_pipelineEvaluationFuture.isCanceled()) {
		try {
			_pipelineEvaluationFuture.results();
		}
		catch(...) {
			qWarning() << "DataSet::pipelineEvaluationFinished(): An exception was thrown in a data pipeline. This should never happen.";
			OVITO_ASSERT(false);
		}
	}

	_pipelineEvaluationFuture.reset();
	_pipelineEvaluationWatcher.reset();
	_currentEvaluationNode.clear();

	// One of the pipelines in the scene became ready.
	// Check if there are more pending pipelines in the scene.
	makeSceneReady(false);
}

/******************************************************************************
* This is the high-level rendering function, which invokes the renderer to generate one or more
* output images of the scene. All rendering parameters are specified in the RenderSettings object.
******************************************************************************/
bool DataSet::renderScene(RenderSettings* settings, Viewport* viewport, FrameBuffer* frameBuffer, AsyncOperation&& operation)
{
	OVITO_CHECK_OBJECT_POINTER(settings);
	OVITO_CHECK_OBJECT_POINTER(viewport);
	OVITO_ASSERT(frameBuffer);

	// Get the selected scene renderer.
	SceneRenderer* renderer = settings->renderer();
	if(!renderer) throwException(tr("No rendering engine has been selected."));

	operation.setProgressText(tr("Initializing renderer"));
	try {

		// Resize output frame buffer.
		if(frameBuffer->size() != QSize(settings->outputImageWidth(), settings->outputImageHeight())) {
			frameBuffer->setSize(QSize(settings->outputImageWidth(), settings->outputImageHeight()));
			frameBuffer->clear();
		}

		// Don't update viewports while rendering.
		ViewportSuspender noVPUpdates(this);

		// Initialize the renderer.
		if(renderer->startRender(this, settings)) {

			VideoEncoder* videoEncoder = nullptr;
#ifdef OVITO_VIDEO_OUTPUT_SUPPORT
			QScopedPointer<VideoEncoder> videoEncoderPtr;
			// Initialize video encoder.
			if(settings->saveToFile() && settings->imageInfo().isMovie()) {

				if(settings->imageFilename().isEmpty())
					throwException(tr("Cannot save rendered images to movie file. Output filename has not been specified."));

				videoEncoderPtr.reset(new VideoEncoder());
				videoEncoder = videoEncoderPtr.data();
				int fps = settings->framesPerSecond() > 0 ? settings->framesPerSecond() : animationSettings()->framesPerSecond();
				videoEncoder->openFile(settings->imageFilename(), settings->outputImageWidth(), settings->outputImageHeight(), fps);
			}
#endif

			if(settings->renderingRangeType() == RenderSettings::CURRENT_FRAME) {
				// Render a single frame.
				TimePoint renderTime = animationSettings()->time();
				int frameNumber = animationSettings()->timeToFrame(renderTime);
				operation.setProgressText(tr("Rendering frame %1").arg(frameNumber));
				renderFrame(renderTime, frameNumber, settings, renderer, viewport, frameBuffer, videoEncoder, std::move(operation));
			}
			else if(settings->renderingRangeType() == RenderSettings::CUSTOM_FRAME) {
				// Render a specific frame.
				TimePoint renderTime = animationSettings()->frameToTime(settings->customFrame());
				operation.setProgressText(tr("Rendering frame %1").arg(settings->customFrame()));
				renderFrame(renderTime, settings->customFrame(), settings, renderer, viewport, frameBuffer, videoEncoder, std::move(operation));
			}
			else if(settings->renderingRangeType() == RenderSettings::ANIMATION_INTERVAL || settings->renderingRangeType() == RenderSettings::CUSTOM_INTERVAL) {
				// Render an animation interval.
				TimePoint renderTime;
				int firstFrameNumber, numberOfFrames;
				if(settings->renderingRangeType() == RenderSettings::ANIMATION_INTERVAL) {
					renderTime = animationSettings()->animationInterval().start();
					firstFrameNumber = animationSettings()->timeToFrame(animationSettings()->animationInterval().start());
					numberOfFrames = (animationSettings()->timeToFrame(animationSettings()->animationInterval().end()) - firstFrameNumber + 1);
				}
				else {
					firstFrameNumber = settings->customRangeStart();
					renderTime = animationSettings()->frameToTime(firstFrameNumber);
					numberOfFrames = (settings->customRangeEnd() - firstFrameNumber + 1);
				}
				numberOfFrames = (numberOfFrames + settings->everyNthFrame() - 1) / settings->everyNthFrame();
				if(numberOfFrames < 1)
					throwException(tr("Invalid rendering range: Frame %1 to %2").arg(settings->customRangeStart()).arg(settings->customRangeEnd()));
				operation.setProgressMaximum(numberOfFrames);

				// Render frames, one by one.
				for(int frameIndex = 0; frameIndex < numberOfFrames; frameIndex++) {
					int frameNumber = firstFrameNumber + frameIndex * settings->everyNthFrame() + settings->fileNumberBase();

					operation.setProgressValue(frameIndex);
					operation.setProgressText(tr("Rendering animation (frame %1 of %2)").arg(frameIndex+1).arg(numberOfFrames));

					renderFrame(renderTime, frameNumber, settings, renderer, viewport, frameBuffer, videoEncoder, operation.createSubTask());
					if(operation.isCanceled())
						break;

					// Go to next animation frame.
					renderTime += animationSettings()->ticksPerFrame() * settings->everyNthFrame();

					// Periodically free visual element resources during animation rendering to avoid clogging the memory.
					visCache().discardUnusedObjects();
				}
			}

#ifdef OVITO_VIDEO_OUTPUT_SUPPORT
			// Finalize movie file.
			if(videoEncoder)
				videoEncoder->closeFile();
#endif
		}

		// Shutdown renderer.
		renderer->endRender();

		// Free visual element resources to avoid clogging the memory in cases where render() get called repeatedly from a script.
		if(Application::instance()->executionContext() == Application::ExecutionContext::Scripting)
			visCache().discardUnusedObjects();
	}
	catch(Exception& ex) {
		// Shutdown renderer.
		renderer->endRender();
		// Provide a context for this error.
		if(ex.context() == nullptr) ex.setContext(this);
		throw;
	}

	return !operation.isCanceled();
}

/******************************************************************************
* Renders a single frame and saves the output file.
******************************************************************************/
bool DataSet::renderFrame(TimePoint renderTime, int frameNumber, RenderSettings* settings, SceneRenderer* renderer, Viewport* viewport,
		FrameBuffer* frameBuffer, VideoEncoder* videoEncoder, AsyncOperation&& operation)
{
	// Determine output filename for this frame.
	QString imageFilename;
	if(settings->saveToFile() && !videoEncoder) {
		imageFilename = settings->imageFilename();
		if(imageFilename.isEmpty())
			throwException(tr("Cannot save rendered image to file, because no output filename has been specified."));

		// Append frame number to filename when rendering an animation.
		if(settings->renderingRangeType() != RenderSettings::CURRENT_FRAME && settings->renderingRangeType() != RenderSettings::CUSTOM_FRAME) {
			QFileInfo fileInfo(imageFilename);
			imageFilename = fileInfo.path() + QChar('/') + fileInfo.baseName() + QString("%1.").arg(frameNumber, 4, 10, QChar('0')) + fileInfo.completeSuffix();

			// Check for existing image file and skip.
			if(settings->skipExistingImages() && QFileInfo(imageFilename).isFile())
				return true;
		}
	}

	// Set up preliminary projection.
	ViewProjectionParameters projParams = viewport->computeProjectionParameters(renderTime, settings->outputImageAspectRatio());

	// Fill frame buffer with background color.
	if(!settings->generateAlphaChannel()) {
		frameBuffer->clear(ColorA(settings->backgroundColor()));
	}
	else {
		frameBuffer->clear();
	}

	// Request scene bounding box.
	Box3 boundingBox = renderer->computeSceneBoundingBox(renderTime, projParams, nullptr, operation);
	if(operation.isCanceled()) {
		renderer->endFrame(false);
		return false;
	}

	// Determine final view projection.
	projParams = viewport->computeProjectionParameters(renderTime, settings->outputImageAspectRatio(), boundingBox);

	// Render one frame.
	try {
		// Render viewport "underlays".
		for(ViewportOverlay* overlay : viewport->overlays()) {
			if(overlay->isEnabled() && overlay->renderBehindScene()) {
				{
					overlay->render(viewport, renderTime, frameBuffer, projParams, settings, operation);
					if(operation.isCanceled()) {
						renderer->endFrame(false);
						return false;
					}
				}
				frameBuffer->update();
			}
		}

		// Let the scene renderer do its work.
		renderer->beginFrame(renderTime, projParams, viewport);
		if(!renderer->renderFrame(frameBuffer, SceneRenderer::NonStereoscopic, operation)) {
			renderer->endFrame(false);
			return false;
		}
		renderer->endFrame(true);
	}
	catch(...) {
		renderer->endFrame(false);
		throw;
	}

	// Render viewport overlays on top.
	for(ViewportOverlay* overlay : viewport->overlays()) {
		if(overlay->isEnabled() && !overlay->renderBehindScene()) {
			{
				overlay->render(viewport, renderTime, frameBuffer, projParams, settings, operation);
				if(operation.isCanceled())
					return false;
			}
			frameBuffer->update();
		}
	}

	// Save rendered image to disk.
	if(settings->saveToFile()) {
		if(!videoEncoder) {
			OVITO_ASSERT(!imageFilename.isEmpty());
			if(!frameBuffer->image().save(imageFilename, settings->imageInfo().format()))
				throwException(tr("Failed to save rendered image to output file '%1'.").arg(imageFilename));
		}
		else {
#ifdef OVITO_VIDEO_OUTPUT_SUPPORT
			videoEncoder->writeFrame(frameBuffer->image());
#endif
		}
	}

	return !operation.isCanceled();
}

/******************************************************************************
* Saves the dataset to the given file.
******************************************************************************/
void DataSet::saveToFile(const QString& filePath)
{
	QFile fileStream(filePath);
    if(!fileStream.open(QIODevice::WriteOnly))
    	throwException(tr("Failed to open output file '%1' for writing.").arg(filePath));

	QDataStream dataStream(&fileStream);
	ObjectSaveStream stream(dataStream);
	stream.saveObject(this);
	stream.close();

	if(fileStream.error() != QFile::NoError)
		throwException(tr("Failed to write output file '%1'.").arg(filePath));
	fileStream.close();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
