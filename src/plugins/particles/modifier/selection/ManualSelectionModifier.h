///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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


#include <plugins/particles/Particles.h>
#include <plugins/particles/util/ParticleSelectionSet.h>
#include <core/dataset/pipeline/Modifier.h>
#include <core/dataset/pipeline/ModifierApplication.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)

/**
 * Modifiers that allows the user to select individual particles by hand.
 */
class OVITO_PARTICLES_EXPORT ManualSelectionModifier : public Modifier
{
	/// Give this modifier class its own metaclass.
	class OOMetaClass : public Modifier::OOMetaClass 
	{
	public:

		/// Inherit constructor from base metaclass.
		using Modifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(ManualSelectionModifier, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Manual selection");
	Q_CLASSINFO("ModifierCategory", "Selection");

public:

	/// Constructor.
	Q_INVOKABLE ManualSelectionModifier(DataSet* dataset) : Modifier(dataset) {}

	/// \brief Create a new modifier application that refers to this modifier instance.
	virtual OORef<ModifierApplication> createModifierApplication() override;

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// Modifies the input data in an immediate, preliminary way.
	virtual PipelineFlowState evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

	/// Adopts the selection state from the modifier's input.
	void resetSelection(ModifierApplication* modApp, const PipelineFlowState& state);

	/// Selects all particles.
	void selectAll(ModifierApplication* modApp, const PipelineFlowState& state);

	/// Deselects all particles.
	void clearSelection(ModifierApplication* modApp, const PipelineFlowState& state);

	/// Toggles the selection state of a single particle.
	void toggleParticleSelection(ModifierApplication* modApp, const PipelineFlowState& state, size_t particleIndex);

	/// Replaces the particle selection.
	void setParticleSelection(ModifierApplication* modApp, const PipelineFlowState& state, const QBitArray& selection, ParticleSelectionSet::SelectionMode mode);

protected:

	/// Returns the selection set object stored in the ModifierApplication, or, if it does not exist, creates one when requested.
	ParticleSelectionSet* getSelectionSet(ModifierApplication* modApp, bool createIfNotExist);
};

/**
 * \brief The type of ModifierApplication create for a ManualSelectionModifier 
 *        when it is inserted into in a data pipeline.
 */
class OVITO_PARTICLES_EXPORT ManualSelectionModifierApplication : public ModifierApplication
{
	Q_OBJECT
	OVITO_CLASS(ManualSelectionModifierApplication)
   
public:

	/// \brief Constructs a modifier application.
	Q_INVOKABLE ManualSelectionModifierApplication(DataSet* dataset) : ModifierApplication(dataset) {}

private:

	/// The per-application data of the modifier.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(ParticleSelectionSet, selectionSet, setSelectionSet, PROPERTY_FIELD_ALWAYS_CLONE);
};
 

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


