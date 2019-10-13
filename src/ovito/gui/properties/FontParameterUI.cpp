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

#include <ovito/gui/GUI.h>
#include <ovito/gui/properties/FontParameterUI.h>
#include <ovito/gui/dialogs/FontSelectionDialog.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/DataSetContainer.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

IMPLEMENT_OVITO_CLASS(FontParameterUI);

/******************************************************************************
* The constructor.
******************************************************************************/
FontParameterUI::FontParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField)
	: PropertyParameterUI(parentEditor, propField)
{
	_label = new QLabel(propField.displayName() + ":");
	_fontPicker = new QPushButton();
	connect(_fontPicker.data(), &QPushButton::clicked, this, &FontParameterUI::onButtonClicked);
}

/******************************************************************************
* Destructor.
******************************************************************************/
FontParameterUI::~FontParameterUI()
{
	// Release GUI controls.
	delete label();
	delete fontPicker();
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void FontParameterUI::resetUI()
{
	PropertyParameterUI::resetUI();

	if(fontPicker())  {
		if(editObject() && (!isReferenceFieldUI() || parameterObject())) {
			fontPicker()->setEnabled(isEnabled());
		}
		else {
			fontPicker()->setEnabled(false);
			fontPicker()->setText(QString());
		}
	}
}

/******************************************************************************
* This method updates the displayed value of the parameter UI.
******************************************************************************/
void FontParameterUI::updateUI()
{
	if(editObject() && fontPicker()) {
		if(isPropertyFieldUI()) {
			QVariant currentValue = editObject()->getPropertyFieldValue(*propertyField());
			OVITO_ASSERT(currentValue.isValid());
			if(currentValue.canConvert<QFont>())
				fontPicker()->setText(currentValue.value<QFont>().family());
			else
				fontPicker()->setText(QString());
		}
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void FontParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	PropertyParameterUI::setEnabled(enabled);
	if(fontPicker()) {
		if(isReferenceFieldUI())
			fontPicker()->setEnabled(parameterObject() != NULL && isEnabled());
		else
			fontPicker()->setEnabled(editObject() != NULL && isEnabled());
	}
}

/******************************************************************************
* Is called when the user has pressed the font picker button.
******************************************************************************/
void FontParameterUI::onButtonClicked()
{
	if(fontPicker() && editObject() && isPropertyFieldUI()) {
		QVariant currentValue = editObject()->getPropertyFieldValue(*propertyField());
		OVITO_ASSERT(currentValue.isValid());
		QFont currentFont;
		if(currentValue.canConvert<QFont>())
			currentFont = currentValue.value<QFont>();
		bool ok;
		QFont font = FontSelectionDialog::getFont(&ok, currentFont, fontPicker()->window());
		if(ok && font != currentFont) {
			undoableTransaction(tr("Change font"), [this, &font]() {
				editObject()->setPropertyFieldValue(*propertyField(), QVariant::fromValue(font));
				Q_EMIT valueEntered();
			});
		}
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
