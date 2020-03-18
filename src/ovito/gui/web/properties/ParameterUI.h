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

#pragma once


#include <ovito/gui/web/GUIWeb.h>
#include <ovito/core/oo/RefMaker.h>
#include <ovito/core/oo/RefTarget.h>

namespace Ovito {

/**
 * \brief QML parameter modifier for exposing C++ object parameters.
 */
class ParameterUI : public RefMaker, public QQmlPropertyValueSource
{
	Q_OBJECT
	Q_INTERFACES(QQmlPropertyValueSource)
    Q_PROPERTY(Ovito::RefTarget* editObject READ editObject WRITE setEditObject NOTIFY editObjectReplaced)
	Q_PROPERTY(QString propertyName READ propertyName WRITE setPropertyName);
	Q_PROPERTY(QVariant propertyValue READ getCurrentValue WRITE setCurrentValue);
	Q_PROPERTY(QStringList editorComponentList READ editorComponentList NOTIFY editObjectReplaced);
	Q_PROPERTY(QStringList subobjectFieldList READ subobjectFieldList NOTIFY editObjectReplaced);
	Q_PROPERTY(FloatType minParameterValue READ minParameterValue NOTIFY editObjectReplaced);
	Q_PROPERTY(FloatType maxParameterValue READ maxParameterValue NOTIFY editObjectReplaced);
	Q_PROPERTY(QString propertyDisplayName READ propertyDisplayName NOTIFY editObjectReplaced);
	OVITO_CLASS(ParameterUI)

public:

	/// \brief Constructor.
	ParameterUI() = default;

	/// \brief Destructor.
	virtual ~ParameterUI() { clearAllReferences(); }

	/// Returns the name of the object property exposed by this ParameterUI instance.
	const QString& propertyName() const { return _propertyName; }

	/// Sets the name of the object property exposed by this ParameterUI instance.
	void setPropertyName(const QString& name) { 
		if(name != _propertyName) {
			_propertyName = name;
			updatePropertyField();
		}
	}

	/// Returns the C++ RefMaker property or reference field that is exposed by this QML property source.
	const PropertyFieldDescriptor* propertyField() const { return _propertyField; }

	/// Set the target property for the QML value source. 
	/// This method will be called by the QML engine when assigning a value source. 
	virtual void setTarget(const QQmlProperty& prop) override { _qmlProperty = prop; }

	/// Updates the displayed value in the UI.
	virtual void updateUI();

	/// Returns the QML property this value source is attached to.
	QQmlProperty& qmlProperty() { return _qmlProperty; }

	/// Obtains the current value of the parameter from the C++ object.
	virtual QVariant getCurrentValue() const;

	/// Changes the current value of the C++ object parameter.
	virtual void setCurrentValue(const QVariant& val);

	/// Returns the list of QML components that display the user interface for the current edit object.
	QStringList editorComponentList() const;

	/// Returns the list of reference fields of the edit object for which the PROPERTY_FIELD_OPEN_SUBEDITOR flag is set.
	QStringList subobjectFieldList() const;

	/// Returns the minimum value for the numeric parameter.
	FloatType minParameterValue() const;

	/// Returns the maximum value for the numeric parameter.
	FloatType maxParameterValue() const;

	/// Returns the UI display name of the parameter.
	QString propertyDisplayName() const;

Q_SIGNALS:

	/// This signal is emitted whenever the edit object of this parameter UI is replaced.
	void editObjectReplaced();

protected:

	/// Is called when the value of a reference field of this RefMaker changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Updates the internal pointer to the RefMaker property or reference field.
	void updatePropertyField();

private:

	/// The object whose parameter is being edited.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(RefTarget, editObject, setEditObject, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_WEAK_REF | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// The QML property this value source is attached to.
	QQmlProperty _qmlProperty;

	/// The property or reference field being edited or NULL if bound to a QObject property.
	const PropertyFieldDescriptor* _propertyField = nullptr;

	/// The name of the property being edited.
	QString _propertyName;
};

}	// End of namespace
