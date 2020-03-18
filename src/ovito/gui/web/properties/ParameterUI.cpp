////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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

#include <ovito/gui/web/GUIWeb.h>
#include <ovito/gui/web/properties/ParameterUI.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/app/PluginManager.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(ParameterUI);
DEFINE_REFERENCE_FIELD(ParameterUI, editObject);

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void ParameterUI::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(editObject)) {
		if(oldTarget) oldTarget->unsetObjectEditingFlag();
		if(newTarget) newTarget->setObjectEditingFlag();
		updatePropertyField();
		updateUI();
		Q_EMIT editObjectReplaced();
	}
	RefMaker::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool ParameterUI::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject() && event.type() == ReferenceEvent::TargetChanged) {
		// The edited object has changed -> update value shown in UI.
		updateUI();
	}

#if 0
	if(isReferenceFieldUI()) {
		if(source == editObject() && event.type() == ReferenceEvent::ReferenceChanged) {
			if(propertyField() == static_cast<const ReferenceFieldEvent&>(event).field()) {
				// The parameter value object stored in the reference field of the edited object
				// has been replaced by another one, so update our own reference to the parameter value object.
				if(editObject()->getReferenceField(*propertyField()) != parameterObject())
					resetUI();
			}
		}
		else if(source == parameterObject() && event.type() == ReferenceEvent::TargetChanged) {
			// The parameter value object has changed -> update value shown in UI.
			updateUI();
		}
	}
	else if(source == editObject() && event.type() == ReferenceEvent::TargetChanged) {
		// The edited object has changed -> update value shown in UI.
		updateUI();
	}
#endif
	return RefMaker::referenceEvent(source, event);
}

/******************************************************************************
* Updates the internal pointer to the RefMaker property or reference field.
******************************************************************************/
void ParameterUI::updatePropertyField()
{
	if(editObject() && !propertyName().isEmpty()) {
		_propertyField = editObject()->getOOMetaClass().findPropertyField(qPrintable(propertyName()), true);
	}
	else {
		_propertyField = nullptr;
	}
}

/******************************************************************************
* Obtains the current value of the parameter from the C++ object.
******************************************************************************/
QVariant ParameterUI::getCurrentValue() const
{
	if(editObject()) {
		if(propertyField()) {
			if(propertyField()->isReferenceField()) {
				RefTarget* target = editObject()->getReferenceField(*propertyField()).getInternal();
				if(Controller* ctrl = dynamic_object_cast<Controller>(target)) {
					switch(ctrl->controllerType()) {
					case Controller::ControllerTypeFloat:
						return QVariant::fromValue(ctrl->currentFloatValue());
					case Controller::ControllerTypeInt:
						return QVariant::fromValue(ctrl->currentIntValue());
					case Controller::ControllerTypeVector3:
						return QVariant::fromValue(QVector3D(ctrl->currentVector3Value()));
					default:
						qWarning() << "ParameterUI::getCurrentValue(): Unsupported animation controller type:" << ctrl->controllerType();
						return {};
					}
				}
				else {
					return QVariant::fromValue(target);
				}
			}
			else {
				QVariant v = editObject()->getPropertyFieldValue(*propertyField());
				if(v.type() == QVariant::Color) {
					QColor c = v.value<QColor>();
					return QVariant::fromValue(QVector3D(c.redF(), c.greenF(), c.blueF()));
				}
				return v;
			}
		}
		else if(!propertyName().isEmpty()) {
			QVariant val = editObject()->property(qPrintable(propertyName()));
			OVITO_ASSERT_MSG(val.isValid(), "ParameterUI::getCurrentValue()", qPrintable(QString("The object class %1 does not define a property with the name %2 that can be cast to QVariant type.").arg(editObject()->metaObject()->className(), propertyName())));
			if(!val.isValid())
				editObject()->throwException(tr("The object class %1 does not define a property with the name %2 that can be cast to QVariant type.").arg(editObject()->metaObject()->className(), propertyName()));
			return val;
		}
	}
	return {};
}

/******************************************************************************
* Changes the current value of the C++ object parameter.
******************************************************************************/
void ParameterUI::setCurrentValue(const QVariant& val)
{
	if(editObject()) {
		UndoableTransaction::handleExceptions(editObject()->dataset()->undoStack(), tr("Change parameter"), [&]() {
			if(propertyField()) {
				if(propertyField()->isReferenceField()) {
					RefTarget* target = editObject()->getReferenceField(*propertyField()).getInternal();
					if(Controller* ctrl = dynamic_object_cast<Controller>(target)) {
						switch(ctrl->controllerType()) {
						case Controller::ControllerTypeFloat:
							ctrl->setCurrentFloatValue(val.toDouble());
							break;
						case Controller::ControllerTypeInt:
							ctrl->setCurrentIntValue(val.toInt());
							break;
						case Controller::ControllerTypeVector3:
							ctrl->setCurrentVector3Value(Vector3(val.value<QVector3D>()));
							break;
						default:
							qWarning() << "ParameterUI::setCurrentValue(): Unsupported animation controller type:" << ctrl->controllerType();
						}
					}
				}
				else {
					if(val.type() == QVariant::Vector3D) {
						editObject()->setPropertyFieldValue(*propertyField(), QVariant::fromValue(Color(val.value<QVector3D>())));
					}
					else {
						editObject()->setPropertyFieldValue(*propertyField(), val);
					}
				}
			}
			else if(!propertyName().isEmpty()) {
				if(!editObject()->setProperty(qPrintable(propertyName()), val)) {
					OVITO_ASSERT_MSG(false, "ParameterUI::setCurrentValue()", qPrintable(QString("The value of property %1 of object class %2 could not be set.").arg(propertyName()).arg(editObject()->metaObject()->className())));
				}
			}
		});
	}
}

/******************************************************************************
* Updates the displayed value in the UI.
******************************************************************************/
void ParameterUI::updateUI()
{
	if(qmlProperty().isValid()) {
		qmlProperty().write(getCurrentValue());
	}
}

/******************************************************************************
* Returns the list of QML components that display the user interface for the current edit object.
******************************************************************************/
QStringList ParameterUI::editorComponentList() const
{
	QStringList componentList;
	if(editObject()) {
		for(OvitoClassPtr clazz = &editObject()->getOOClass(); clazz != nullptr; clazz = clazz->superClass()) {
			QString filePath = QStringLiteral(":/%1/editors/%2.qml").arg(clazz->plugin()->pluginId()).arg(clazz->name());
			if(QFile::exists(filePath)) {
				componentList.push_back(QStringLiteral("qrc") + filePath);
			}
		}
	}
	return componentList;
}

/******************************************************************************
* Returns the list of reference fields of the edit object for which the PROPERTY_FIELD_OPEN_SUBEDITOR flag is set.
******************************************************************************/
QStringList ParameterUI::subobjectFieldList() const
{
	QStringList fieldList;
	if(editObject()) {
		for(auto fieldIter = editObject()->getOOMetaClass().propertyFields().crbegin(); fieldIter != editObject()->getOOMetaClass().propertyFields().crend(); ++fieldIter) {
			const PropertyFieldDescriptor* field = *fieldIter;
			if(!field->isReferenceField()) continue;
			if(field->isVector()) continue;
			if(!field->flags().testFlag(PROPERTY_FIELD_OPEN_SUBEDITOR)) continue;
			fieldList.push_back(field->identifier());
		}
	}
	return fieldList;
}

/******************************************************************************
* Returns the minimum value for the numeric parameter.
******************************************************************************/
FloatType ParameterUI::minParameterValue() const
{
	if(propertyField() && propertyField()->numericalParameterInfo()) {
		return propertyField()->numericalParameterInfo()->minValue;
	}
	return std::numeric_limits<FloatType>::min();
}

/******************************************************************************
* Returns the maximum value for the numeric parameter.
******************************************************************************/
FloatType ParameterUI::maxParameterValue() const
{
	if(propertyField() && propertyField()->numericalParameterInfo()) {
		return propertyField()->numericalParameterInfo()->maxValue;
	}
	return std::numeric_limits<FloatType>::max();
}

/******************************************************************************
* Returns the UI display name of the parameter.
******************************************************************************/
QString ParameterUI::propertyDisplayName() const
{
	if(propertyField()) {
		return propertyField()->displayName();
	}
	return {};
}

}	// End of namespace
