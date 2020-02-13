////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/core/dataset/io/FileExporter.h>

namespace Ovito {

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

}	// End of namespace
