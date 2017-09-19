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
#include <plugins/particles/objects/ParticleType.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <core/dataset/data/simcell/SimulationCell.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>
#include <core/dataset/pipeline/AsynchronousModifierApplication.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief Base class for modifiers that assign a structure type to each particle.
 */
class OVITO_PARTICLES_EXPORT StructureIdentificationModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class StructureIdentificationModifierClass : public ModifierClass 
	{
	public:

		/// Inherit constructor from base metaclass.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(StructureIdentificationModifier, StructureIdentificationModifierClass)
	
public:

	/// Holds the modifier's results.
	class StructureIdentificationResults : public ComputeEngineResults
	{
	public:

		/// Constructor.
		StructureIdentificationResults(size_t particleCount) :
			_structures(ParticleProperty::createStandardStorage(particleCount, ParticleProperty::StructureTypeProperty, false)) {}

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

		/// Returns the property storage that contains the computed per-particle structure types.
		const PropertyPtr& structures() const { return _structures; }

	protected:

		/// Gives subclasses the possibility to post-process per-particle structure types
		/// before they are output to the data pipeline.
		virtual PropertyPtr postProcessStructureTypes(TimePoint time, ModifierApplication* modApp, const PropertyPtr& structures) { 
			return structures;
		}

	private:

		const PropertyPtr _structures;
	};

	/// Computes the modifier's results.
	class StructureIdentificationEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		StructureIdentificationEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, const SimulationCell& simCell, QVector<bool> typesToIdentify, ConstPropertyPtr selection = {}) :
			ComputeEngine(validityInterval),
			_positions(std::move(positions)), 
			_simCell(simCell),
			_typesToIdentify(std::move(typesToIdentify)),
			_selection(std::move(selection)) {}

		/// Returns the property storage that contains the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Returns the property storage that contains the particle selection (optional).
		const ConstPropertyPtr& selection() const { return _selection; }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

		/// Returns the list of structure types to search for.
		const QVector<bool>& typesToIdentify() const { return _typesToIdentify; }

	private:

		const ConstPropertyPtr _positions;
		const ConstPropertyPtr _selection;
		const SimulationCell _simCell;
		const QVector<bool> _typesToIdentify;
	};

public:

	/// Constructor.
	StructureIdentificationModifier(DataSet* dataset);

	/// \brief Create a new modifier application that refers to this modifier instance.
	virtual OORef<ModifierApplication> createModifierApplication() override;

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Inserts a structure type into the list.
	void addStructureType(ParticleType* type) { _structureTypes.push_back(this, PROPERTY_FIELD(structureTypes), type); }

	/// Create an instance of the ParticleType class to represent a structure type.
	void createStructureType(int id, ParticleType::PredefinedStructureType predefType);

	/// Returns a bit flag array which indicates what structure types to search for.
	QVector<bool> getTypesToIdentify(int numTypes) const;

private:

	/// Contains the list of structure types recognized by this analysis modifier.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(ElementType, structureTypes, setStructureTypes);

	/// Controls whether analysis should take into account only selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedParticles, setOnlySelectedParticles);
};


/**
 * \brief The type of ModifierApplication create for a StructureIdentificationModifier 
 *        when it is inserted into in a data pipeline.
 */
class OVITO_PARTICLES_EXPORT StructureIdentificationModifierApplication : public AsynchronousModifierApplication
{
	Q_OBJECT
	OVITO_CLASS(StructureIdentificationModifierApplication)

public:

	/// Constructor.
	Q_INVOKABLE StructureIdentificationModifierApplication(DataSet* dataset) : AsynchronousModifierApplication(dataset) {}

	/// Returns an array that contains the number of matching particles for each structure type.
	const std::vector<size_t>& structureCounts() const { return _structureCounts; }

	/// Sets the array containing the number of matching particles for each structure type.
	void setStructureCounts(std::vector<size_t> counts) {
		_structureCounts = std::move(counts);
		notifyDependents(ReferenceEvent::ObjectStatusChanged);
	}

private:

	std::vector<size_t> _structureCounts;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


