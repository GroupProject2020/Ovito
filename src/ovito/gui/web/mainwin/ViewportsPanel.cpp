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

#include <ovito/gui/web/GUIWeb.h>
#include <ovito/gui/web/mainwin/MainWindow.h>
#include <ovito/gui/web/viewport/ViewportWindow.h>
#include <ovito/gui/base/viewport/ViewportInputManager.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include "ViewportsPanel.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Constructor.
******************************************************************************/
ViewportsPanel::ViewportsPanel()
{
	// Activate the new viewport layout as soon as a new state file is loaded.
	connect(this, &QQuickItem::windowChanged, this, [this](QQuickWindow* window) {
		if(window) {
			connect(&mainWindow()->datasetContainer(), &DataSetContainer::viewportConfigReplaced, this, &ViewportsPanel::onViewportConfigurationReplaced);
			connect(mainWindow()->viewportInputManager(), &ViewportInputManager::inputModeChanged, this, &ViewportsPanel::onInputModeChanged);
		}
	});
}

/******************************************************************************
* This is called when a new viewport configuration has been loaded.
******************************************************************************/
void ViewportsPanel::onViewportConfigurationReplaced(ViewportConfiguration* newViewportConfiguration)
{
	disconnect(_activeViewportChangedConnection);
	disconnect(_maximizedViewportChangedConnection);

	// Delete all existing viewport windows first.
	for(ViewportWindow* window : findChildren<ViewportWindow*>())
		delete window;

	_viewportConfig = newViewportConfiguration;
	if(newViewportConfiguration) {

		// Create windows for the new viewports.
		try {
			for(Viewport* vp : newViewportConfiguration->viewports()) {
				OVITO_ASSERT(vp->window() == nullptr);
				new ViewportWindow(vp, mainWindow()->viewportInputManager(), mainWindow(), this);
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

		// Layout viewport windows.
		layoutViewports();
	}
}

/******************************************************************************
* This is called when the current viewport input mode has changed.
******************************************************************************/
void ViewportsPanel::onInputModeChanged(ViewportInputMode* oldMode, ViewportInputMode* newMode)
{
	disconnect(_activeModeCursorChangedConnection);
	if(newMode) {
		_activeModeCursorChangedConnection = connect(newMode, &ViewportInputMode::curserChanged, this, &ViewportsPanel::viewportModeCursorChanged);
		viewportModeCursorChanged(newMode->cursor());
	}
	else viewportModeCursorChanged(cursor());
}

/******************************************************************************
* This is called when the mouse cursor of the active input mode has changed.
******************************************************************************/
void ViewportsPanel::viewportModeCursorChanged(const QCursor& cursor)
{
	if(!_viewportConfig) return;

//	for(Viewport* vp : _viewportConfig->viewports()) {
//		if(ViewportWindow* vpWindow = static_cast<ViewportWindow*>(vp->window())) {
//			vpWindow->setCursor(cursor);
//		}
//	}
}

/******************************************************************************
* Handles resize events for the item.
******************************************************************************/
void ViewportsPanel::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry)
{
	QQuickItem::geometryChanged(newGeometry, oldGeometry);
	layoutViewports();
}

/******************************************************************************
* Arranges the viewport windows within the container.
******************************************************************************/
void ViewportsPanel::layoutViewports()
{
	if(!_viewportConfig) return;
	const QVector<Viewport*>& viewports = _viewportConfig->viewports();
	Viewport* maximizedViewport = _viewportConfig->maximizedViewport();

	// Count the number of visible windows.
	int nvisible = 0;
	for(Viewport* viewport : viewports) {
		ViewportWindow* vpwin = static_cast<ViewportWindow*>(viewport->window());
		if(!vpwin) continue;
		if(maximizedViewport == nullptr || maximizedViewport == viewport)
			nvisible++;
		else
			vpwin->setVisible(false);
	}
	if(nvisible == 0) return;

	// Compute number of rows/columns
	int rows = (int)(sqrt((double)nvisible) + 0.5);
	int columns = (nvisible+rows-1) / rows;

	// Get client rect.
	QRectF clientRect(QPointF(0,0), size());

	// Position items.
	int count = 0;
	for(Viewport* viewport : viewports) {
		ViewportWindow* vpwin = static_cast<ViewportWindow*>(viewport->window());
		if(!vpwin) continue;
		if(maximizedViewport != nullptr && maximizedViewport != viewport)
			continue;

		int x = count%columns;
		int y = count/columns;
		QRectF rect(clientRect.topLeft(), QSizeF(0,0));
		rect.translate(clientRect.width() * x / columns, clientRect.height() * y / rows);
		rect.setWidth((clientRect.width() * (x+1) / columns) - rect.x());
		rect.setHeight((clientRect.height() * (y+1) / rows) - rect.y());
		rect.adjust(2,2,-2,-2);

		vpwin->setX(rect.x());
		vpwin->setY(rect.y());
		vpwin->setSize(rect.size());
		vpwin->setVisible(true);
		count++;
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
