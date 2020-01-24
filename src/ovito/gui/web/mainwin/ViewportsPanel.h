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

#pragma once


#include <ovito/gui/web/GUIWeb.h>
#include <ovito/core/viewport/ViewportConfiguration.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * The container widget for the viewports in OVITO's main window.
 */
class ViewportsPanel : public QQuickItem
{
	Q_OBJECT

public:

	/// Constructor.
	ViewportsPanel();

	/// Returns the main window this panel is part of.
	MainWindow* mainWindow() const { return static_cast<MainWindow*>(window()); }

	/// Arranges the viewport windows within the container.
	void layoutViewports();

protected:

	/// Handles resize events for the item.
	virtual void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private Q_SLOTS:

	/// This is called when a new viewport configuration has been loaded.
	void onViewportConfigurationReplaced(ViewportConfiguration* newViewportConfiguration);

	/// This is called when the current viewport input mode has changed.
	void onInputModeChanged(ViewportInputMode* oldMode, ViewportInputMode* newMode);

	/// This is called when the mouse cursor of the active input mode has changed.
	void viewportModeCursorChanged(const QCursor& cursor);

private:

	QMetaObject::Connection _activeViewportChangedConnection;
	QMetaObject::Connection _maximizedViewportChangedConnection;
	QMetaObject::Connection _activeModeCursorChangedConnection;

	OORef<ViewportConfiguration> _viewportConfig;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace