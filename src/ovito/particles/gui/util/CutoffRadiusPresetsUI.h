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


#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/gui/properties/ParameterUI.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

class OVITO_PARTICLESGUI_EXPORT ChemicalElement
{
public:

	enum CrystalStructure {
		Unknown, SimpleCubic, FaceCenteredCubic, BodyCenteredCubic,
		HexagonalClosePacked, Tetragonal, Diatom, Diamond, Orthorhombic,
		Cubic, Monoclinic, Atom, Rhombohedral
	};

	CrystalStructure structure;
	FloatType latticeParameter;

	const char* elementName;
};

extern ChemicalElement ChemicalElements[];
extern const size_t NumberOfChemicalElements;

class OVITO_PARTICLESGUI_EXPORT CutoffRadiusPresetsUI : public PropertyParameterUI
{
	Q_OBJECT
	OVITO_CLASS(CutoffRadiusPresetsUI)

public:

	/// Constructor for a PropertyField property.
	CutoffRadiusPresetsUI(PropertiesEditor* parentEditor, const PropertyFieldDescriptor& propField);

	/// Destructor.
	virtual ~CutoffRadiusPresetsUI();

	/// This returns the QComboBox managed by this ParameterUI.
	QComboBox* comboBox() const { return _comboBox; }

	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;

	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.
	virtual void resetUI() override;

public:

	Q_PROPERTY(QComboBox comboBox READ comboBox);

protected Q_SLOTS:

	/// Is called when the user has selected an item in the cutoff presets box.
	void onSelect(int index);

protected:

	/// The combo box control of the UI component.
	QPointer<QComboBox> _comboBox;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
