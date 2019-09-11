///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once


#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/gui/properties/ModifierPropertiesEditor.h>
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
