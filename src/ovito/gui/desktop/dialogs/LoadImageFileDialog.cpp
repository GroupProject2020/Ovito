////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Alexander Stukowski
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

#include <ovito/gui/desktop/GUI.h>
#include "LoadImageFileDialog.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Dialogs)

/******************************************************************************
* Constructs the dialog window.
******************************************************************************/
LoadImageFileDialog::LoadImageFileDialog(QWidget* parent, const QString& caption, const ImageInfo& imageInfo) :
	HistoryFileDialog("load_image", parent, caption), _imageInfo(imageInfo)
{
	connect(this, &QFileDialog::fileSelected, this, &LoadImageFileDialog::onFileSelected);
	setAcceptMode(QFileDialog::AcceptOpen);
	setNameFilter(tr("Image files (*.png *.jpg *.jpeg)"));
	if(_imageInfo.filename().isEmpty() == false)
		selectFile(_imageInfo.filename());
}

/******************************************************************************
* This is called when the user has pressed the OK button of the dialog.
******************************************************************************/
void LoadImageFileDialog::onFileSelected(const QString& file)
{
	_imageInfo.setFilename(file);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
