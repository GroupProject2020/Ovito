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


#include <ovito/gui/GUI.h>
#include <ovito/gui/properties/PropertiesEditor.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

/**
 * \brief User interface component for the AttributeFileExporter class.
 */
class AttributeFileExporterEditor : public PropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(AttributeFileExporterEditor)

public:

	/// Constructor.
	Q_INVOKABLE AttributeFileExporterEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

private Q_SLOTS:

	/// Rebuilds the displayed list of exportable attributes.
	void updateAttributesList();

	/// Is called when the user checked/unchecked an item in the attributes list.
	void onAttributeChanged();

private:

	/// Populates the column mapping list box with an entry.
	void insertAttributeItem(const QString& displayName, const QStringList& selectedAttributeList);

	QListWidget* _columnMappingWidget;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


