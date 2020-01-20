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
#include <ovito/core/dataset/DataSetContainer.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/**
 * \brief Manages the DataSet being edited.
 */
class OVITO_GUIWASM_EXPORT WasmDataSetContainer : public DataSetContainer
{
	Q_OBJECT
	OVITO_CLASS(WasmDataSetContainer)

public:

	/// \brief Constructor.
	WasmDataSetContainer(MainWindow* mainWindow);

	/// \brief Returns the window this dataset container is linked to.
	MainWindow* mainWindow() const { return _mainWindow; }

private:

	/// The window this dataset container is linked to.
	MainWindow* _mainWindow;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
