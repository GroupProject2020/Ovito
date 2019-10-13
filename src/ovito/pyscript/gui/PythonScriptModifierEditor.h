////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Alexander Stukowski
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


