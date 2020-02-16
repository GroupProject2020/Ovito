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


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/BondsVis.h>
#include <ovito/particles/objects/BondType.h>
#include <ovito/particles/util/ParticleOrderingFingerprint.h>
#include <ovito/stdobj/simcell/SimulationCell.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { namespace Particles {

/**
 * \brief A modifier that creates bonds between pairs of particles based on their distance.
 */
class OVITO_PARTICLES_EXPORT CreateBondsModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class CreateBondsModifierClass : public ModifierClass
	{
	public:

		/// Inherit constructor from base class.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(CreateBondsModifier, CreateBondsModifierClass)

	Q_CLASSINFO("DisplayName", "Create bonds");
	Q_CLASSINFO("ModifierCategory", "Visualization");

public:

	enum CutoffMode {
		UniformCutoff,		///< A single cutoff radius for all particles.
		PairCutoff,			///< Individual cutoff radius for each pair of particle types.
	};
	Q_ENUMS(CutoffMode);

	/// The container type used to store the pair-wise cutoffs.
	typedef QMap<QPair<QVariant,QVariant>, FloatType> PairwiseCutoffsList;

private:

	/// Compute engine that creates bonds between particles.
	class BondsEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		BondsEngine(ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions, ConstPropertyPtr particleTypes,
				const SimulationCell& simCell, CutoffMode cutoffMode, FloatType maxCutoff, FloatType minCutoff, std::vector<std::vector<FloatType>> pairCutoffsSquared,
				ConstPropertyPtr moleculeIDs) :
					_positions(std::move(positions)),
					_particleTypes(std::move(particleTypes)),
					_simCell(simCell),
					_cutoffMode(cutoffMode),
					_maxCutoff(maxCutoff),
					_minCutoff(minCutoff),
					_pairCutoffsSquared(std::move(pairCutoffsSquared)),
					_moleculeIDs(std::move(moleculeIDs)),
					_inputFingerprint(std::move(fingerprint)) {}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		/// Returns the list of generated bonds.
		std::vector<Bond>& bonds() { return _bonds; }

		/// Returns the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

	private:

		const CutoffMode _cutoffMode;
		const FloatType _maxCutoff;
		const FloatType _minCutoff;
		const std::vector<std::vector<FloatType>> _pairCutoffsSquared;
		ConstPropertyPtr _positions;
		ConstPropertyPtr _particleTypes;
		ConstPropertyPtr _moleculeIDs;
		const SimulationCell _simCell;
		ParticleOrderingFingerprint _inputFingerprint;
		std::vector<Bond> _bonds;
	};

public:

	/// Constructor.
	Q_INVOKABLE CreateBondsModifier(DataSet* dataset);

	/// \brief This method is called by the system when the modifier has been inserted into a data pipeline.
	/// \param modApp The ModifierApplication object that has been created for this modifier.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// Indicate that outdated computation results should never be reused if the modifier's inputs have changed.
	virtual bool discardResultsOnInputChange() const override { return true; }

	/// Sets the cutoff radius for a pair of particle types.
	void setPairwiseCutoff(const QVariant& typeA, const QVariant& typeB, FloatType cutoff);

	/// Returns the pair-wise cutoff radius for a pair of particle types.
	FloatType getPairwiseCutoff(const QVariant& typeA, const QVariant& typeB) const;

protected:

	/// Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input) override;

	/// Looks up a particle type in the type list based on the name or the numeric ID.
	static const ElementType* lookupParticleType(const PropertyObject* typeProperty, const QVariant& typeSpecification);

private:

	/// The mode of choosing the cutoff radius.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(CutoffMode, cutoffMode, setCutoffMode);

	/// The cutoff radius for bond generation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, uniformCutoff, setUniformCutoff, PROPERTY_FIELD_MEMORIZE);

	/// The minimum bond length.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, minimumCutoff, setMinimumCutoff);

	/// The cutoff radii for pairs of particle types.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(PairwiseCutoffsList, pairwiseCutoffs, setPairwiseCutoffs);

	/// If true, bonds will only be created between atoms from the same molecule.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, onlyIntraMoleculeBonds, setOnlyIntraMoleculeBonds, PROPERTY_FIELD_MEMORIZE);

	/// The bond type object that will be assigned to the newly created bonds.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(BondType, bondType, setBondType, PROPERTY_FIELD_MEMORIZE | PROPERTY_FIELD_OPEN_SUBEDITOR);

	/// The vis element for rendering the bonds.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(BondsVis, bondsVis, setBondsVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE | PROPERTY_FIELD_OPEN_SUBEDITOR);

	/// Controls whether the modifier should automatically turn off the display in case the number of bonds is unusually large.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, autoDisableBondDisplay, setAutoDisableBondDisplay, PROPERTY_FIELD_NO_CHANGE_MESSAGE | PROPERTY_FIELD_NO_UNDO);
};

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::CreateBondsModifier::CutoffMode);
Q_DECLARE_TYPEINFO(Ovito::Particles::CreateBondsModifier::CutoffMode, Q_PRIMITIVE_TYPE);
