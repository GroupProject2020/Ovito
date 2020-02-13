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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/widgets/general/RolloutContainer.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/dataset/DataSet.h>
#include "PropertiesPanel.h"

namespace Ovito {

/**
 * \brief Base class for property editors for RefTarget derived objects.
 *
 * A properties editor for a RefTarget derived object can be created using the PropertiesEditor::create() function.
 */
class OVITO_GUI_EXPORT PropertiesEditor : public RefMaker
{
	Q_OBJECT
	OVITO_CLASS(PropertiesEditor)

public:

	/// Registry for editor classes.
	class OVITO_GUI_EXPORT Registry : private std::map<OvitoClassPtr, OvitoClassPtr>
	{
	public:
		void registerEditorClass(OvitoClassPtr refTargetClass, OvitoClassPtr editorClass) {
			insert(std::make_pair(refTargetClass, editorClass));
		}
		OvitoClassPtr getEditorClass(OvitoClassPtr refTargetClass) const {
			auto entry = find(refTargetClass);
			if(entry != end()) return entry->second;
			else return nullptr;
		}
	};

	/// Returns the global editor registry, which allows looking up the editor class for a RefTarget class.
	static Registry& registry();

protected:

	/// \brief The constructor.
	PropertiesEditor() = default;

public:

	/// \brief Creates a PropertiesEditor for an editable object.
	/// \param obj The object for which an editor should be created.
	/// \return The new editor component that allows the user to edit the properties of the RefTarget object.
	///         It will be automatically destroyed by the system when the editor is closed.
	///         Returns NULL if no editor component is registered for the RefTarget type.
	///
	/// The returned editor object is not initialized yet. Call initialize() once to do so.
	static OORef<PropertiesEditor> create(RefTarget* obj);

	/// \brief The virtual destructor.
	virtual ~PropertiesEditor() { clearAllReferences(); }

	/// \brief This will bind the editor to the given container.
	/// \param container The properties panel that's the host of the editor.
	/// \param mainWindow The main window that hosts the editor.
	/// \param rolloutParams Specifies how the editor's rollouts should be created.
	/// \param parentEditor The editor that owns this editor if it is a sub-object editor; NULL otherwise.
	///
	/// This method is called by the PropertiesPanel class to initialize the editor and to create the UI.
	void initialize(PropertiesPanel* container, MainWindow* mainWindow, const RolloutInsertionParameters& rolloutParams, PropertiesEditor* parentEditor);

	/// \brief Returns the rollout container widget this editor is placed in.
	PropertiesPanel* container() const { return _container; }

	/// \brief Returns the main window that hosts the editor.
	MainWindow* mainWindow() const { return _mainWindow; }

	/// Returns a pointer to the parent editor which has opened this editor for one of its sub-components.
	PropertiesEditor* parentEditor() const { return _parentEditor; }

	/// \brief Creates a new rollout in the rollout container and returns
	///        the empty widget that can then be filled with UI controls.
	/// \param title The title of the rollout.
	/// \param rolloutParams Specifies how the rollout should be created.
	/// \param helpPage The help page or topic in the user manual that describes this rollout.
	///
	/// \note The rollout is automatically deleted when the editor is deleted.
	QWidget* createRollout(const QString& title, const RolloutInsertionParameters& rolloutParams, const char* helpPage = nullptr);

	/// \brief Completely disables the UI elements in the given rollout widget.
	/// \param rolloutWidget The rollout widget to be disabled, which has been created by createRollout().
	/// \param noticeText A text to displayed in the rollout panel to inform the user why the rollout has been disabled.
	void disableRollout(QWidget* rolloutWidget, const QString& noticeText);

	/// \brief Executes the passed functor and catches any exceptions thrown during its execution.
	/// If an exception is thrown by the functor, all changes done by the functor
	/// so far will be undone and an error message is shown to the user.
	template<typename Function>
	void undoableTransaction(const QString& operationLabel, Function&& func) {
		UndoableTransaction::handleExceptions(dataset()->undoStack(), operationLabel, std::forward<Function>(func));
	}

public Q_SLOTS:

	/// \brief Sets the object being edited in this editor.
	/// \param newObject The new object to load into the editor. This must be of the same class
	///                  as the previous object.
	///
	/// This method generates a contentsReplaced() and a contentsChanged() signal.
	void setEditObject(RefTarget* newObject) {
		OVITO_ASSERT_MSG(!editObject() || !newObject || newObject->getOOClass().isDerivedFrom(editObject()->getOOClass()),
				"PropertiesEditor::setEditObject()", "This properties editor was not made for this object class.");
		_editObject.set(this, PROPERTY_FIELD(editObject), newObject);
	}

Q_SIGNALS:

	/// \brief This signal is emitted by the editor when a new edit object
	///        has been loaded into the editor via the setEditObject() method.
	/// \sa newEditObject The new object loaded into the editor.
    void contentsReplaced(Ovito::RefTarget* newEditObject);

	/// \brief This signal is emitted by the editor when the current edit object has generated a TargetChanged
	///        event or if a new object has been loaded into editor via the setEditObject() method.
	/// \sa editObject The object that has changed.
    void contentsChanged(Ovito::RefTarget* editObject);

protected:

	/// Creates the user interface controls for the editor.
	/// This must be implemented by sub-classes.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) = 0;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Is called when the value of a reference field of this RefMaker changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

private:

	/// The container widget the editor is shown in.
	PropertiesPanel* _container = nullptr;

	/// The main window that hosts the editor.
	MainWindow* _mainWindow = nullptr;

	/// Pointer to the parent editor which opened this editor for a sub-component.
	PropertiesEditor* _parentEditor = nullptr;

	/// The object being edited in this editor.
	DECLARE_REFERENCE_FIELD_FLAGS(RefTarget, editObject, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// The list of rollout widgets that have been created by editor.
	/// The cleanup handler is used to delete them when the editor is being deleted.
	QObjectCleanupHandler _rollouts;
};

/// This macro is used to assign a PropertiesEditor-derived class to a RefTarget-derived class.
#define SET_OVITO_OBJECT_EDITOR(RefTargetClass, PropertiesEditorClass) \
	static const int __editorSetter##RefTargetClass = (Ovito::PropertiesEditor::registry().registerEditorClass(&RefTargetClass::OOClass(), &PropertiesEditorClass::OOClass()), 0);

}	// End of namespace
