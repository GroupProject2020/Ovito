////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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


#include <ovito/stdmod/StdMod.h>
#include <ovito/stdobj/util/ElementSelectionSet.h>
#include <ovito/stdobj/properties/GenericPropertyModifier.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>

namespace Ovito { namespace StdMod {

/**
 * Modifiers that allows the user to select individual elements, e.g. particles or bonds, by hand.
 */
class OVITO_STDMOD_EXPORT ManualSelectionModifier : public GenericPropertyModifier
{
	Q_OBJECT
	OVITO_CLASS(ManualSelectionModifier)

	Q_CLASSINFO("DisplayName", "Manual selection");
	Q_CLASSINFO("ModifierCategory", "Selection");

public:

	/// Constructor.
	Q_INVOKABLE ManualSelectionModifier(DataSet* dataset);

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// Modifies the input data in an immediate, preliminary way.
	virtual void evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// Adopts the selection state from the modifier's input.
	void resetSelection(ModifierApplication* modApp, const PipelineFlowState& state);

	/// Selects all elements.
	void selectAll(ModifierApplication* modApp, const PipelineFlowState& state);

	/// Deselects all elements.
	void clearSelection(ModifierApplication* modApp, const PipelineFlowState& state);

	/// Toggles the selection state of a single element.
	void toggleElementSelection(ModifierApplication* modApp, const PipelineFlowState& state, size_t elementIndex);

	/// Replaces the selection.
	void setSelection(ModifierApplication* modApp, const PipelineFlowState& state, const boost::dynamic_bitset<>& selection, ElementSelectionSet::SelectionMode mode);

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Returns the selection set object stored in the ModifierApplication, or, if it does not exist, creates one when requested.
	ElementSelectionSet* getSelectionSet(ModifierApplication* modApp, bool createIfNotExist);
};

/**
 * \brief The type of ModifierApplication create for a ManualSelectionModifier
 *        when it is inserted into in a data pipeline.
 */
class OVITO_STDMOD_EXPORT ManualSelectionModifierApplication : public ModifierApplication
{
	Q_OBJECT
	OVITO_CLASS(ManualSelectionModifierApplication)

public:

	/// \brief Constructs a modifier application.
	Q_INVOKABLE ManualSelectionModifierApplication(DataSet* dataset) : ModifierApplication(dataset) {}

private:

	/// The per-application data of the modifier.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(ElementSelectionSet, selectionSet, setSelectionSet, PROPERTY_FIELD_ALWAYS_CLONE);
};

}	// End of namespace
}	// End of namespace
