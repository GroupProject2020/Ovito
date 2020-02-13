////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
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
#include <ovito/gui/desktop/properties/PropertiesEditor.h>

namespace Ovito {

/**
 * \brief Abstract base class for properties editors for FileImporter derived classes.
 */
class OVITO_GUI_EXPORT FileImporterEditor : public PropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(FileImporterEditor)

public:

	/// Constructor.
	FileImporterEditor() {}

	/// This is called by the system when the user has selected a new file to import.
	virtual bool inspectNewFile(FileImporter* importer, const QUrl& sourceFile, QWidget* parent) { return true; }
};

}	// End of namespace


