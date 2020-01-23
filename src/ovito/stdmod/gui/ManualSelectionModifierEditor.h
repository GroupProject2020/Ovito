////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/stdobj/util/ElementSelectionSet.h>
#include <ovito/gui/viewport/input/ViewportInputMode.h>
#include <ovito/gui/desktop/properties/ModifierPropertiesEditor.h>


namespace Ovito { namespace StdMod {
/**
 * A properties editor for the ManualSelectionModifier class.
 */
class ManualSelectionModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(ManualSelectionModifierEditor)

public:

	/// Default constructor
	Q_INVOKABLE ManualSelectionModifierEditor() {}

	/// This is called when the user has selected an element in the viewports.
	void onElementPicked(const ViewportPickResult& pickResult, size_t elementIndex, const ConstDataObjectPath& pickedObjectPath);

	/// This is called when the user has drawn a selection fence around elements.
	void onFence(const QVector<Point2>& fence, Viewport* viewport, ElementSelectionSet::SelectionMode mode);

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Adopts the selection state from the modifier's input.
	void resetSelection();

	/// Selects all particles
	void selectAll();

	/// Clears the selection.
	void clearSelection();
};

}	// End of namespace
}	// End of namespace
