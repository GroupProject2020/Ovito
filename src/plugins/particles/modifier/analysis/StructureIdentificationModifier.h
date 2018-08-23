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
#include <plugins/particles/util/ParticleOrderingFingerprint.h>
#include <plugins/stdobj/simcell/SimulationCell.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief Base class for modifiers that assign a structure type to each particle.
 */
class OVITO_PARTICLES_EXPORT StructureIdentificationModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class StructureIdentificationModifierClass : public AsynchronousModifier::OOMetaClass 
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(StructureIdentificationModifier, StructureIdentificationModifierClass)
	
public:

	/// Computes the modifier's results.
	class StructureIdentificationEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		StructureIdentificationEngine(ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions, const SimulationCell& simCell, QVector<bool> typesToIdentify, ConstPropertyPtr selection = {}) :
			ComputeEngine(),
			_positions(std::move(positions)), 
			_simCell(simCell),
			_typesToIdentify(std::move(typesToIdentify)),
			_selection(std::move(selection)),
			_structures(ParticleProperty::createStandardStorage(fingerprint.particleCount(), ParticleProperty::StructureTypeProperty, false)),
			_inputFingerprint(std::move(fingerprint)) {}

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_positions.reset();
			_selection.reset();
			decltype(_typesToIdentify){}.swap(_typesToIdentify);
			ComputeEngine::cleanup();
		}

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

		/// Returns the property storage that contains the computed per-particle structure types.
		const PropertyPtr& structures() const { return _structures; }

		/// Returns the property storage that contains the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Returns the property storage that contains the particle selection (optional).
		const ConstPropertyPtr& selection() const { return _selection; }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

		/// Returns the list of structure types to search for.
		const QVector<bool>& typesToIdentify() const { return _typesToIdentify; }

		/// Returns the number of identified particles of the given structure type.
		qlonglong getTypeCount(int typeIndex) const {
			if(_typeCounts && _typeCounts->size() > typeIndex) return _typeCounts->getInt64(typeIndex);
			return 0;
		}

	protected:

		/// Gives subclasses the possibility to post-process per-particle structure types
		/// before they are output to the data pipeline.
		virtual PropertyPtr postProcessStructureTypes(TimePoint time, ModifierApplication* modApp, const PropertyPtr& structures) { 
			return structures;
		}

	private:

		ConstPropertyPtr _positions;
		ConstPropertyPtr _selection;
		const SimulationCell _simCell;
		QVector<bool> _typesToIdentify;
		const PropertyPtr _structures;
		ParticleOrderingFingerprint _inputFingerprint;
		PropertyPtr _typeCounts;
	};

public:

	/// Constructor.
	StructureIdentificationModifier(DataSet* dataset);

	/// This method indicates whether cached computation results of the modifier should be discarded whenever
	/// a parameter of the modifier changes.
	virtual bool discardResultsOnModifierChange(const PropertyFieldEvent& event) const override { 
		// Avoid a recomputation from scratch if the color-by-type option is being changed.
		if(event.field() == &PROPERTY_FIELD(colorByType)) return false;
		return AsynchronousModifier::discardResultsOnModifierChange(event);
	}

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Inserts a structure type into the list.
	void addStructureType(ParticleType* type) { _structureTypes.push_back(this, PROPERTY_FIELD(structureTypes), type); }

	/// Create an instance of the ParticleType class to represent a structure type.
	ParticleType* createStructureType(int id, ParticleType::PredefinedStructureType predefType);

	/// Returns a bit flag array which indicates what structure types to search for.
	QVector<bool> getTypesToIdentify(int numTypes) const;

private:

	/// Contains the list of structure types recognized by this analysis modifier.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(ElementType, structureTypes, setStructureTypes);

	/// Controls whether analysis should take into account only selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedParticles, setOnlySelectedParticles);

	/// Controls whether the modifier colors particles based on their type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, colorByType, setColorByType);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


