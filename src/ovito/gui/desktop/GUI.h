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

#ifndef __OVITO_GUI_
#define __OVITO_GUI_

#include <ovito/gui/base/GUIBase.h>

/******************************************************************************
* QT Library
******************************************************************************/
#include <QApplication>
#include <QSettings>
#include <QMenuBar>
#include <QMenu>
#include <QResource>
#include <QtWidgets>
#include <QtDebug>
#include <QtGui>
#include <QCommandLineParser>
#include <QNetworkAccessManager>

/******************************************************************************
* Forward declaration of classes.
******************************************************************************/

namespace Ovito {

	class UtilityApplet;
	class GuiAutoStartObject;
	class MainWindow;
	class GuiApplication;
	class ActionManager;
	class DataInspectionApplet;
	class PropertiesPanel;
	class SpinnerWidget;
	class ColorPickerWidget;
	class RolloutContainer;
	class FrameBufferWindow;
	class FrameBufferWidget;
	class AutocompleteTextEdit;
	class AutocompleteLineEdit;
	class ElidedTextLabel;
	class HtmlListWidget;
	class PropertiesEditor;
	class AffineTransformationParameterUI;
	class BooleanParameterUI;
	class BooleanActionParameterUI;
	class BooleanGroupBoxParameterUI;
	class BooleanRadioButtonParameterUI;
	class ColorParameterUI;
	class CustomParameterUI;
	class FilenameParameterUI;
	class FloatParameterUI;
	class FontParameterUI;
	class IntegerRadioButtonParameterUI;
	class IntegerParameterUI;
	class ParameterUI;
	class RefTargetListParameterUI;
	class StringParameterUI;
	class SubObjectParameterUI;
	class VariantComboBoxParameterUI;
	class Vector3ParameterUI;
	class ViewportModeAction;
	class FileExporterSettingsDialog;
	class CoordinateDisplayWidget;
	class CommandPanel;
	class DataInspectorPanel;
	class ModifyCommandPage;
	class RenderCommandPage;
	class OverlayCommandPage;
	class UtilityCommandPage;
	class ViewportMenu;
	class ViewportWindow;
}

#endif // __OVITO_GUI_
