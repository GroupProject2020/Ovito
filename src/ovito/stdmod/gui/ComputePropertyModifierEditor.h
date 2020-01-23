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

#pragma once


#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/gui/desktop/properties/ModifierPropertiesEditor.h>
#include <ovito/core/utilities/DeferredMethodInvocation.h>

namespace Ovito { namespace StdMod {

/**
 * A properties editor for the ComputePropertyModifier class.
 */
class ComputePropertyModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(ComputePropertyModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE ComputePropertyModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

protected Q_SLOTS:

	/// Is called when the user has typed in an expression.
	void onExpressionEditingFinished();

	/// Updates the editor's input fields for the expressions.
	void updateExpressionFields();

	/// Updates the editor's display of the available expression variables.
	void updateVariablesList();

private:

	QGroupBox* expressionsGroupBox;
	QList<AutocompleteLineEdit*> expressionLineEdits;
	QList<AutocompleteTextEdit*> expressionTextEdits;
	QList<QLabel*> expressionLabels;
	QGridLayout* expressionsLayout;
	QLabel* variableNamesDisplay;

	// For deferred invocation of the UI update functions.
	DeferredMethodInvocation<ComputePropertyModifierEditor, &ComputePropertyModifierEditor::updateExpressionFields> updateExpressionFieldsLater;
	DeferredMethodInvocation<ComputePropertyModifierEditor, &ComputePropertyModifierEditor::updateVariablesList> updateVariablesListLater;
};

}	// End of namespace
}	// End of namespace
