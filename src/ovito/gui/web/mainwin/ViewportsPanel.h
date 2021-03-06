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

namespace Ovito {

/**
 * The container widget for the viewports in OVITO's main window.
 */
class ViewportsPanel : public QQuickItem
{
	Q_OBJECT
	Q_PROPERTY(Ovito::ViewportConfiguration* viewportConfiguration READ viewportConfiguration NOTIFY viewportConfigurationReplaced);

public:

	/// Constructor.
	ViewportsPanel();

	/// Returns the main window this panel is part of.
	MainWindow* mainWindow() const {
		return static_cast<MainWindow*>(parentItem()); 
	}

	/// Arranges the viewport windows within the container.
	void layoutViewports();

	/// Returns the current viewport configuration object.
	ViewportConfiguration* viewportConfiguration() const { return _viewportConfig; }

Q_SIGNALS:

	/// This signal is emitted whenever a new ViewportConfiguration became active.
	void viewportConfigurationReplaced(ViewportConfiguration* configuration);

protected:

	/// Handles resize events for the item.
	virtual void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private Q_SLOTS:

	/// This is called when a new viewport configuration has been loaded.
	void onViewportConfigurationReplaced(ViewportConfiguration* newViewportConfiguration);

	/// This is called when new animation settings have been loaded.
	void onAnimationSettingsReplaced(AnimationSettings* newAnimationSettings);

	/// This is called when the current viewport input mode has changed.
	void onInputModeChanged(ViewportInputMode* oldMode, ViewportInputMode* newMode);

	/// This is called when the mouse cursor of the active input mode has changed.
	void viewportModeCursorChanged(const QCursor& cursor);

	/// Repaints all viewport windows.
	void updateViewportWindows();

private:

	QMetaObject::Connection _activeViewportChangedConnection;
	QMetaObject::Connection _maximizedViewportChangedConnection;
	QMetaObject::Connection _timeChangeCompleteConnection;
	QMetaObject::Connection _activeModeCursorChangedConnection;

	QQmlComponent* _viewportComponent = nullptr;
	OORef<ViewportConfiguration> _viewportConfig;
	OORef<AnimationSettings> _animSettings;
};

}	// End of namespace
