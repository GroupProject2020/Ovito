///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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
#include <gui/properties/PropertiesEditor.h>

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


