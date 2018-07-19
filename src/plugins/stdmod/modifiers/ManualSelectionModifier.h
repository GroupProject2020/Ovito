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


#include <plugins/stdmod/StdMod.h>
#include <plugins/stdobj/util/ElementSelectionSet.h>
#include <plugins/stdobj/properties/GenericPropertyModifier.h>
#include <core/dataset/pipeline/ModifierApplication.h>

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
	virtual PipelineFlowState evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

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
