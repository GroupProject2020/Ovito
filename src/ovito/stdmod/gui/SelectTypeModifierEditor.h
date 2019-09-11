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


#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/gui/properties/ModifierPropertiesEditor.h>
#include <ovito/stdobj/gui/widgets/PropertyReferenceParameterUI.h>

namespace Ovito { namespace StdMod {

/**
 * A properties editor for the SelectTypeModifier class.
 */
class SelectTypeModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(SelectTypeModifierEditor)

public:

	/// Default constructor
	Q_INVOKABLE SelectTypeModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// Determines if the given property is a valid input property for the Select Type modifier.
	static bool isValidInputProperty(const PropertyObject* property) {
		return property->elementTypes().empty() == false && property->componentCount() == 1 && property->dataType() == PropertyStorage::Int;
	}

protected Q_SLOTS:

	/// Updates the contents of the element type list box.
	void updateElementTypeList();

	/// This is called when the user has selected another type.
	void onElementTypeSelected(QListWidgetItem* item);

private:

	// Selection box for the input property.
	PropertyReferenceParameterUI* _sourcePropertyUI;

	/// The list of selectable element types.
	QListWidget* _elementTypesBox;
};

}	// End of namespace
}	// End of namespace
