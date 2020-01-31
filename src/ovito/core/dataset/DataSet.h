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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/oo/PropertyField.h>
#include <ovito/core/dataset/animation/TimeInterval.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/dataset/pipeline/PipelineEvaluation.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/utilities/concurrent/SharedFuture.h>
#include <ovito/core/utilities/concurrent/Promise.h>
#include <ovito/core/utilities/concurrent/TaskWatcher.h>
#include <ovito/core/utilities/concurrent/SignalPromise.h>
#include <ovito/core/utilities/MixedKeyCache.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/**
 * \brief Stores the current program state including the three-dimensional scene, viewport configuration,
 *        render settings etc.
 *
 * A DataSet represents the state of the current user session.
 * It can be completely saved to a file (.ovito suffix) and loaded again at a later time.
 *
 * The DataSet class consists of various sub-objects that store different aspects. The
 * ViewportConfiguration object returned by viewportConfig(), for example, stores the list
 * of viewports.
 */
class OVITO_CORE_EXPORT DataSet : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(DataSet)
	Q_PROPERTY(Ovito::AnimationSettings* animationSettings READ animationSettings WRITE setAnimationSettings NOTIFY animationSettingsReplaced);
	Q_PROPERTY(Ovito::ViewportConfiguration* viewportConfiguration READ viewportConfig WRITE setViewportConfig NOTIFY viewportConfigReplaced);

public:

	/// \brief Constructs an empty dataset.
	/// \param self This parameter is not used and is there to provide a constructor signature that is compatible
	///             with the RefTarget base class.
	Q_INVOKABLE DataSet(DataSet* self = nullptr);

	/// \brief Destructor.
	virtual ~DataSet();

	/// \brief Returns the path where this dataset is stored on disk.
	/// \return The location where the dataset is stored or will be stored on disk.
	const QString& filePath() const { return _filePath; }

	/// \brief Sets the path where this dataset is stored.
	/// \param path The new path (should be absolute) where the dataset will be stored.
	void setFilePath(const QString& path) {
		if(path != _filePath) {
			_filePath = path;
			Q_EMIT filePathChanged(_filePath);
		}
	}

	/// \brief Returns the undo stack that keeps track of changes made to this dataset.
	UndoStack& undoStack() {
		OVITO_CHECK_OBJECT_POINTER(this);
		return _undoStack;
	}

	/// \brief Returns the manager of ParameterUnit objects.
	UnitsManager& unitsManager() { return _unitsManager; }

	/// \brief Returns a pointer to the main window in which this dataset is being edited.
	/// \return The main window, or NULL if this data set is not being edited in any window.
	//MainWindow* mainWindow() const;

	/// \brief Returns the container this dataset belongs to.
	DataSetContainer* container() const;

	/// Returns the TaskManager responsible for this DataSet.
	TaskManager& taskManager() const;

	/// \brief Deletes all nodes from the scene.
	/// \undoable
	Q_INVOKABLE void clearScene();

	/// \brief Rescales the animation keys of all controllers in the scene.
	/// \param oldAnimationInterval The old animation interval, which will be mapped to the new animation interval.
	/// \param newAnimationInterval The new animation interval.
	///
	/// This method calls Controller::rescaleTime() for all controllers in the scene.
	/// For keyed controllers this will rescale the key times of all keys from the
	/// old animation interval to the new interval using a linear mapping.
	///
	/// Keys that lie outside of the old active animation interval will also be rescaled
	/// according to a linear extrapolation.
	///
	/// \undoable
	void rescaleTime(const TimeInterval& oldAnimationInterval, const TimeInterval& newAnimationInterval);

	/// \brief This is the high-level rendering function, which invokes the renderer to generate one or more
	///        output images of the scene. All rendering parameters are specified in the RenderSettings object.
	/// \param settings A RenderSettings object that specifies output image size, animation range to render etc.
	/// \param viewport The viewport to render. This determines the camera orientation.
	/// \param frameBuffer The frame buffer that will receive the rendered image. When rendering an animation
	///        sequence, the buffer will contain only the last rendered frame when the function returns.
	/// \return true on success; false if operation has been canceled by the user.
	/// \throw Exception on error.
	bool renderScene(RenderSettings* settings, Viewport* viewport, FrameBuffer* frameBuffer, AsyncOperation&& operation);

	/// \brief Returns a future that is triggered once all data pipelines in the scene
	///        have been completely evaluated at the current animation time.
	SharedFuture<> whenSceneReady();

	/// \brief Saves the dataset to the given file.
	/// \throw Exception on error.
	///
	/// Note that this method does NOT invoke setFilePath().
	void saveToFile(const QString& filePath);

	/// \brief Appends an object to this dataset's list of global objects.
	void addGlobalObject(const RefTarget* target) {
		if(!_globalObjects.contains(target))
			_globalObjects.push_back(this, PROPERTY_FIELD(globalObjects), target);
	}

	/// \brief Removes an object from this dataset's list of global objects.
	void removeGlobalObject(int index) {
		_globalObjects.remove(this, PROPERTY_FIELD(globalObjects), index);
	}

	/// \brief Looks for a global object of the given type.
	template<class T>
	T* findGlobalObject() const {
		for(RefTarget* obj : globalObjects()) {
			T* castObj = dynamic_object_cast<T>(obj);
			if(castObj) return castObj;
		}
		return nullptr;
	}

	/// Provides access to the global data cache used by visualzation elements.
	MixedKeyCache& visCache() { return _visCache; }

Q_SIGNALS:

	/// \brief This signal is emitted whenever the current viewport configuration of this dataset
	///        has been replaced by a new one.
	/// \note This signal is NOT emitted when parameters of the current viewport configuration change.
    void viewportConfigReplaced(ViewportConfiguration* newViewportConfiguration);

	/// \brief This signal is emitted whenever the current animation settings of this dataset
	///        have been replaced by new ones.
	/// \note This signal is NOT emitted when parameters of the current animation settings object change.
    void animationSettingsReplaced(AnimationSettings* newAnimationSettings);

	/// \brief This signal is emitted whenever the current render settings of this dataset
	///        have been replaced by new ones.
	/// \note This signal is NOT emitted when parameters of the current render settings object change.
    void renderSettingsReplaced(RenderSettings* newRenderSettings);

	/// \brief This signal is emitted whenever the current selection set of this dataset
	///        has been replaced by another one.
	/// \note This signal is NOT emitted when nodes are added or removed from the current selection set.
    void selectionSetReplaced(SelectionSet* newSelectionSet);

	/// \brief This signal is emitted whenever the dataset has been saved under a new file name.
    void filePathChanged(const QString& filePath);

protected:

	/// Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Is called when the value of a reference field of this RefMaker changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

private:

	/// Renders a single frame and saves the output file. This is part of the implementation of the renderScene() method.
	bool renderFrame(TimePoint renderTime, int frameNumber, RenderSettings* settings, SceneRenderer* renderer,
			Viewport* viewport, FrameBuffer* frameBuffer, VideoEncoder* videoEncoder, AsyncOperation&& operation);

	/// Returns a viewport configuration that is used as template for new scenes.
	OORef<ViewportConfiguration> createDefaultViewportConfiguration();

	/// Requests the (re-)evaluation of all data pipelines in the current scene.
	void makeSceneReady(bool forceReevaluation);

private Q_SLOTS:

	/// Is called when the pipeline evaluation of a scene node has finished.
	void pipelineEvaluationFinished();

	/// Is called whenver viewport updates are resumed.
	void onViewportUpdatesResumed();

private:

	/// The configuration of the viewports.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(ViewportConfiguration, viewportConfig, setViewportConfig, PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);

	/// Current animation settings.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(AnimationSettings, animationSettings, setAnimationSettings, PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);

	/// Root node of the scene node tree.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(RootSceneNode, sceneRoot, setSceneRoot, PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_ALWAYS_DEEP_COPY);

	/// The current node selection set.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(SelectionSet, selection, setSelection, PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_ALWAYS_DEEP_COPY);

	/// The settings used when rendering the scene.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(RenderSettings, renderSettings, setRenderSettings, PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);

	/// Global data managed by plugins.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD_FLAGS(RefTarget, globalObjects, setGlobalObjects, PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_ALWAYS_CLONE|PROPERTY_FIELD_ALWAYS_DEEP_COPY);

	/// The file path this DataSet has been saved to.
	QString _filePath;

	/// The undo stack that keeps track of changes made to this dataset.
	UndoStack _undoStack;

	/// The manager of ParameterUnit objects.
	UnitsManager _unitsManager;

	/// This signal/slot connection updates the viewports when the animation time changes.
	QMetaObject::Connection _updateViewportOnTimeChangeConnection;

	/// The promise of the scene becoming ready.
	SignalPromise _sceneReadyPromise;

	/// The future of the scene becoming ready.
	SharedFuture<> _sceneReadyFuture;

	/// The last animation time at which the scene was made ready.
	TimePoint _sceneReadyTime;

	/// The current pipeline evaluation that is in progress.
	PipelineEvaluationFuture _pipelineEvaluation;

	/// The watcher object that is used to monitor the evaluation of data pipelines in the scene.
	TaskWatcher _pipelineEvaluationWatcher;

	/// The DataSetContainer which currently hosts this DataSet.
	QPointer<DataSetContainer> _container;

	/// Data cache used by visualzation elements.
	MixedKeyCache _visCache;

	friend class DataSetContainer;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
