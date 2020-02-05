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

#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/stdmod/modifiers/ManualSelectionModifier.h>
#include <ovito/stdobj/gui/widgets/PropertyContainerParameterUI.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/viewport/ViewportWindowInterface.h>
#include <ovito/gui/desktop/actions/ViewportModeAction.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/base/rendering/ViewportSceneRenderer.h>
#include <ovito/gui/base/viewport/ViewportInputManager.h>
#include <ovito/gui/base/viewport/ViewportInputMode.h>
#include "ManualSelectionModifierEditor.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ManualSelectionModifierEditor);
SET_OVITO_OBJECT_EDITOR(ManualSelectionModifier, ManualSelectionModifierEditor);

/**
 * Viewport input mode that allows to pick individual elements, adding and removing them
 * from the current selection set.
 */
class PickElementMode : public ViewportInputMode
{
public:

	/// Constructor.
	PickElementMode(ManualSelectionModifierEditor* editor) : ViewportInputMode(editor), _editor(editor) {}

	/// Handles the mouse up events for a Viewport.
	virtual void mouseReleaseEvent(ViewportWindowInterface* vpwin, QMouseEvent* event) override {
		if(event->button() == Qt::LeftButton) {
			ManualSelectionModifier* mod = static_object_cast<ManualSelectionModifier>(_editor->editObject());
			if(mod && mod->subject()) {
				// Find out what's under the mouse cursor.
				ViewportPickResult pickResult = vpwin->pick(event->pos());
				if(pickResult.isValid()) {
					// Look up the index of the element that was picked.
					std::pair<size_t, ConstDataObjectPath> indexAndContainer = mod->subject().dataClass()->elementFromPickResult(pickResult);
					if(indexAndContainer.first != std::numeric_limits<size_t>::max()) {
						// Let the editor class handle it from here.
						_editor->onElementPicked(pickResult, indexAndContainer.first, indexAndContainer.second);
					}
					else {
						inputManager()->mainWindow()->showStatusBarMessage(tr("You did not click on an element of type '%1'.").arg(mod->subject().dataClass()->elementDescriptionName()), 1000);
					}
				}
			}
		}
		ViewportInputMode::mouseReleaseEvent(vpwin, event);
	}

	/// Handles the mouse events for a Viewport.
	virtual void mouseMoveEvent(ViewportWindowInterface* vpwin, QMouseEvent* event) override {
		ViewportInputMode::mouseMoveEvent(vpwin, event);

		// Check if a selectable element is beneath the mouse cursor position.
		// If yes, indicate that by changing the mouse cursor shape.
		ManualSelectionModifier* mod = static_object_cast<ManualSelectionModifier>(_editor->editObject());
		if(mod && mod->subject()) {
			// Find out what's under the mouse cursor.
			ViewportPickResult pickResult = vpwin->pick(event->pos());
			if(pickResult.isValid()) {
				// Look up the index of the element.
				std::pair<size_t, ConstDataObjectPath> indexAndContainer = mod->subject().dataClass()->elementFromPickResult(pickResult);
				if(indexAndContainer.first != std::numeric_limits<size_t>::max()) {
					setCursor(SelectionMode::selectionCursor());
					return;
				}
			}
		}

		// Switch back to default mouse cursor.
		setCursor(QCursor());
	}

	ManualSelectionModifierEditor* _editor;
};

/**
 * Viewport input mode that allows to select a group of elements by drawing a fence around them.
 */
class FenceSelectionMode : public ViewportInputMode, public ViewportGizmo
{
public:

	/// Constructor.
	FenceSelectionMode(ManualSelectionModifierEditor* editor) : ViewportInputMode(editor), _editor(editor) {}

	/// Destructor.
	virtual ~FenceSelectionMode() {
		if(isActive())
			inputManager()->removeInputMode(this);
	}

	/// Handles the mouse down events for a Viewport.
	virtual void mousePressEvent(ViewportWindowInterface* vpwin, QMouseEvent* event) override {
		_fence.clear();
		if(event->button() == Qt::LeftButton) {
			_fence.push_back(Point2(event->localPos().x(), event->localPos().y())
					* (FloatType)vpwin->devicePixelRatio());
			vpwin->viewport()->updateViewport();
		}
		else ViewportInputMode::mousePressEvent(vpwin, event);
	}

	/// Handles the mouse move events for a Viewport.
	virtual void mouseMoveEvent(ViewportWindowInterface* vpwin, QMouseEvent* event) override {
		if(!_fence.isEmpty()) {
			_fence.push_back(Point2(event->localPos().x(), event->localPos().y())
					* (FloatType)vpwin->devicePixelRatio());
			vpwin->viewport()->updateViewport();
		}
		ViewportInputMode::mouseMoveEvent(vpwin, event);
	}

	/// Handles the mouse up events for a Viewport.
	virtual void mouseReleaseEvent(ViewportWindowInterface* vpwin, QMouseEvent* event) override {
		if(!_fence.isEmpty()) {
			if(_fence.size() >= 3) {
				ElementSelectionSet::SelectionMode mode = ElementSelectionSet::SelectionReplace;
				if(event->modifiers().testFlag(Qt::ControlModifier))
					mode = ElementSelectionSet::SelectionAdd;
				else if(event->modifiers().testFlag(Qt::AltModifier))
					mode = ElementSelectionSet::SelectionSubtract;
				_editor->onFence(_fence, vpwin->viewport(), mode);
			}
			_fence.clear();
			vpwin->viewport()->updateViewport();
		}
		ViewportInputMode::mouseReleaseEvent(vpwin, event);
	}

	/// Lets the input mode render its 2d overlay content in a viewport.
	virtual void renderOverlay2D(Viewport* vp, SceneRenderer* renderer) override {
		if(isActive() && vp == vp->dataset()->viewportConfig()->activeViewport() && _fence.size() >= 2) {
			if(ViewportSceneRenderer* vpRenderer = dynamic_object_cast<ViewportSceneRenderer>(renderer))
				vpRenderer->render2DPolyline(_fence.constData(), _fence.size(), ViewportSettings::getSettings().viewportColor(ViewportSettings::COLOR_SELECTION), true);
		}
	}

protected:

	/// This is called by the system when the input handler has become active.
	virtual void activated(bool temporary) override {
		ViewportInputMode::activated(temporary);
		ManualSelectionModifier* mod = static_object_cast<ManualSelectionModifier>(_editor->editObject());
		if(mod && mod->subject()) {
#ifndef Q_OS_MACX
			inputManager()->mainWindow()->showStatusBarMessage(
					tr("Draw a fence around a group of %1 to select. Use CONTROL or ALT keys to extend or reduce existing selection set.")
					.arg(mod->subject().dataClass()->elementDescriptionName()));
#else
			inputManager()->mainWindow()->showStatusBarMessage(
					tr("Draw a fence around a group of %1 to select. Use COMMAND or ALT keys to extend or reduce existing selection set.")
					.arg(mod->subject().dataClass()->elementDescriptionName()));
#endif
		}
		inputManager()->addViewportGizmo(this);
	}

	/// This is called by the system after the input handler is no longer the active handler.
	virtual void deactivated(bool temporary) override {
		_fence.clear();
		inputManager()->mainWindow()->clearStatusBarMessage();
		inputManager()->removeViewportGizmo(this);
		ViewportInputMode::deactivated(temporary);
	}

private:

	ManualSelectionModifierEditor* _editor;
	QVector<Point2> _fence;
};

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ManualSelectionModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Manual selection"), rolloutParams, "particles.modifiers.manual_selection.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);

	QGroupBox* operateOnGroup = new QGroupBox(tr("Operate on"));
	QVBoxLayout* sublayout = new QVBoxLayout(operateOnGroup);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(6);
	layout->addWidget(operateOnGroup);

	PropertyContainerParameterUI* pclassUI = new PropertyContainerParameterUI(this, PROPERTY_FIELD(GenericPropertyModifier::subject));
	sublayout->addWidget(pclassUI->comboBox());

	// List only property containers that support element selection.
	pclassUI->setContainerFilter([](const PropertyContainer* container) {
		return container->getOOMetaClass().isValidStandardPropertyId(PropertyStorage::GenericSelectionProperty)
			&& container->getOOMetaClass().supportsViewportPicking();
	});

	QGroupBox* mouseSelectionGroup = new QGroupBox(tr("Viewport modes"));
	sublayout = new QVBoxLayout(mouseSelectionGroup);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(6);
	layout->addWidget(mouseSelectionGroup);

	PickElementMode* pickElementMode = new PickElementMode(this);
	connect(this, &QObject::destroyed, pickElementMode, &ViewportInputMode::removeMode);
	ViewportModeAction* pickModeAction = new ViewportModeAction(mainWindow(), tr("Pick"), this, pickElementMode);
	sublayout->addWidget(pickModeAction->createPushButton());

	FenceSelectionMode* fenceMode = new FenceSelectionMode(this);
	connect(this, &QObject::destroyed, fenceMode, &ViewportInputMode::removeMode);
	ViewportModeAction* fenceModeAction = new ViewportModeAction(mainWindow(), tr("Fence selection"), this, fenceMode);
	sublayout->addWidget(fenceModeAction->createPushButton());

	// Deactivate input modes when editor is reset.
	connect(this, &PropertiesEditor::contentsReplaced, pickModeAction, &ViewportModeAction::deactivateMode);
	connect(this, &PropertiesEditor::contentsReplaced, fenceModeAction, &ViewportModeAction::deactivateMode);

	QGroupBox* globalSelectionGroup = new QGroupBox(tr("Actions"));
	sublayout = new QVBoxLayout(globalSelectionGroup);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(6);
	layout->addWidget(globalSelectionGroup);

	QPushButton* selectAllBtn = new QPushButton(tr("Select all"));
	connect(selectAllBtn, &QPushButton::clicked, this, &ManualSelectionModifierEditor::selectAll);
	sublayout->addWidget(selectAllBtn);

	QPushButton* clearSelectionBtn = new QPushButton(tr("Clear selection"));
	connect(clearSelectionBtn, &QPushButton::clicked, this, &ManualSelectionModifierEditor::clearSelection);
	sublayout->addWidget(clearSelectionBtn);

	QPushButton* resetSelectionBtn = new QPushButton(tr("Reset selection"));
	connect(resetSelectionBtn, &QPushButton::clicked, this, &ManualSelectionModifierEditor::resetSelection);
	sublayout->addWidget(resetSelectionBtn);

	// Status label.
	layout->addSpacing(12);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* Adopts the selection state from the modifier's input.
******************************************************************************/
void ManualSelectionModifierEditor::resetSelection()
{
	ManualSelectionModifier* mod = static_object_cast<ManualSelectionModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Reset selection"), [this,mod]() {
		for(ModifierApplication* modApp : modifierApplications()) {
			mod->resetSelection(modApp, modApp->evaluateInputSynchronous());
		}
	});
}

/******************************************************************************
* Selects all elements.
******************************************************************************/
void ManualSelectionModifierEditor::selectAll()
{
	ManualSelectionModifier* mod = static_object_cast<ManualSelectionModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Select all"), [this,mod]() {
		for(ModifierApplication* modApp : modifierApplications()) {
			mod->selectAll(modApp, modApp->evaluateInputSynchronous());
		}
	});
}

/******************************************************************************
* Clears the selection.
******************************************************************************/
void ManualSelectionModifierEditor::clearSelection()
{
	ManualSelectionModifier* mod = static_object_cast<ManualSelectionModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Clear selection"), [this,mod]() {
		for(ModifierApplication* modApp : modifierApplications()) {
			mod->clearSelection(modApp, modApp->evaluateInputSynchronous());
		}
	});
}

/******************************************************************************
* This is called when the user has selected an element.
******************************************************************************/
void ManualSelectionModifierEditor::onElementPicked(const ViewportPickResult& pickResult, size_t elementIndex, const ConstDataObjectPath& pickedObjectPath)
{
	ManualSelectionModifier* mod = static_object_cast<ManualSelectionModifier>(editObject());
	if(!mod || !mod->subject()) return;

	undoableTransaction(tr("Toggle selection"), [this, mod, elementIndex, &pickedObjectPath, &pickResult]() {
		for(ModifierApplication* modApp : modifierApplications()) {

			// Make sure we are in the right data pipeline.
			if(!modApp->pipelines(true).contains(pickResult.pipelineNode()))
				continue;

			// Get the modifier's input data.
			const PipelineFlowState& modInput = modApp->evaluateInputSynchronous();
			const ConstDataObjectPath& inputObjectPath = modInput.expectObject(mod->subject());

			// Look up the right element in the modifier's input.
			// Note that elements may have been added or removed further down the pipeline.
			// Thus, we need to translate the element index into the pipeline output data collection
			// into an index into the modifier's input data collection.
			size_t translatedIndex = mod->subject().dataClass()->remapElementIndex(pickedObjectPath, elementIndex, inputObjectPath);
			if(translatedIndex != std::numeric_limits<size_t>::max()) {
				mod->toggleElementSelection(modApp, modInput, translatedIndex);
				break;
			}
			else {
				mainWindow()->statusBar()->showMessage(tr("Cannot select this element, because it doesn't exist in the modifier's input data."), 2000);
			}
		}
	});
}

/******************************************************************************
* This is called when the user has drawn a fence around particles.
******************************************************************************/
void ManualSelectionModifierEditor::onFence(const QVector<Point2>& fence, Viewport* viewport, ElementSelectionSet::SelectionMode mode)
{
	ManualSelectionModifier* mod = static_object_cast<ManualSelectionModifier>(editObject());
	if(!mod || !mod->subject()) return;

	undoableTransaction(tr("Select"), [this, mod, &fence, viewport, mode]() {
		for(ModifierApplication* modApp : modifierApplications()) {

			// Get the modifier's input data.
			const PipelineFlowState& modInput = modApp->evaluateInputSynchronous();
			const ConstDataObjectPath& inputObjectPath = modInput.expectObject(mod->subject());

			// Iterate of the nodes that use this pipeline.
			// We'll need their object-to-world transformation.
			for(PipelineSceneNode* node : modApp->pipelines(true)) {

				// Set up projection matrix transforming elements from object space to screen space.
				TimeInterval interval;
				const AffineTransformation& nodeTM = node->getWorldTransform(mod->dataset()->animationSettings()->time(), interval);
				Matrix4 ndcToScreen = Matrix4::Identity();
				ndcToScreen(0,0) = 0.5 * viewport->windowSize().width();
				ndcToScreen(1,1) = 0.5 * viewport->windowSize().height();
				ndcToScreen(0,3) = ndcToScreen(0,0);
				ndcToScreen(1,3) = ndcToScreen(1,1);
				ndcToScreen(1,1) = -ndcToScreen(1,1);	// Vertical flip.
				Matrix4 projectionTM = ndcToScreen * viewport->projectionParams().projectionMatrix * (viewport->projectionParams().viewMatrix * nodeTM);

				// Determine which particles are within the closed fence polygon.
				boost::dynamic_bitset<> selection = mod->subject().dataClass()->viewportFenceSelection(fence, inputObjectPath, node, projectionTM);
				if(selection.size() != 0) {
					mod->setSelection(modApp, modInput, selection, mode);
				}
				else {
					mod->throwException(tr("Sorry, making a fence-based selection is not supported for %1.").arg(mod->subject().dataClass()->elementDescriptionName()));
				}
				break;
			}
		}
	});
}

}	// End of namespace
}	// End of namespace
