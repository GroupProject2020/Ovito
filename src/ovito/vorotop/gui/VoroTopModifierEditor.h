////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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


#include <ovito/vorotop/VoroTopPlugin.h>
#include <ovito/vorotop/VoroTopModifier.h>
#include <ovito/gui/properties/ModifierPropertiesEditor.h>

namespace Ovito { namespace VoroTop {

/**
 * A properties editor for the VoroTopModifier class.
 */
class VoroTopModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(VoroTopModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE VoroTopModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private Q_SLOTS:

	/// Is called when the user presses the 'Load filter' button.
	void onLoadFilter();
};

}	// End of namespace
}	// End of namespace
