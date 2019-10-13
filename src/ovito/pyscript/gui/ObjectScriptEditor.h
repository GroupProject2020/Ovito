////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
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


#include <ovito/pyscript/PyScript.h>
#include <ovito/core/oo/RefTargetListener.h>
#include <ovito/gui/GUI.h>

class QsciScintilla;

namespace PyScript {

using namespace Ovito;

/**
 * \brief A script editor UI component.
 */
class ObjectScriptEditor : public QMainWindow
{
	Q_OBJECT

public:

	/// Constructor.
	ObjectScriptEditor(QWidget* parentWidget, RefTarget* scriptableObject);

	/// Returns an existing editor window for the given object if there is one.
	static ObjectScriptEditor* findEditorForObject(RefTarget* scriptableObject);

public Q_SLOTS:

	/// Compiles/runs the current script.
	void onCommitScript();

	/// Lets the user load a script file into the editor.
	void onLoadScriptFromFile();

	/// Lets the user save the current script to a file.
	void onSaveScriptToFile();

protected Q_SLOTS:

	/// Is called when the scriptable object generates an event.
	void onNotificationEvent(const ReferenceEvent& event);

	/// Replaces the editor contents with the script from the owning object.
	void updateEditorContents();

	/// Replaces the output window contents with the script output cached by the owning object.
	void updateOutputWindow();

protected:

	/// Obtains the current script from the owner object.
	virtual const QString& getObjectScript(RefTarget* obj) const = 0;

	/// Obtains the script output cached by the owner object.
	virtual QString getOutputText(RefTarget* obj) const = 0;

	/// Sets the current script of the owner object.
	virtual void setObjectScript(RefTarget* obj, const QString& script) const = 0;

	/// Is called when the user closes the window.
	virtual void closeEvent(QCloseEvent* event) override;

	/// Is called when the window is shown.
	virtual void showEvent(QShowEvent* event) override;

	/// The main text editor component.
	QsciScintilla* _codeEditor;

	/// The text box that displays the script's output.
	QsciScintilla* _outputWindow;

	/// The object which the current script belongs to.
	RefTargetListener<RefTarget> _scriptableObject;

	/// The action that undoes the last edit operation.
	QAction* _undoAction;

	/// The action that redoes the last undone edit operation.
	QAction* _redoAction;
};

};


