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

#include <ovito/gui/GUI.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include <ovito/gui/properties/BooleanGroupBoxParameterUI.h>
#include <ovito/gui/properties/IntegerParameterUI.h>
#include <ovito/gui/properties/FloatParameterUI.h>
#include <ovito/gui/properties/SubObjectParameterUI.h>
#include <ovito/gui/viewport/input/ViewportInputManager.h>
#include <ovito/gui/viewport/input/ViewportInputMode.h>
#include <ovito/gui/viewport/ViewportWindow.h>
#include <ovito/gui/actions/ViewportModeAction.h>
#include <ovito/gui/mainwin/MainWindow.h>
#include <ovito/gui/widgets/general/HtmlListWidget.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/ospray/renderer/OSPRayRenderer.h>
#include "OSPRayRendererEditor.h"

namespace Ovito { namespace OSPRay { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(OSPRayRendererEditor);
SET_OVITO_OBJECT_EDITOR(OSPRayRenderer, OSPRayRendererEditor);

/**
 * Viewport input mode that allows to pick a focal length.
 */
class PickFocalLengthInputMode : public ViewportInputMode
{
public:

	/// Constructor.
	PickFocalLengthInputMode(OSPRayRendererEditor* editor) : ViewportInputMode(editor), _editor(editor) {}

	/// Handles the mouse move events for a Viewport.
	virtual void mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event) override {

		// Change mouse cursor while hovering over an object.
		setCursor(vpwin->pick(event->localPos()).isValid() ? SelectionMode::selectionCursor() : QCursor());

		ViewportInputMode::mouseMoveEvent(vpwin, event);
	}

	/// Handles the mouse up events for a Viewport.
	virtual void mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event) override {
		if(event->button() == Qt::LeftButton) {
			ViewportPickResult pickResult = vpwin->pick(event->localPos());
			if(pickResult.isValid() && vpwin->viewport()->isPerspectiveProjection()) {
				FloatType distance = (pickResult.hitLocation() - vpwin->viewport()->cameraPosition()).length();

				if(OSPRayRenderer* renderer = static_object_cast<OSPRayRenderer>(_editor->editObject())) {
					_editor->undoableTransaction(tr("Set focal length"), [renderer, distance]() {
						renderer->setDofFocalLength(distance);
					});
				}
			}
			inputManager()->removeInputMode(this);
		}
		ViewportInputMode::mouseReleaseEvent(vpwin, event);
	}

protected:

	/// This is called by the system when the input handler has become active.
	virtual void activated(bool temporary) override {
		ViewportInputMode::activated(temporary);
		inputManager()->mainWindow()->statusBar()->showMessage(
				tr("Click on an object in the viewport to set the camera's focal length."));
	}

	/// This is called by the system after the input handler is no longer the active handler.
	virtual void deactivated(bool temporary) override {
		inputManager()->mainWindow()->statusBar()->clearMessage();
		ViewportInputMode::deactivated(temporary);
	}

private:

	OSPRayRendererEditor* _editor;
};

/******************************************************************************
* Creates the UI controls for the editor.
******************************************************************************/
void OSPRayRendererEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("OSPRay settings"), rolloutParams, "rendering.ospray_renderer.html");

	QVBoxLayout* mainLayout = new QVBoxLayout(rollout);
	mainLayout->setContentsMargins(4,4,4,4);

	// Quality
	QGroupBox* qualityGroupBox = new QGroupBox(tr("Quality"));
	mainLayout->addWidget(qualityGroupBox);

	QGridLayout* layout = new QGridLayout(qualityGroupBox);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	IntegerParameterUI* refinementIterationsUI = new IntegerParameterUI(this, PROPERTY_FIELD(OSPRayRenderer::refinementIterations));
	layout->addWidget(refinementIterationsUI->label(), 0, 0);
	layout->addLayout(refinementIterationsUI->createFieldLayout(), 0, 1);

	IntegerParameterUI* aaSamplesUI = new IntegerParameterUI(this, PROPERTY_FIELD(OSPRayRenderer::samplesPerPixel));
	layout->addWidget(aaSamplesUI->label(), 1, 0);
	layout->addLayout(aaSamplesUI->createFieldLayout(), 1, 1);

	IntegerParameterUI* maxRayRecursionUI = new IntegerParameterUI(this, PROPERTY_FIELD(OSPRayRenderer::maxRayRecursion));
	layout->addWidget(maxRayRecursionUI->label(), 2, 0);
	layout->addLayout(maxRayRecursionUI->createFieldLayout(), 2, 1);

	BooleanGroupBoxParameterUI* enableDirectLightUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(OSPRayRenderer::directLightSourceEnabled));
	QGroupBox* directLightsGroupBox = enableDirectLightUI->groupBox();
	mainLayout->addWidget(directLightsGroupBox);

	layout = new QGridLayout(enableDirectLightUI->childContainer());
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	// Direct light brightness.
	FloatParameterUI* defaultLightIntensityUI = new FloatParameterUI(this, PROPERTY_FIELD(OSPRayRenderer::defaultLightSourceIntensity));
	defaultLightIntensityUI->label()->setText(tr("Brightness:"));
	layout->addWidget(defaultLightIntensityUI->label(), 0, 0);
	layout->addLayout(defaultLightIntensityUI->createFieldLayout(), 0, 1);

	// Angular diameter.
	FloatParameterUI* defaultLightSourceAngularDiameterUI = new FloatParameterUI(this, PROPERTY_FIELD(OSPRayRenderer::defaultLightSourceAngularDiameter));
	layout->addWidget(defaultLightSourceAngularDiameterUI->label(), 1, 0);
	layout->addLayout(defaultLightSourceAngularDiameterUI->createFieldLayout(), 1, 1);

	BooleanGroupBoxParameterUI* enableAmbientLightUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(OSPRayRenderer::ambientLightEnabled));
	QGroupBox* ambientLightsGroupBox = enableAmbientLightUI->groupBox();
	mainLayout->addWidget(ambientLightsGroupBox);

	layout = new QGridLayout(enableAmbientLightUI->childContainer());
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	// Ambient brightness.
	FloatParameterUI* ambientBrightnessUI = new FloatParameterUI(this, PROPERTY_FIELD(OSPRayRenderer::ambientBrightness));
	ambientBrightnessUI->label()->setText(tr("Brightness:"));
	layout->addWidget(ambientBrightnessUI->label(), 0, 0);
	layout->addLayout(ambientBrightnessUI->createFieldLayout(), 0, 1);

	// Material
	QGroupBox* materialGroupBox = new QGroupBox(tr("Material"));
	mainLayout->addWidget(materialGroupBox);

	layout = new QGridLayout(materialGroupBox);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	FloatParameterUI* matSpecularUI = new FloatParameterUI(this, PROPERTY_FIELD(OSPRayRenderer::materialSpecularBrightness));
	layout->addWidget(matSpecularUI->label(), 0, 0);
	layout->addLayout(matSpecularUI->createFieldLayout(), 0, 1);

	FloatParameterUI* matShininessUI = new FloatParameterUI(this, PROPERTY_FIELD(OSPRayRenderer::materialShininess));
	layout->addWidget(matShininessUI->label(), 1, 0);
	layout->addLayout(matShininessUI->createFieldLayout(), 1, 1);

	// Depth of field
	BooleanGroupBoxParameterUI* enableDepthOfFieldUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(OSPRayRenderer::depthOfFieldEnabled));
	QGroupBox* dofGroupBox = enableDepthOfFieldUI->groupBox();
	mainLayout->addWidget(dofGroupBox);

	layout = new QGridLayout(enableDepthOfFieldUI->childContainer());
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	// Focal length
	FloatParameterUI* focalLengthUI = new FloatParameterUI(this, PROPERTY_FIELD(OSPRayRenderer::dofFocalLength));
	layout->addWidget(focalLengthUI->label(), 0, 0);
	layout->addLayout(focalLengthUI->createFieldLayout(), 0, 1);

	// Focal length picking mode.
	ViewportInputMode* pickFocalLengthMode = new PickFocalLengthInputMode(this);
	connect(this, &QObject::destroyed, pickFocalLengthMode, &ViewportInputMode::removeMode);
	ViewportModeAction* modeAction = new ViewportModeAction(mainWindow(), tr("Pick in viewport"), this, pickFocalLengthMode);
	layout->addWidget(modeAction->createPushButton(), 0, 2);

	// Aperture
	FloatParameterUI* apertureUI = new FloatParameterUI(this, PROPERTY_FIELD(OSPRayRenderer::dofAperture));
	layout->addWidget(apertureUI->label(), 1, 0);
	layout->addLayout(apertureUI->createFieldLayout(), 1, 1, 1, 2);

	// 'Switch backend' button.
	QPushButton* switchBackendButton = new QPushButton(tr("Switch OSPRay backend..."));
	connect(switchBackendButton, &QPushButton::clicked, this, &OSPRayRendererEditor::onSwitchBackend);
	mainLayout->addWidget(switchBackendButton);

	// Open a sub-editor for the backend.
	new SubObjectParameterUI(this, PROPERTY_FIELD(OSPRayRenderer::backend), rolloutParams.after(rollout));
}

/******************************************************************************
* Lets the user choose a different OSPRay engine.
******************************************************************************/
void OSPRayRendererEditor::onSwitchBackend()
{
	OSPRayRenderer* renderer = static_object_cast<OSPRayRenderer>(editObject());
	if(!renderer) return;

	// Create list of available backends.
	QVector<OvitoClassPtr> backendClasses = PluginManager::instance().listClasses(OSPRayBackend::OOClass());
	int current = -1;
	int index = 0;
	QStringList items;
	for(const OvitoClassPtr& clazz : backendClasses) {
		items << clazz->displayName();
		if(renderer->backend() && &renderer->backend()->getOOClass() == clazz)
			current = index;
		++index;
	}

	// Let user choose a new backend.
	bool ok;
	QString item = QInputDialog::getItem(container(), tr("Switch OSPRay backend"),
		tr("Select an OSPRay rendering backend."), items, current, false, &ok);
	if(!ok) return;

	int newIndex = items.indexOf(item);
	if(newIndex >= 0 && newIndex < backendClasses.size()) {
		if(!renderer->backend() || &renderer->backend()->getOOClass() != backendClasses[newIndex]) {
			undoableTransaction(tr("Switch backend"), [renderer, newIndex, &backendClasses]() {
				OORef<OSPRayBackend> backend = static_object_cast<OSPRayBackend>(backendClasses[newIndex]->createInstance(renderer->dataset()));
				backend->loadUserDefaults();
				renderer->setBackend(backend);
			});
		}
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
