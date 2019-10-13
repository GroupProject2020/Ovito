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
#include "FloatParameterUI.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

/******************************************************************************
* A parameter UI for Vector3 properties.
* This ParameterUI lets the user edit one of the X, Y and Z components of the vector.
******************************************************************************/
class OVITO_GUI_EXPORT Vector3ParameterUI : public FloatParameterUI
{
	Q_OBJECT
	OVITO_CLASS(Vector3ParameterUI)

public:

	/// Constructor for a Qt property.
	Vector3ParameterUI(QObject* parentEditor, const char* propertyName, size_t vectorComponent, const QString& labelText = QString(), const QMetaObject* parameterUnitType = nullptr);

	/// Constructor for a PropertyField or ReferenceField property.
	Vector3ParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField, size_t vectorComponent);

	/// This method updates the displayed value of the parameter UI.
	virtual void updateUI() override;

	/// Takes the value entered by the user and stores it in the property field
	/// this property UI is bound to.
	virtual void updatePropertyValue() override;

private:

	/// The vector component to control (0 - 2).
	size_t _component;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


