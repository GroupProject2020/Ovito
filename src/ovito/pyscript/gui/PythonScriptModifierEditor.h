///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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


#include <ovito/pyscript/PyScript.h>
#include <ovito/gui/properties/ModifierPropertiesEditor.h>

namespace PyScript { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace Ovito;

/**
 * \brief A properties editor for the PythonScriptModifier class.
 */
class PythonScriptModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(PythonScriptModifierEditor)

public:

	/// Constructor.
	Q_INVOKABLE PythonScriptModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Is called when the current edit object has generated a change
	/// event or if a new object has been loaded into editor.
	void updateUserInterface();

	/// Is called when the user presses the 'Edit script' button.
	void onOpenEditor();

private:

	QPushButton* _editScriptButton;
	QTextEdit* _outputDisplay;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


