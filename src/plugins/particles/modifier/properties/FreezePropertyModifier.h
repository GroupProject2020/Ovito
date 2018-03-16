///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include <plugins/particles/objects/ParticleProperty.h>
#include <core/dataset/pipeline/Modifier.h>
#include <core/dataset/pipeline/ModifierApplication.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)

/**
 * \brief Saves the current state of a particle property and preserves it over time.
 */
class OVITO_PARTICLES_EXPORT FreezePropertyModifier : public Modifier
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
	OVITO_CLASS_META(FreezePropertyModifier, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Freeze property");
	Q_CLASSINFO("ModifierCategory", "Modification");

public:

	/// Constructor.
	Q_INVOKABLE FreezePropertyModifier(DataSet* dataset);

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// Modifies the input data.
	virtual Future<PipelineFlowState> evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
		
	/// Modifies the input data in an immediate, preliminary way.
	virtual PipelineFlowState evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// The particle property that is preserved by this modifier.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, sourceProperty, setSourceProperty);

	/// The particle property to which the stored values should be written
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, destinationProperty, setDestinationProperty);

	/// Animation time at which the frozen property is taken.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(TimePoint, freezeTime, setFreezeTime);
};

/**
 * Used by the FreezePropertyModifier to store the values of the selected particle property.
 */
class OVITO_PARTICLES_EXPORT FreezePropertyModifierApplication : public ModifierApplication
{
	OVITO_CLASS(FreezePropertyModifierApplication)
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE FreezePropertyModifierApplication(DataSet* dataset) : ModifierApplication(dataset) {}

	/// Makes a copy of the given source property and, optionally, of the provided
	/// particle identifier list, which will allow to restore the saved property
	/// values even if the order of particles changes.
	void updateStoredData(ParticleProperty* property, ParticleProperty* identifiers, TimeInterval validityInterval);

	/// Returns true if the frozen state for given animation time is already stored.
	bool hasFrozenState(TimePoint time) const { return _validityInterval.contains(time); }

	/// Clears the stored state.
	void invalidateFrozenState() {
		setProperty(nullptr);
		setIdentifiers(nullptr);
		_validityInterval.setEmpty();
	}

protected:

	/// Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;
	
private:

	/// The stored copy of the particle property.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(ParticleProperty, property, setProperty, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_CHANGE_MESSAGE | PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_SUB_ANIM | PROPERTY_FIELD_DONT_SAVE_RECOMPUTABLE_DATA);

	/// A copy of the particle identifiers, taken at the time when the property values were saved.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(ParticleProperty, identifiers, setIdentifiers, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_CHANGE_MESSAGE | PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_SUB_ANIM | PROPERTY_FIELD_DONT_SAVE_RECOMPUTABLE_DATA);

	/// The cached visalization elements that are attached to the output particle property.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD_FLAGS(DataVis, cachedVisElements, setCachedVisElements, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_CHANGE_MESSAGE | PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_SUB_ANIM | PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES);

	/// The validity interval of the frozen property.
	TimeInterval _validityInterval;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


