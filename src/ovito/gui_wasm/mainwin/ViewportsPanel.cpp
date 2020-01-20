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

#include <ovito/gui_wasm/GUI.h>
#include <ovito/gui_wasm/mainwin/MainWindow.h>
#include <ovito/gui/viewport/ViewportWindow.h>
#include <ovito/core/viewport/ViewportSettings.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include "ViewportsPanel.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* The constructor of the viewports container class.
******************************************************************************/
ViewportsPanel::ViewportsPanel(MainWindow* parent) : QObject(parent)
{
	// Activate the new viewport layout as soon as a new state file is loaded.
	connect(&parent->datasetContainer(), &DataSetContainer::viewportConfigReplaced, this, &ViewportsPanel::onViewportConfigurationReplaced);
	connect(&parent->datasetContainer(), &DataSetContainer::animationSettingsReplaced, this, &ViewportsPanel::onAnimationSettingsReplaced);
}

/******************************************************************************
* Returns the widget that is associated with the given viewport.
******************************************************************************/
QWidget* ViewportsPanel::viewportWidget(Viewport* vp)
{
	return static_cast<ViewportWindow*>(vp->window());
}

/******************************************************************************
* This is called when a new viewport configuration has been loaded.
******************************************************************************/
void ViewportsPanel::onViewportConfigurationReplaced(ViewportConfiguration* newViewportConfiguration)
{
	disconnect(_activeViewportChangedConnection);
	disconnect(_maximizedViewportChangedConnection);

	// Delete all existing viewport widgets first.
	for(QWidget* widget : findChildren<QWidget*>())
		delete widget;

	_viewportConfig = newViewportConfiguration;

	if(newViewportConfiguration) {

		// Create windows for the new viewports.
		try {
			ViewportInputManager* inputManager = MainWindow::fromDataset(newViewportConfiguration->dataset())->viewportInputManager();
			for(Viewport* vp : newViewportConfiguration->viewports()) {
				OVITO_ASSERT(vp->window() == nullptr);
				ViewportWindow* viewportWindow = new ViewportWindow(vp, inputManager, this);
			}
		}
		catch(const Exception& ex) {
			ex.reportError(true);
			QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
		}

		// Repaint the viewport borders when another viewport has been activated.
		_activeViewportChangedConnection = connect(newViewportConfiguration, &ViewportConfiguration::activeViewportChanged, this, (void (ViewportsPanel::*)())&ViewportsPanel::update);

		// Update layout when a viewport has been maximized.
		_maximizedViewportChangedConnection = connect(newViewportConfiguration, &ViewportConfiguration::maximizedViewportChanged, this, &ViewportsPanel::layoutViewports);
	}
}

/******************************************************************************
* This is called when new animation settings have been loaded.
******************************************************************************/
void ViewportsPanel::onAnimationSettingsReplaced(AnimationSettings* newAnimationSettings)
{
	disconnect(_autoKeyModeChangedConnection);
	_animSettings = newAnimationSettings;

	if(newAnimationSettings) {
		_autoKeyModeChangedConnection = connect(newAnimationSettings, &AnimationSettings::autoKeyModeChanged, this, (void (ViewportsPanel::*)())&ViewportsPanel::update);
		_timeChangeCompleteConnection = connect(newAnimationSettings, &AnimationSettings::timeChangeComplete, this, (void (ViewportsPanel::*)())&ViewportsPanel::update);
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
