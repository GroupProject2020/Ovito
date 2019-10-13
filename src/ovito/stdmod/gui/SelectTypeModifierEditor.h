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
