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

#pragma once


#include <ovito/gui/GUI.h>
#include <ovito/gui/dataset/GuiDataSetContainer.h>
#include "VRRenderingWidget.h"

namespace VRPlugin {

using namespace Ovito;

/**
 * \brief A window that renders the scene for VR visualization.
 */
class VRWindow : public QMainWindow
{
	Q_OBJECT

public:

	/// Constructor.
	VRWindow(QWidget* parentWidget, GuiDataSetContainer* datasetContainer);

private:

	/// The OpenGL widget used for rendering.
 	VRRenderingWidget* _glWidget;
};

}	// End of namespace
