////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/properties/PropertiesPanel.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * The command panel page lets user render the scene.
 */
class OVITO_GUI_EXPORT RenderCommandPage : public QWidget
{
	Q_OBJECT

public:

	/// Initializes the render page.
    RenderCommandPage(MainWindow* mainWindow, QWidget* parent);

private Q_SLOTS:

	/// This is called when a new dataset has been loaded.
	void onDataSetChanged(DataSet* newDataSet);

	/// This is called when new render settings have been loaded.
	void onRenderSettingsReplaced(RenderSettings* newRenderSettings);

private:

	/// This panel shows the properties of the render settings object.
	PropertiesPanel* propertiesPanel;

	QMetaObject::Connection _renderSettingsReplacedConnection;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


