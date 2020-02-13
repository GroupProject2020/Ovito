////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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

#include <ovito/core/Core.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/concurrent/TaskWatcher.h>
#include <ovito/core/dataset/DataSet.h>
#include "AnimationSettings.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(AnimationSettings);
DEFINE_PROPERTY_FIELD(AnimationSettings, time);
DEFINE_PROPERTY_FIELD(AnimationSettings, animationInterval);
DEFINE_PROPERTY_FIELD(AnimationSettings, ticksPerFrame);
DEFINE_PROPERTY_FIELD(AnimationSettings, playbackSpeed);
DEFINE_PROPERTY_FIELD(AnimationSettings, loopPlayback);
DEFINE_PROPERTY_FIELD(AnimationSettings, autoAdjustInterval);

/******************************************************************************
* Constructor.
******************************************************************************/
AnimationSettings::AnimationSettings(DataSet* dataset) : RefTarget(dataset),
		_ticksPerFrame(TICKS_PER_SECOND/10),
		_playbackSpeed(1),
		_animationInterval(0, 0),
		_time(0),
		_loopPlayback(true),
		_autoAdjustInterval(true)
{
}

/******************************************************************************
* Is called when the value of a non-animatable property field of this RefMaker has changed.
******************************************************************************/
void AnimationSettings::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(time))
		onTimeChanged();
	else if(field == PROPERTY_FIELD(animationInterval))
		Q_EMIT intervalChanged(animationInterval());
	else if(field == PROPERTY_FIELD(ticksPerFrame))
		Q_EMIT speedChanged(ticksPerFrame());
	else if(field == PROPERTY_FIELD(autoAdjustInterval) && autoAdjustInterval() && !isBeingLoaded())
		adjustAnimationInterval();
}

/******************************************************************************
* Saves the class' contents to an output stream.
******************************************************************************/
void AnimationSettings::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	RefTarget::saveToStream(stream, excludeRecomputableData);
	stream.beginChunk(0x01);
	stream << _namedFrames;
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from an input stream.
******************************************************************************/
void AnimationSettings::loadFromStream(ObjectLoadStream& stream)
{
	RefTarget::loadFromStream(stream);
	stream.expectChunk(0x01);
	stream >> _namedFrames;
	stream.closeChunk();
}

/******************************************************************************
* Creates a copy of this object.
******************************************************************************/
OORef<RefTarget> AnimationSettings::clone(bool deepCopy, CloneHelper& cloneHelper) const
{
	// Let the base class create an instance of this class.
	OORef<AnimationSettings> clone = static_object_cast<AnimationSettings>(RefTarget::clone(deepCopy, cloneHelper));

	// Copy internal data.
	clone->_namedFrames = this->_namedFrames;

	return clone;
}

/******************************************************************************
* Is called when the current animation time has changed.
******************************************************************************/
void AnimationSettings::onTimeChanged()
{
	Q_EMIT timeChanged(time());
	if(_isTimeChanging)
		return;
	_isTimeChanging = true;

	// Wait until scene is complete, then generate a timeChangeComplete event.
	_sceneReadyFuture = dataset()->whenSceneReady().then(executor(), [this]() {
		_isTimeChanging = false;
		_sceneReadyFuture.reset();
		Q_EMIT timeChangeComplete();
	});
}

/******************************************************************************
* Converts a time value to its string representation.
******************************************************************************/
QString AnimationSettings::timeToString(TimePoint time)
{
	return QString::number(timeToFrame(time));
}

/******************************************************************************
* Converts a string to a time value.
* Throws an exception when a parsing error occurs.
******************************************************************************/
TimePoint AnimationSettings::stringToTime(const QString& stringValue)
{
	TimePoint value;
	bool ok;
	value = (TimePoint)stringValue.toInt(&ok);
	if(!ok)
		throwException(tr("Invalid frame number format: %1").arg(stringValue));
	return frameToTime(value);
}

/******************************************************************************
* Enables or disables auto key generation mode.
******************************************************************************/
void AnimationSettings::setAutoKeyMode(bool on)
{
	if(_autoKeyMode == on)
		return;

	_autoKeyMode = on;
	Q_EMIT autoKeyModeChanged(_autoKeyMode);
}

/******************************************************************************
* Sets the current animation time to the start of the animation interval.
******************************************************************************/
void AnimationSettings::jumpToAnimationStart()
{
	setTime(animationInterval().start());
}

/******************************************************************************
* Sets the current animation time to the end of the animation interval.
******************************************************************************/
void AnimationSettings::jumpToAnimationEnd()
{
	setTime(animationInterval().end());
}

/******************************************************************************
* Jumps to the previous animation frame.
******************************************************************************/
void AnimationSettings::jumpToPreviousFrame()
{
	// Subtract one frame from current time.
	TimePoint newTime = frameToTime(timeToFrame(time()) - 1);
	// Clamp new time
	newTime = std::max(newTime, animationInterval().start());
	// Set new time.
	setTime(newTime);
}

/******************************************************************************
* Jumps to the previous animation frame.
******************************************************************************/
void AnimationSettings::jumpToNextFrame()
{
	// Subtract one frame from current time.
	TimePoint newTime = frameToTime(timeToFrame(time()) + 1);
	// Clamp new time
	newTime = std::min(newTime, animationInterval().end());
	// Set new time.
	setTime(newTime);
}

/******************************************************************************
* Starts or stops animation playback in the viewports.
******************************************************************************/
void AnimationSettings::setAnimationPlayback(bool on)
{
	if(on) {
		bool reverse = false;
		if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive) {
			if(QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
				reverse = true;
		}
		startAnimationPlayback(reverse ? -1 : 1);
	}
	else {
		stopAnimationPlayback();
	}
}

/******************************************************************************
* Starts playback of the animation in the viewports.
******************************************************************************/
void AnimationSettings::startAnimationPlayback(FloatType playbackRate)
{
	if(_activePlaybackRate != playbackRate) {
		_activePlaybackRate = playbackRate;
		Q_EMIT playbackChanged(_activePlaybackRate != 0);

		if(_activePlaybackRate > 0) {
			if(time() < animationInterval().end())
				scheduleNextAnimationFrame();
			else
				continuePlaybackAtTime(animationInterval().start());
		}
		else if(_activePlaybackRate < 0) {
			if(time() > animationInterval().start())
				scheduleNextAnimationFrame();
			else
				continuePlaybackAtTime(animationInterval().end());
		}
	}
}

/******************************************************************************
* Jumps to the given animation time, then schedules the next frame as soon as
* the scene was completely shown.
******************************************************************************/
void AnimationSettings::continuePlaybackAtTime(TimePoint time)
{
	setTime(time);

	if(isPlaybackActive()) {
		
		// Tell DataSet to make the scene ready at the current animation frame.
		SharedFuture<> playbackFrameReadyFuture = dataset()->whenSceneReady();

		// Once the scene is ready, schedule the next animation frame.
		playbackFrameReadyFuture.finally(executor(), false, [this, future = playbackFrameReadyFuture]() {
			if(future.isCanceled())
				stopAnimationPlayback();
			else
				scheduleNextAnimationFrame();
		});
	}
}

/******************************************************************************
* Starts a timer to show the next animation frame.
******************************************************************************/
void AnimationSettings::scheduleNextAnimationFrame()
{
	if(!isPlaybackActive()) return;

	int timerSpeed = 1000 / std::abs(_activePlaybackRate);
	if(playbackSpeed() > 1) timerSpeed /= playbackSpeed();
	else if(playbackSpeed() < -1) timerSpeed *= -playbackSpeed();
	QTimer::singleShot(timerSpeed / framesPerSecond(), this, &AnimationSettings::onPlaybackTimer);
}

/******************************************************************************
* Stops playback of the animation in the viewports.
******************************************************************************/
void AnimationSettings::stopAnimationPlayback()
{
	if(isPlaybackActive()) {
		_activePlaybackRate = 0;
		Q_EMIT playbackChanged(false);
	}
}

/******************************************************************************
* Timer callback used during animation playback.
******************************************************************************/
void AnimationSettings::onPlaybackTimer()
{
	// Check if the animation playback has been deactivated in the meantime.
	if(!isPlaybackActive())
		return;

	// Add one frame to current time
	int newFrame = timeToFrame(time()) + (_activePlaybackRate > 0 ? 1 : -1);
	TimePoint newTime = frameToTime(newFrame);

	// Loop back to first frame if end has been reached.
	if(newTime > animationInterval().end()) {
		if(loopPlayback() && animationInterval().duration() > 0) {
			newTime = animationInterval().start();
		}
		else {
			newTime = animationInterval().end();
			stopAnimationPlayback();
		}
	}
	else if(newTime < animationInterval().start()) {
		if(loopPlayback() && animationInterval().duration() > 0) {
			newTime = animationInterval().end();
		}
		else {
			newTime = animationInterval().start();
			stopAnimationPlayback();
		}
	}

	// Set new time and continue playing.
	continuePlaybackAtTime(newTime);
}

/******************************************************************************
* Recalculates the length of the animation interval to accomodate all loaded
* source animations in the scene.
******************************************************************************/
void AnimationSettings::adjustAnimationInterval()
{
	TimeInterval interval;
	_namedFrames.clear();
	dataset()->sceneRoot()->visitObjectNodes([&interval,this](PipelineSceneNode* node) {
		if(node->dataProvider()) {
			int nframes = node->dataProvider()->numberOfSourceFrames();
			if(nframes > 0) {

				// Final animation interval should encompass the local intervals
				// of all animated objects in the scene.
				TimePoint start = node->dataProvider()->sourceFrameToAnimationTime(0);
				if(interval.isEmpty() || start < interval.start()) interval.setStart(start);
				TimePoint end = node->dataProvider()->sourceFrameToAnimationTime(nframes) - 1;
				if(interval.isEmpty() || end > interval.end()) interval.setEnd(end);

				// Save the list of the named animation frames.
				// Merge with other list(s) from other scene objects if there are any.
				if(_namedFrames.empty())
					_namedFrames = node->dataProvider()->animationFrameLabels();
				else {
					auto additionalLabels = node->dataProvider()->animationFrameLabels();
					if(!additionalLabels.empty())
						_namedFrames.unite(additionalLabels);
				}
			}
		}
		return true;
	});
	if(interval.isEmpty())
		interval.setInstant(0);
	else {
		// Round interval to nearest frame time.
		// Always include frame 0 in the animation interval.
		interval.setStart(std::min(0, frameToTime(timeToFrame(interval.start()))));
		interval.setEnd(frameToTime(timeToFrame(interval.end())));
	}
	setAnimationInterval(interval);
	if(time() < interval.start())
		setTime(interval.start());
	else if(time() > interval.end())
		setTime(interval.end());
}

/******************************************************************************
* uspends the automatic generation of animation keys by calling
* AnimationSettings::suspendAnim().
******************************************************************************/
AnimationSuspender::AnimationSuspender(RefMaker* object) : AnimationSuspender(object->dataset()->animationSettings())
{
}

}	// End of namespace
