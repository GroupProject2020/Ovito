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
#include <ovito/stdmod/modifiers/ColorCodingModifier.h>

namespace Ovito { namespace StdMod {

/**
 * A properties editor for the ColorCodingModifier class.
 */
class ColorCodingModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(ColorCodingModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE ColorCodingModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private:

	/// Returns an icon representing the given color map class.
	QIcon iconFromColorMapClass(OvitoClassPtr clazz);

	/// Returns an icon representing the given color map.
	QIcon iconFromColorMap(ColorCodingGradient* map);

	/// The list of available color gradients.
	QComboBox* colorGradientList;

	/// Indicates the combo box already contains an item for a custom color map.
	bool _gradientListContainCustomItem;

	/// Label that displays the color gradient picture.
	QLabel* colorLegendLabel;

protected Q_SLOTS:

	/// Updates the display for the color gradient.
	void updateColorGradient();

	/// Is called when the user selects a color gradient in the list box.
	void onColorGradientSelected(int index);

	/// Is called when the user presses the "Adjust range" button.
	void onAdjustRange();

	/// Is called when the user presses the "Adjust range over all frames" button.
	void onAdjustRangeGlobal();

	/// Is called when the user presses the "Reverse range" button.
	void onReverseRange();

	/// Is called when the user presses the "Export color scale" button.
	void onExportColorScale();

protected:

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

private:

	PropertyReferenceParameterUI* _sourcePropertyUI;
};

}	// End of namespace
}	// End of namespace
