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
#include <gui/properties/ModifierPropertiesEditor.h>
#include <gui/properties/PropertyReferenceParameterUI.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene) OVITO_BEGIN_INLINE_NAMESPACE(StdObj) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

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

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
