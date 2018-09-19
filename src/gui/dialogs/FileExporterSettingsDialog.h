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

#pragma once


#include <gui/GUI.h>
#include <core/dataset/io/FileExporter.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief This dialog box lets the user adjust the settings of a FileExporter.
 */
class OVITO_GUI_EXPORT FileExporterSettingsDialog : public QDialog
{
	Q_OBJECT
	
public:

	/// Constructor.
	FileExporterSettingsDialog(MainWindow* parent, FileExporter* exporter);

	virtual int exec() override {
		// If there is no animation sequence (just a single frame), and if the exporter does not expose any other settings,
		// then it is possible to skip showing the settings dialog altogether.
		if(_skipDialog) return QDialog::Accepted;
		return QDialog::exec();
	}

protected Q_SLOTS:

	/// This is called when the user has pressed the OK button.
	virtual void onOk();

	/// Updates the displayed list of data object available for export.
	void updateDataObjectList();

protected:

	QVBoxLayout* _mainLayout;
	OORef<FileExporter> _exporter;
	SpinnerWidget* _startTimeSpinner;
	SpinnerWidget* _endTimeSpinner;
	SpinnerWidget* _nthFrameSpinner;
	QLineEdit* _wildcardTextbox;
	QButtonGroup* _fileGroupButtonGroup = nullptr;
	QButtonGroup* _rangeButtonGroup;
	QComboBox* _sceneNodeBox;
	QComboBox* _dataObjectBox;
	bool _skipDialog = true;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
