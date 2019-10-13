////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Alexander Stukowski
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
#include <ovito/gui/properties/PropertiesEditor.h>

namespace PyScript { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace Ovito;

/**
 * \brief A properties editor for the PythonViewportOverlay class.
 */
class PythonViewportOverlayEditor : public PropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(PythonViewportOverlayEditor)

public:

	/// Constructor.
	Q_INVOKABLE PythonViewportOverlayEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

protected Q_SLOTS:

	/// Is called when the current edit object has generated a change
	/// event or if a new object has been loaded into editor.
	void onContentsChanged(RefTarget* editObject);

	/// Is called when the user presses the 'Edit script' button.
	void onOpenEditor();

private:

	QPushButton* _editScriptButton;
	QTextEdit* _outputDisplay;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


