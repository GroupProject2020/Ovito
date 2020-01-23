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
#include <ovito/core/rendering/FrameBuffer.h>
#include "HistoryFileDialog.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Dialogs)

/**
 * \brief This file chooser dialog lets the user select an image file for output.
 */
class OVITO_GUI_EXPORT SaveImageFileDialog : public HistoryFileDialog
{
	Q_OBJECT

public:

	/// \brief Constructs the dialog window.
	SaveImageFileDialog(QWidget* parent = nullptr, const QString& caption = QString(), bool includeVideoFormats = false, const ImageInfo& imageInfo = ImageInfo());

	/// \brief Returns the file info after the dialog has been closed with "OK".
	const ImageInfo& imageInfo() const { return _imageInfo; }

private Q_SLOTS:

	/// This is called when the user has pressed the OK button of the dialog box.
	void onFileSelected(const QString& file);

	/// This is called when the user has selected a file format.
	void onFilterSelected(const QString& filter);

private:

	QList<QByteArray> _formatList;
	ImageInfo _imageInfo;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


