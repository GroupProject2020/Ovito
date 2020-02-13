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

//
// Standard precompiled header file included by all source files in this module
//

#ifndef __OVITO_GUI_BASE_
#define __OVITO_GUI_BASE_

#include <ovito/core/Core.h>

/******************************************************************************
* QT Library
******************************************************************************/
#include <QResource>
#include <QtDebug>
#include <QtGui>

/*! \namespace Ovito::Gui
    \brief This namespace contains the graphical user interface classes.
*/
/*! \namespace Ovito::Gui::Widgets
    \brief This namespace contains the widget classes that can be used in the graphical user interface.
*/
/*! \namespace Ovito::Gui::Params
    \brief This namespace contains GUI classes for parameter editing.
*/
/*! \namespace Ovito::Gui::ViewportInput
    \brief This namespace contains classes for interaction with the viewports.
*/
/*! \namespace Ovito::Gui::Dialogs
    \brief This namespace contains common dialog box classes.
*/

/******************************************************************************
* Forward declaration of classes.
******************************************************************************/
namespace Ovito {

    class MainWindowInterface;
    class PickingSceneRenderer;
    class ViewportSceneRenderer;
    class ViewportInputManager;
    class ViewportInputMode;
    class ViewportGizmo;
}

#endif // __OVITO_GUI_BASE_
