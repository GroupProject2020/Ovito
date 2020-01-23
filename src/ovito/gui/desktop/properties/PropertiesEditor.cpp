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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/properties/PropertiesEditor.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

IMPLEMENT_OVITO_CLASS(PropertiesEditor);
DEFINE_REFERENCE_FIELD(PropertiesEditor, editObject);

/******************************************************************************
* Returns the global editor registry, which can be used to look up the editor
* class for editable RefTarget class.
******************************************************************************/
PropertiesEditor::Registry& PropertiesEditor::registry()
{
	static Registry singleton;
	return singleton;
}

/******************************************************************************
* Creates a PropertiesEditor for an object.
******************************************************************************/
OORef<PropertiesEditor> PropertiesEditor::create(RefTarget* obj)
{
	OVITO_CHECK_POINTER(obj);
	try {
		// Look if an editor class has been registered for this RefTarget class and one of its super classes.
		for(OvitoClassPtr clazz = &obj->getOOClass(); clazz != nullptr; clazz = clazz->superClass()) {
			OvitoClassPtr editorClass = registry().getEditorClass(clazz);
			if(editorClass) {
				if(!editorClass->isDerivedFrom(PropertiesEditor::OOClass()))
					throw Exception(tr("The editor class %1 assigned to the RefTarget-derived class %2 is not derived from PropertiesEditor.").arg(editorClass->name(), clazz->name()));
				return dynamic_object_cast<PropertiesEditor>(editorClass->createInstance(nullptr));
			}
		}
	}
	catch(Exception& ex) {
		if(ex.context() == nullptr) ex.setContext(obj->dataset());
		ex.prependGeneralMessage(tr("Failed to create editor component for the '%1' object.").arg(obj->objectTitle()));
		ex.reportError();
	}
	return nullptr;
}

/******************************************************************************
* This will bind the editor to the given container.
******************************************************************************/
void PropertiesEditor::initialize(PropertiesPanel* container, MainWindow* mainWindow, const RolloutInsertionParameters& rolloutParams, PropertiesEditor* parentEditor)
{
	OVITO_CHECK_POINTER(container);
	OVITO_CHECK_POINTER(mainWindow);
	OVITO_ASSERT_MSG(_container == nullptr, "PropertiesEditor::initialize()", "Editor can only be initialized once.");
	_container = container;
	_mainWindow = mainWindow;
	_parentEditor = parentEditor;
	createUI(rolloutParams);
	Q_EMIT contentsReplaced(nullptr);
}

/******************************************************************************
* Creates a new rollout in the rollout container and returns
* the empty widget that can then be filled with UI controls.
* The rollout is automatically deleted when the editor is deleted.
******************************************************************************/
QWidget* PropertiesEditor::createRollout(const QString& title, const RolloutInsertionParameters& params, const char* helpPage)
{
	OVITO_ASSERT_MSG(container(), "PropertiesEditor::createRollout()", "Editor has not been properly initialized.");
	QWidget* panel = new QWidget(params.container());
	_rollouts.add(panel);
	if(params.container() == nullptr) {

		// Let the rollout-insertion parameters set the rollout title prefix.
		QString titlePrefix;
		if(!params.title().isEmpty())
			titlePrefix = QString("%1: ").arg(params.title());

		// Create a new rollout in the rollout container.
		QPointer<Rollout> rollout = container()->addRollout(panel, titlePrefix + title, params, helpPage);

		// Check if a title for the rollout has been specified. If not,
		// automatically set the rollout title to the title of the object being edited.
		if(title.isEmpty()) {

			if(editObject())
				rollout->setTitle(titlePrefix + editObject()->objectTitle());

			// Automatically update rollout title each time a new object is loaded into the editor.
			connect(this, &PropertiesEditor::contentsReplaced, rollout, [rollout, titlePrefix](RefTarget* target) {
				if(rollout && target)
					rollout->setTitle(titlePrefix + target->objectTitle());
			});
		}
	}
	else if(params.container()->layout()) {

		// Instead of creating a new rollout for the widget, insert widget into a prescribed parent widget.
		params.container()->layout()->addWidget(panel);
	}
	return panel;
}

/******************************************************************************
* Completely disables the UI elements in the given rollout widget.
******************************************************************************/
void PropertiesEditor::disableRollout(QWidget* rolloutWidget, const QString& noticeText)
{
	rolloutWidget->setEnabled(false);
	if(Rollout* rollout = container()->findRolloutFromWidget(rolloutWidget)) {
		rollout->setNotice(noticeText);
		// Force a re-layout of the rollouts.
		QTimer::singleShot(100, container(), &RolloutContainer::updateRollouts);	
	}
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool PropertiesEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject() && event.type() == ReferenceEvent::TargetChanged) {
		Q_EMIT contentsChanged(source);
	}
	return RefMaker::referenceEvent(source, event);
}

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void PropertiesEditor::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(editObject)) {
		setDataset(editObject() ? editObject()->dataset() : nullptr);
		if(oldTarget) oldTarget->unsetObjectEditingFlag();
		if(newTarget) newTarget->setObjectEditingFlag();
		Q_EMIT contentsReplaced(editObject());
		Q_EMIT contentsChanged(editObject());
	}
	RefMaker::referenceReplaced(field, oldTarget, newTarget);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
