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


#include <ovito/gui/GUI.h>
#include "ParameterUI.h"
#include "PropertiesEditor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

/******************************************************************************
* This parameter UI will open up a sub-editor for an object that is
* referenced by the edit object.
******************************************************************************/
class OVITO_GUI_EXPORT SubObjectParameterUI : public PropertyParameterUI
{
	Q_OBJECT
	OVITO_CLASS(SubObjectParameterUI)

public:

	/// Constructor.
	SubObjectParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& refField, const RolloutInsertionParameters& rolloutParams = RolloutInsertionParameters());

	/// Destructor.
	virtual ~SubObjectParameterUI() {}

	/// This method is called when a new sub-object has been assigned to the reference field of the editable object
	/// this parameter UI is bound to. It is also called when the editable object itself has
	/// been replaced in the editor.
	virtual void resetUI() override;

	/// Returns the current sub-editor or NULL if there is none.
	PropertiesEditor* subEditor() const { return _subEditor; }

	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;

protected:

	/// The editor for the referenced sub-object.
	OORef<PropertiesEditor> _subEditor;

	/// Controls where the sub-editor is opened and whether the sub-editor is opened in a collapsed state.
	RolloutInsertionParameters _rolloutParams;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


