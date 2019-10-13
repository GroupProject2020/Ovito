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


#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/import/InputColumnMapping.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import)

/**
 * \brief Dialog box that lets the user edit an InputColumnMapping.
 */
class OVITO_PARTICLESGUI_EXPORT InputColumnMappingDialog : public QDialog
{
	Q_OBJECT

public:

	/// Constructor.
	InputColumnMappingDialog(const InputColumnMapping& mapping, QWidget* parent);

	/// Fills the editor with the given mapping.
	void setMapping(const InputColumnMapping& mapping);

	/// Returns the user-defined column mapping.
	InputColumnMapping mapping() const;

protected Q_SLOTS:

	/// This is called when the user has pressed the OK button.
	void onOk();

	/// Updates the list of vector components for the given file column.
	void updateVectorComponentList(int columnIndex);

	/// Saves the current mapping as a preset.
	void onSavePreset();

	/// Loads a preset mapping.
	void onLoadPreset();

protected:

	/// \brief Returns the string representation of a property's data type.
	static QString dataTypeToString(int dataType);

	/// The main table widget that contains the entries for each data column of the input file.
	QTableWidget* _tableWidget;

	QVector<QCheckBox*> _fileColumnBoxes;
	QVector<QComboBox*> _propertyBoxes;
	QVector<QComboBox*> _vectorComponentBoxes;
	QVector<int> _propertyDataTypes;

	QSignalMapper* _vectorCmpntSignalMapper;

	QLabel* _fileExcerptLabel;
	QTextEdit* _fileExcerptField;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


