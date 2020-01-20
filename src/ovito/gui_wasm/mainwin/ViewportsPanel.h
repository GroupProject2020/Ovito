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


#include <ovito/gui_wasm/GUI.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * The container for the viewports windows.
 */
class ViewportsPanel : public QObject
{
	Q_OBJECT

public:

	/// \brief Constructs the viewport panel.
	ViewportsPanel(MainWindow* parent);

	/// \brief Returns the window that is associated with the given OVITO viewport.
	static QQuickWindow* viewportWindow(Viewport* vp);

private Q_SLOTS:

	/// This is called when a new viewport configuration has been loaded.
	void onViewportConfigurationReplaced(ViewportConfiguration* newViewportConfiguration);

	/// This is called when new animation settings have been loaded.
	void onAnimationSettingsReplaced(AnimationSettings* newAnimationSettings);

private:

	QMetaObject::Connection _activeViewportChangedConnection;
	QMetaObject::Connection _maximizedViewportChangedConnection;
	QMetaObject::Connection _autoKeyModeChangedConnection;
	QMetaObject::Connection _timeChangeCompleteConnection;

	OORef<ViewportConfiguration> _viewportConfig;
	OORef<AnimationSettings> _animSettings;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
