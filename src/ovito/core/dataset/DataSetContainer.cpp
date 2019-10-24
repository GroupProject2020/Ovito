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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/io/FileImporter.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/utilities/io/ObjectSaveStream.h>
#include <ovito/core/utilities/io/ObjectLoadStream.h>
#include <ovito/core/utilities/io/FileManager.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

IMPLEMENT_OVITO_CLASS(DataSetContainer);
DEFINE_REFERENCE_FIELD(DataSetContainer, currentSet);

/******************************************************************************
* Initializes the dataset manager.
******************************************************************************/
DataSetContainer::DataSetContainer() : _taskManager(this)
{
}

/******************************************************************************
* Destructor.
******************************************************************************/
DataSetContainer::~DataSetContainer()
{
	setCurrentSet(nullptr);
	taskManager().cancelAllAndWait();
	clearAllReferences();
}

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void DataSetContainer::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(currentSet)) {

		if(oldTarget) {
			DataSet* oldDataSet = static_object_cast<DataSet>(oldTarget);

			// Stop animation playback for the old dataset.
			oldDataSet->animationSettings()->stopAnimationPlayback();

			if(oldDataSet->_container == this)
				oldDataSet->_container = nullptr;
		}

		// Forward signals from the current dataset.
		disconnect(_selectionSetReplacedConnection);
		disconnect(_viewportConfigReplacedConnection);
		disconnect(_animationSettingsReplacedConnection);
		disconnect(_renderSettingsReplacedConnection);
		disconnect(_filePathChangedConnection);
		disconnect(_undoStackCleanChangedConnection);
		if(currentSet()) {
			_selectionSetReplacedConnection = connect(currentSet(), &DataSet::selectionSetReplaced, this, &DataSetContainer::onSelectionSetReplaced);
			_viewportConfigReplacedConnection = connect(currentSet(), &DataSet::viewportConfigReplaced, this, &DataSetContainer::viewportConfigReplaced);
			_animationSettingsReplacedConnection = connect(currentSet(), &DataSet::animationSettingsReplaced, this, &DataSetContainer::animationSettingsReplaced);
			_renderSettingsReplacedConnection = connect(currentSet(), &DataSet::renderSettingsReplaced, this, &DataSetContainer::renderSettingsReplaced);
			_filePathChangedConnection = connect(currentSet(), &DataSet::filePathChanged, this, &DataSetContainer::filePathChanged);
			_undoStackCleanChangedConnection = connect(&currentSet()->undoStack(), &UndoStack::cleanChanged, this, &DataSetContainer::modificationStatusChanged);
			currentSet()->_container = this;
		}

		Q_EMIT dataSetChanged(currentSet());

		if(currentSet()) {
			Q_EMIT viewportConfigReplaced(currentSet()->viewportConfig());
			Q_EMIT animationSettingsReplaced(currentSet()->animationSettings());
			Q_EMIT renderSettingsReplaced(currentSet()->renderSettings());
			Q_EMIT filePathChanged(currentSet()->filePath());
			Q_EMIT modificationStatusChanged(currentSet()->undoStack().isClean());
			onSelectionSetReplaced(currentSet()->selection());
			onAnimationSettingsReplaced(currentSet()->animationSettings());
		}
		else {
			onSelectionSetReplaced(nullptr);
			onAnimationSettingsReplaced(nullptr);
			Q_EMIT viewportConfigReplaced(nullptr);
			Q_EMIT animationSettingsReplaced(nullptr);
			Q_EMIT renderSettingsReplaced(nullptr);
			Q_EMIT filePathChanged(QString());
			Q_EMIT modificationStatusChanged(true);
		}
	}
	RefMaker::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* This handler is invoked when the current selection set of the current dataset
* has been replaced.
******************************************************************************/
void DataSetContainer::onSelectionSetReplaced(SelectionSet* newSelectionSet)
{
	// Forward signals from the current selection set.
	disconnect(_selectionSetChangedConnection);
	disconnect(_selectionSetChangeCompleteConnection);
	if(newSelectionSet) {
		_selectionSetChangedConnection = connect(newSelectionSet, &SelectionSet::selectionChanged, this, &DataSetContainer::selectionChanged);
		_selectionSetChangeCompleteConnection = connect(newSelectionSet, &SelectionSet::selectionChangeComplete, this, &DataSetContainer::selectionChangeComplete);
	}
	Q_EMIT selectionSetReplaced(newSelectionSet);
	Q_EMIT selectionChanged(newSelectionSet);
	Q_EMIT selectionChangeComplete(newSelectionSet);
}

/******************************************************************************
* This handler is invoked when the current animation settings of the current
* dataset have been replaced.
******************************************************************************/
void DataSetContainer::onAnimationSettingsReplaced(AnimationSettings* newAnimationSettings)
{
	// Forward signals from the current animation settings object.
	disconnect(_animationTimeChangedConnection);
	disconnect(_animationTimeChangeCompleteConnection);
	if(newAnimationSettings) {
		_animationTimeChangedConnection = connect(newAnimationSettings, &AnimationSettings::timeChanged, this, &DataSetContainer::timeChanged);
		_animationTimeChangeCompleteConnection = connect(newAnimationSettings, &AnimationSettings::timeChangeComplete, this, &DataSetContainer::timeChangeComplete);
	}
	if(newAnimationSettings) {
		Q_EMIT timeChanged(newAnimationSettings->time());
		Q_EMIT timeChangeComplete();
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
