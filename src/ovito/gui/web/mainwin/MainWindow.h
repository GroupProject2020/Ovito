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
#include <ovito/gui/web/dataset/WasmDataSetContainer.h>
#include <ovito/gui/base/mainwin/MainWindowInterface.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

/**
 * \brief The main window of the application.
 */
class OVITO_GUIWEB_EXPORT MainWindow : public QQuickWindow, public MainWindowInterface
{
	Q_OBJECT

public:

	/// Constructor of the main window class.
	MainWindow();

	/// Destructor.
	virtual ~MainWindow();

	/// Returns the Qt Quick window associated with this MainWindow instance.
	QQuickWindow* quickWindow() { return this; }

	/// Returns the container that keeps a reference to the current dataset.
	WasmDataSetContainer& datasetContainer() { return _datasetContainer; }

private:

	/// Container managing the current dataset.
	WasmDataSetContainer _datasetContainer;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
