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
* A parameter UI for AffineTransformation type properties.
* This ParameterUI lets the user edit the individual matrix components.
******************************************************************************/
class OVITO_GUI_EXPORT AffineTransformationParameterUI : public FloatParameterUI
{
	Q_OBJECT
	OVITO_CLASS(AffineTransformationParameterUI)

public:

	/// Constructor for a Qt property.
	AffineTransformationParameterUI(QObject* parentEditor, const char* propertyName, size_t row, size_t column, const QString& labelText = QString(), const QMetaObject* parameterUnitType = nullptr);

	/// Constructor for a PropertyField property.
	AffineTransformationParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField, size_t row, size_t column);

	/// This method updates the displayed value of the parameter UI.
	virtual void updateUI() override;

	/// Takes the value entered by the user and stores it in the property field
	/// this property UI is bound to.
	virtual void updatePropertyValue() override;

private:

	/// The matrix component to control.
	size_t row, column;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


