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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/properties/ColorParameterUI.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

IMPLEMENT_OVITO_CLASS(ColorParameterUI);

/******************************************************************************
* The constructor.
******************************************************************************/
ColorParameterUI::ColorParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField)
	: PropertyParameterUI(parentEditor, propField)
{
	_label = new QLabel(propField.displayName() + ":");
	_colorPicker = new ColorPickerWidget();
	_colorPicker->setObjectName("colorButton");
	connect(_colorPicker.data(), &ColorPickerWidget::colorChanged, this, &ColorParameterUI::onColorPickerChanged);
}

/******************************************************************************
* Destructor.
******************************************************************************/
ColorParameterUI::~ColorParameterUI()
{
	// Release GUI controls.
	delete label();
	delete colorPicker();
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void ColorParameterUI::resetUI()
{
	PropertyParameterUI::resetUI();

	if(colorPicker())  {
		if(editObject() && (!isReferenceFieldUI() || parameterObject())) {
			colorPicker()->setEnabled(isEnabled());
		}
		else {
			colorPicker()->setEnabled(false);
			colorPicker()->setColor(Color(1,1,1));
		}
	}

	if(isReferenceFieldUI() && editObject()) {
		// Update the displayed value when the animation time has changed.
		connect(dataset()->container(), &DataSetContainer::timeChanged, this, &ColorParameterUI::updateUI, Qt::UniqueConnection);
	}
}

/******************************************************************************
* This method updates the displayed value of the parameter UI.
******************************************************************************/
void ColorParameterUI::updateUI()
{
	if(editObject() && colorPicker()) {
		if(isReferenceFieldUI()) {
			Controller* ctrl = dynamic_object_cast<Controller>(parameterObject());
			if(ctrl)
				colorPicker()->setColor(ctrl->currentColorValue());
		}
		else if(isPropertyFieldUI()) {
			QVariant currentValue = editObject()->getPropertyFieldValue(*propertyField());
			OVITO_ASSERT(currentValue.isValid());
			if(currentValue.canConvert<Color>()) {
				colorPicker()->setColor(currentValue.value<Color>());
			}
			else if(currentValue.canConvert<QColor>()) {
				colorPicker()->setColor(currentValue.value<QColor>());
			}
		}
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void ColorParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	PropertyParameterUI::setEnabled(enabled);
	if(colorPicker()) {
		if(isReferenceFieldUI())
			colorPicker()->setEnabled(parameterObject() != NULL && isEnabled());
		else
			colorPicker()->setEnabled(editObject() != NULL && isEnabled());
	}
}

/******************************************************************************
* Is called when the user has changed the color.
******************************************************************************/
void ColorParameterUI::onColorPickerChanged()
{
	if(colorPicker() && editObject()) {
		undoableTransaction(tr("Change color"), [this]() {
			if(isReferenceFieldUI()) {
				if(Controller* ctrl = dynamic_object_cast<Controller>(parameterObject()))
					ctrl->setCurrentColorValue(colorPicker()->color());
			}
			else if(isPropertyFieldUI()) {
				editObject()->setPropertyFieldValue(*propertyField(), QVariant::fromValue((QColor)colorPicker()->color()));
			}
			Q_EMIT valueEntered();
		});
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
