///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2018) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <gui/GUI.h>
#include <gui/mainwin/MainWindow.h>
#include "FontSelectionDialog.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* The constructor of the dialog.
******************************************************************************/
FontSelectionDialog::FontSelectionDialog(QWidget* parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Select font"));
	
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
}

/******************************************************************************
* Shows a dialog that lets the pick a font. 
******************************************************************************/
QFont FontSelectionDialog::getFont(bool* ok, QFont currentFont, QWidget* parentWindow)
{
	return QFontDialog::getFont(ok, currentFont, parentWindow);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
