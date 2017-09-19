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

#include <core/Core.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/DataSetContainer.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/dataset/scene/SceneRoot.h>
#include <core/dataset/scene/SelectionSet.h>
#include <core/rendering/RenderSettings.h>
#include <core/rendering/FrameBuffer.h>
#include <core/rendering/SceneRenderer.h>
#include <core/app/Application.h>
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
	setSceneRoot(new SceneRoot(this));
	setSelection(new SelectionSet(this));
	setRenderSettings(new RenderSettings(this));

	connect(&_pipelineEvaluationWatcher, &PromiseWatcher::finished, this, &DataSet::pipelineEvaluationFinished);
}

/******************************************************************************
* Destructor.
******************************************************************************/
DataSet::~DataSet()
{
	// Stop pipeline evaluation, which might still be in progress.
	_pipelineEvaluationWatcher.reset();
	if(_pipelineEvaluationFuture.isValid())
		_pipelineEvaluationFuture.cancelRequest();
}

/******************************************************************************
* Returns a viewport configuration that is used as template for new scenes.
******************************************************************************/
OORef<ViewportConfiguration> DataSet::createDefaultViewportConfiguration()
{
	UndoSuspender noUndo(undoStack());

	OORef<ViewportConfiguration> defaultViewportConfig = new ViewportConfiguration(this);

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
	defaultViewportConfig->setMaximizedViewport(nullptr);

	return defaultViewportConfig;
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool DataSet::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	OVITO_ASSERT_MSG(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread(), "DataSet::referenceEvent", "Reference events may only be processed in the main thread.");

	if(event->type() == ReferenceEvent::TargetChanged) {

		if(source == sceneRoot()) {

			// If any of the scene nodes change, the scene ready state needs to be reset (unless it's still unfulfilled).
			if(_sceneReadyFuture.isValid() && _sceneReadyFuture.isFinished()) {
//				qDebug() << "DataSet::referenceEvent() resetting scene ready promise (source=" << event->sender() << ")";
				_sceneReadyFuture.reset();
				_sceneReadyPromise.reset();
				OVITO_ASSERT(!_pipelineEvaluationFuture.isValid());
				OVITO_ASSERT(!_currentEvaluationNode);
			}

			// If any of the scene nodes change, we should interrupt the pipeline evaluation that is in progress.
			// Ignore messages from display objects, because they usually don't require a pipeline re-evaluation.
			if(_pipelineEvaluationFuture.isValid() && dynamic_object_cast<DisplayObject>(event->sender()) == nullptr) {
//				qDebug() << "DataSet::referenceEvent() canceling pipeline evaluation (due to changing source=" << event->sender() << ")";
//				qDebug() << "DataSet::referenceEvent: a target in the scene has changed:" << event->sender() << "vp suspended=" << viewportConfig()->isSuspended();
				// Restart pipeline evaluation immediately:
				makeSceneReady(true);
			}
		}
		else if(source == animationSettings()) {
			// If the animation time changes, we should interrupt any pipeline evaluation that is in progress.
			if(_pipelineEvaluationFuture.isValid() && _pipelineEvaluationTime != animationSettings()->time()) {
//				qDebug() << "DataSet::referenceEvent() canceling pipeline evaluation (due to changing animation time)";
				_pipelineEvaluationWatcher.reset();
				_currentEvaluationNode.clear();
				_pipelineEvaluationFuture.cancelRequest();
				// Restart pipeline evaluation immediately:
				makeSceneReady(false);
			}
		}
			
		if(source == sceneRoot() || source == selection() || source == renderSettings()) {
			return true;	// Propagate change event to DataSetContainer.
		}
		else {
			return false;	// Do not propagate change event to DataSetContainer.
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
//	qDebug() << "DataSet::whenSceneReady: time=" << animationSettings()->time() << "Is evaluation in progress:" << _pipelineEvaluationWatcher.isWatching();

	OVITO_CHECK_OBJECT_POINTER(sceneRoot());
	OVITO_CHECK_OBJECT_POINTER(animationSettings());
	OVITO_CHECK_OBJECT_POINTER(viewportConfig());
	OVITO_ASSERT(!viewportConfig()->isRendering());
	OVITO_ASSERT(_sceneReadyPromise.isValid() == _sceneReadyFuture.isValid());

	if(_sceneReadyFuture.isValid() && _sceneReadyFuture.isFinished() && _sceneReadyTime != animationSettings()->time()) {
//		qDebug() << "DataSet::whenSceneReady() resetting scene ready promise because animation time has changed";
		_sceneReadyFuture.reset();
		_sceneReadyPromise.reset();
	}
	
	if(!_sceneReadyFuture.isValid()) {
		_sceneReadyPromise = Promise<>::createSynchronous(nullptr, true, false);
//		qDebug() << "DataSet::whenSceneReady: creating scene ready future:" << _sceneReadyPromise.sharedState().get();
		_sceneReadyFuture = _sceneReadyPromise.future();
		_sceneReadyTime = animationSettings()->time();
		makeSceneReady(false);
	}

	OVITO_ASSERT(!_sceneReadyFuture.isCanceled());
	return _sceneReadyFuture;
}

/******************************************************************************
* Makes sure all data pipeline have been evaluated.
******************************************************************************/
void DataSet::makeSceneReady(bool forceReevaluation)
{
	//qDebug() << "DataSet::makeSceneReady(): time=" << animationSettings()->time();
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
//		qDebug() << "DataSet::makeSceneReady(): returning with already finished future";
		return;
	}

	// Is there already a pipeline evaluation in progress?
	if(_pipelineEvaluationFuture.isValid()) {
		// Keep waiting for the current pipeline evaluation to finish unless we are at the different animation time now.
		// Or unless the node has been deleted from the scene in the meantime.
		if(!forceReevaluation && _pipelineEvaluationTime == animationSettings()->time() && _currentEvaluationNode && _currentEvaluationNode->isChildOf(sceneRoot())) {
//			qDebug() << "DataSet::makeSceneReady(): returning with already in progress future";
			return;
		}
	}

	// If viewport updates are suspended, we simply wait until they are resumed.
	if(viewportConfig()->isSuspended()) {
//		qDebug() << "DataSet::makeSceneReady(): returning with unfinished future, because viewports are suspended";
		return;
	}

	// Request result of the data pipeline of each scene node.
	// If at least one of them is not immediately available, we'll have to
	// wait until its pipeline results become available.
	_pipelineEvaluationTime = animationSettings()->time();
	_currentEvaluationNode.clear();
	_pipelineEvaluationWatcher.reset();
//	qDebug() << "DataSet::makeSceneReady(): re-evaluating pipelines at time" << _pipelineEvaluationTime;

	SharedFuture<PipelineFlowState> newPipelineEvaluationFuture;
	sceneRoot()->visitObjectNodes([this,&newPipelineEvaluationFuture](ObjectNode* node) {
		// Request display objects as well.
		SharedFuture<PipelineFlowState> stateFuture = node->evaluateRenderingPipeline(_pipelineEvaluationTime);
		if(!stateFuture.isFinished()) {
			// Wait for this state to become available and return a pending future.
			_currentEvaluationNode = node;
			_pipelineEvaluationWatcher.watch(stateFuture.sharedState());
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

//	if(_currentEvaluationNode)
//		qDebug() << "DataSet::makeSceneReady(): pipeline evaluation in progress";
//	else
//		qDebug() << "DataSet::makeSceneReady(): all pipelines complete";
	
	if(_pipelineEvaluationFuture.isValid()) {
//		qDebug() << "DataSet::makeSceneReady(): canceling old request in progress";
		_pipelineEvaluationFuture.cancelRequest();
	}
	_pipelineEvaluationFuture = std::move(newPipelineEvaluationFuture);
		
	//qDebug() << "  DataSet::makeSceneReady: isReady=" << (!_currentEvaluationNode) << "waiting for pipeline=" << _pipelineEvaluationWatcher.hasPromise() << "promise=" << _pipelineEvaluationWatcher.promiseState().get();

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
//	qDebug() << "DataSet::onViewportUpdatesResumed(): calling makeSceneReady";
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
	if(_pipelineEvaluationFuture.isCanceled() == false) {
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

//	qDebug() << "DataSet::pipelineEvaluationFinished: Pipeline evaluation complete";

	// One of the pipelines in the scene became ready. 
	// Check if there are more pending pipelines in the scene.
	makeSceneReady(false);
}

/******************************************************************************
* This is the high-level rendering function, which invokes the renderer to generate one or more
* output images of the scene. All rendering parameters are specified in the RenderSettings object.
******************************************************************************/
bool DataSet::renderScene(RenderSettings* settings, Viewport* viewport, FrameBuffer* frameBuffer, TaskManager& taskManager)
{
	OVITO_CHECK_OBJECT_POINTER(settings);
	OVITO_CHECK_OBJECT_POINTER(viewport);
	OVITO_ASSERT(frameBuffer);

	// Get the selected scene renderer.
	SceneRenderer* renderer = settings->renderer();
	if(!renderer) throwException(tr("No rendering engine has been selected."));

	Promise<> renderTask = Promise<>::createSynchronous(&taskManager, true, true);
	renderTask.setProgressText(tr("Initializing renderer"));
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
				videoEncoder->openFile(settings->imageFilename(), settings->outputImageWidth(), settings->outputImageHeight(), animationSettings()->framesPerSecond());
			}
#endif

			if(settings->renderingRangeType() == RenderSettings::CURRENT_FRAME) {
				// Render a single frame.
				TimePoint renderTime = animationSettings()->time();
				int frameNumber = animationSettings()->timeToFrame(renderTime);
				renderTask.setProgressText(QString());
				if(!renderFrame(renderTime, frameNumber, settings, renderer, viewport, frameBuffer, videoEncoder, taskManager))
					renderTask.cancel();
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
				renderTask.setProgressMaximum(numberOfFrames);

				// Render frames, one by one.
				for(int frameIndex = 0; frameIndex < numberOfFrames; frameIndex++) {
					int frameNumber = firstFrameNumber + frameIndex * settings->everyNthFrame() + settings->fileNumberBase();

					renderTask.setProgressValue(frameIndex);
					renderTask.setProgressText(tr("Rendering animation (frame %1 of %2)").arg(frameIndex+1).arg(numberOfFrames));

					if(!renderFrame(renderTime, frameNumber, settings, renderer, viewport, frameBuffer, videoEncoder, taskManager))
						renderTask.cancel();
					if(renderTask.isCanceled())
						break;

					// Go to next animation frame.
					renderTime += animationSettings()->ticksPerFrame() * settings->everyNthFrame();
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
	}
	catch(Exception& ex) {
		// Shutdown renderer.
		renderer->endRender();
		// Provide a context for this error.
		if(ex.context() == nullptr) ex.setContext(this);
		throw;
	}

	return !renderTask.isCanceled();
}

/******************************************************************************
* Renders a single frame and saves the output file.
******************************************************************************/
bool DataSet::renderFrame(TimePoint renderTime, int frameNumber, RenderSettings* settings, SceneRenderer* renderer, Viewport* viewport,
		FrameBuffer* frameBuffer, VideoEncoder* videoEncoder, TaskManager& taskManager)
{
	// Determine output filename for this frame.
	QString imageFilename;
	if(settings->saveToFile() && !videoEncoder) {
		imageFilename = settings->imageFilename();
		if(imageFilename.isEmpty())
			throwException(tr("Cannot save rendered image to file, because no output filename has been specified."));

		if(settings->renderingRangeType() != RenderSettings::CURRENT_FRAME) {
			// Append frame number to file name if rendering an animation.
			QFileInfo fileInfo(imageFilename);
			imageFilename = fileInfo.path() + QChar('/') + fileInfo.baseName() + QString("%1.").arg(frameNumber, 4, 10, QChar('0')) + fileInfo.completeSuffix();

			// Check for existing image file and skip.
			if(settings->skipExistingImages() && QFileInfo(imageFilename).isFile())
				return true;
		}
	}

	Promise<> renderTask = Promise<>::createSynchronous(&taskManager, true, true);

	// Setup preliminary projection.
	ViewProjectionParameters projParams = viewport->computeProjectionParameters(renderTime, settings->outputImageAspectRatio());
		
	// Render one frame.
	frameBuffer->clear();
	try {
		// Request scene bounding box.
		Box3 boundingBox = renderer->computeSceneBoundingBox(renderTime, projParams, nullptr, renderTask);
		if(renderTask.isCanceled()) {
			renderer->endFrame(false);
			return false;
		}

		// Determine final view projection.
		projParams = viewport->computeProjectionParameters(renderTime, settings->outputImageAspectRatio(), boundingBox);

		renderer->beginFrame(renderTime, projParams, viewport);
		if(!renderer->renderFrame(frameBuffer, SceneRenderer::NonStereoscopic, renderTask)) {
			renderer->endFrame(false);
			return false;
		}
		renderer->endFrame(true);
	}
	catch(...) {
		renderer->endFrame(false);
		throw;
	}

	// Apply viewport overlays.
	for(ViewportOverlay* overlay : viewport->overlays()) {
		{
			QPainter painter(&frameBuffer->image());
			overlay->render(viewport, renderTime, painter, projParams, settings, false, taskManager);
			if(renderTask.isCanceled())
				return false;
		}
		frameBuffer->update();
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

	return !renderTask.isCanceled();
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
