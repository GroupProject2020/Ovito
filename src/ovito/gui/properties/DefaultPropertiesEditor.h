///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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


#include <ovito/gui/GUI.h>
#include <ovito/gui/properties/PropertiesEditor.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

/**
 * \brief The default properties editor used for RefTarget-derived classes if they do not define their own editor type.
 */
class DefaultPropertiesEditor : public PropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(DefaultPropertiesEditor)

public:

	/// Constructor.
	Q_INVOKABLE DefaultPropertiesEditor() = default;

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Is called when the value of a reference field of this RefMaker changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

private:

	/// Rebuilds the list of sub-editors for the current edit object.
	void updateSubEditors();

	/// The editors for the referenced sub-objects.
	std::vector<OORef<PropertiesEditor>> _subEditors;

	/// Specifies where the sub-editors are opened and whether the sub-editors are opened in a collapsed state.
	RolloutInsertionParameters _rolloutParams;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
