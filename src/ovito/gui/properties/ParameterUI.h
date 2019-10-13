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

/**
 * \file ParameterUI.h
 * \brief Contains the definition of the Ovito::ParameterUI class and some derived classes.
 */

#pragma once


#include <ovito/gui/GUI.h>
#include <ovito/core/oo/RefTarget.h>
#include "PropertiesEditor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

/**
 * \brief Base class for UI components that allow the user to edit a parameter
 *        of a RefTarget derived object in the PropertiesEditor.
 */
class OVITO_GUI_EXPORT ParameterUI : public RefMaker
{
	Q_OBJECT
	OVITO_CLASS(ParameterUI)

public:

	/// \brief Constructor.
	/// \param parentEditor The editor in which this parameter UI is used. This becomes the parent of this object.
	///
	/// The parameter UI is automatically deleted when the editor is deleted.
	ParameterUI(QObject* parent);

	/// \brief Destructor.
	virtual ~ParameterUI() { clearAllReferences(); }

	/// \brief Returns a pointer to the properties editor this parameter UI belongs to.
	/// \return The editor in which this parameter UI is used or NULL if the parameter UI is used outside of a PropertiesEditor.
	PropertiesEditor* editor() const { return qobject_cast<PropertiesEditor*>(this->parent()); }

	/// \brief Returns the enabled state of the UI.
	/// \return \c true if this parameter's value can be changed by the user;
	///         \c false otherwise.
	/// \sa setEnabled()
	bool isEnabled() const { return _enabled; }

	/// \brief Returns the disabled state of the UI. This is just the inverse of the enabled state.
	/// \return \c false if this parameter's value can be changed by the user;
	///         \c true otherwise.
	/// \sa isEnabled()
	bool isDisabled() const { return !isEnabled(); }

	/// \brief Returns the dataset currently being edited.
	DataSet* dataset() {
		if(editObject())
			_dataset = editObject()->dataset();
		OVITO_ASSERT_MSG(!_dataset.isNull(), "ParameterUI::dataset()", "Can access dataset only while editing an object.");
		return _dataset.data();
	}

	/// \brief Executes the passed functor and catches any exceptions thrown during its execution.
	/// If an exception is thrown by the functor, all changes done by the functor
	/// so far will be undone and an error message is shown to the user.
	template<typename Function>
	void undoableTransaction(const QString& operationLabel, Function&& func) {
		UndoableTransaction::handleExceptions(dataset()->undoStack(), operationLabel, std::forward<Function>(func));
	}

public:

    Q_PROPERTY(Ovito::RefTarget* editObject READ editObject);
	Q_PROPERTY(bool isEnabled READ isEnabled WRITE setEnabled);
	Q_PROPERTY(bool isDisabled READ isDisabled WRITE setDisabled);

Q_SIGNALS:

	/// This signal is emitted when the user is changing the value of the parameter by manipulating the UI widget.
	/// It is not emitted when the parameter value has been changed programmatically.
	void valueEntered();

public Q_SLOTS:

	/// \brief This method is called when a new editable object has been assigned to the properties owner
	///       this parameter UI belongs to.
	///
	/// The parameter UI should react to this change appropriately and
	/// show the properties value for the new edit object in the UI. The default implementation
	/// of this method just calls updateUI() to reflect the change.
	///
	/// \sa setEditObject()
	virtual void resetUI() { updateUI(); }

	/// \brief This method updates the displayed value of the parameter UI.
	///
	/// This method should be overridden by derived classes.
	virtual void updateUI() {}

	/// \brief Sets the enabled state of the UI.
	/// \param enabled Controls whether may change the parameter's value or not.
	/// \sa isEnabled()
	virtual void setEnabled(bool enabled) { _enabled = enabled; }

	/// \brief Sets the enabled state of the UI. This is just the reverse of setEnabled().
	/// \param disabled Controls whether may change the parameter's value or not.
	/// \sa setEnabled()
	/// \sa isDisabled()
	void setDisabled(bool disabled) { setEnabled(!disabled); }

	/// \brief Sets the object whose property is being displayed in this parameter UI.
	/// \sa editObject()
	void setEditObject(RefTarget* newObject) {
		_editObject.set(this, PROPERTY_FIELD(editObject), newObject);
		resetUI();
	}

private:

	/// The object whose parameter is being edited.
	DECLARE_REFERENCE_FIELD_FLAGS(RefTarget, editObject, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_WEAK_REF | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// Stores whether this UI is enabled.
	bool _enabled;

	/// The dataset currently being edited.
	QPointer<DataSet> _dataset;
};

/**
 * \brief Base class for UI components that allow the user to edit a property of
 *        an object that is stored in a reference field, a property field, or a Qt property.
 */
class OVITO_GUI_EXPORT PropertyParameterUI : public ParameterUI
{
	OVITO_CLASS(PropertyParameterUI)
	Q_OBJECT

public:

	/// \brief Constructor for a Qt property.
	/// \param parent The editor in which this parameter UI is used. This becomes the parent of this object.
	/// \param propertyName The name of the property that has been defined using the \c Q_PROPERTY macro.
	PropertyParameterUI(QObject* parent, const char* propertyName);

	/// \brief Constructor for a PropertyField or ReferenceField.
	/// \param parent The editor in which this parameter UI is used. This becomes the parent of this object.
	/// \param propField The property or reference field.
	PropertyParameterUI(QObject* parent, const PropertyFieldDescriptor& propField);

	/// \brief Destructor.
	virtual ~PropertyParameterUI() { clearAllReferences(); }

	/// \brief Returns the property being edited in this parameter UI.
	/// \return The name of the QObject property this UI is bound to or \c
	///         NULL if this PropertyUI is bound to a PropertyField.
	/// \sa propertyField()
	const char* propertyName() const { return _propertyName; }

	/// \brief Returns the property or reference field being edited.
	/// \return A pointer to the descriptor of the PropertyField or ReferenceField being edited or
	///         \c NULL if this PropertyUI is bound to a normal Qt property.
	/// \sa propertyName()
	const PropertyFieldDescriptor* propertyField() const { return _propField; }

	/// \brief Indicates whether this parameter UI is representing a sub-object property (e.g. an animation controller).
	bool isReferenceFieldUI() const { return (_propField && _propField->isReferenceField()); }

	/// \brief Indicates whether this parameter UI is representing a PropertyField based property.
	bool isPropertyFieldUI() const { return (_propField && !_propField->isReferenceField()); }

	/// \brief Indicates whether this parameter UI is representing a Qt property.
	bool isQtPropertyUI() const { return _propField == nullptr; }

	/// \brief This method is called when parameter object has been assigned to the reference field of the editable object
	/// this parameter UI is bound to.
	///
	/// It is also called when the editable object itself has
	/// been replaced in the editor. The parameter UI should react to this change appropriately and
	/// show the property value for the new edit object in the UI. New implementations of this
	/// method must call the base implementation before any other action is taken.
	virtual void resetUI() override;

public:

	Q_PROPERTY(const char* propertyName READ propertyName);
	Q_PROPERTY(RefTarget parameterObject READ parameterObject);

protected Q_SLOTS:

	/// This slot is called when the user has changed the value of the parameter.
	/// It stores the new value in the application's settings store so that it can be used
	/// as the default initialization value next time when a new object of the same class is created.
	void memorizeDefaultParameterValue();

	/// Opens the animation key editor if the parameter managed by this UI class is animatable.
	void openAnimationKeyEditor();

protected:

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

private:

	/// The controller or sub-object whose value is being edited.
	/// This may be \c NULL either when there is no editable object selected in the parent editor
	/// or if the editable object's reference field is currently empty.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(RefTarget, parameterObject, setParameterObject, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_WEAK_REF | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// The property or reference field being edited or NULL if bound to a Qt property.
	const PropertyFieldDescriptor* _propField;

	/// The name of the Qt property being edited or NULL.
	const char* _propertyName;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


