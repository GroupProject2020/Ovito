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


#include <plugins/particles/gui/ParticlesGui.h>
#include <gui/properties/ModifierPropertiesEditor.h>
#include <core/utilities/DeferredMethodInvocation.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A properties editor for the ParticlesComputePropertyModifierDelegate class.
 */
class ParticlesComputePropertyModifierDelegateEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(ParticlesComputePropertyModifierDelegateEditor)

public:

	/// Default constructor.
	Q_INVOKABLE ParticlesComputePropertyModifierDelegateEditor() {}

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

	QGroupBox* neighborExpressionsGroupBox;
	QList<AutocompleteLineEdit*> neighborExpressionLineEdits;
	QList<AutocompleteTextEdit*> neighborExpressionTextEdits;
	QList<QLabel*> neighborExpressionLabels;
	QGridLayout* neighborExpressionsLayout;

	// For deferred invocation of the UI update functions.
	DeferredMethodInvocation<ParticlesComputePropertyModifierDelegateEditor, &ParticlesComputePropertyModifierDelegateEditor::updateExpressionFields> updateExpressionFieldsLater;
	DeferredMethodInvocation<ParticlesComputePropertyModifierDelegateEditor, &ParticlesComputePropertyModifierDelegateEditor::updateVariablesList> updateVariablesListLater;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
