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
#include <plugins/particles/objects/BondsDisplay.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <plugins/stdobj/simcell/SimulationCell.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

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
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(CreateBondsModifier, CreateBondsModifierClass)

	Q_CLASSINFO("DisplayName", "Create bonds");
	Q_CLASSINFO("ModifierCategory", "Modification");

public:

	enum CutoffMode {
		UniformCutoff,		///< A single cutoff radius for all particles.
		PairCutoff,			///< Individual cutoff radius for each pair of particle types.
	};
	Q_ENUMS(CutoffMode);

	/// The container type used to store the pair-wise cutoffs.
	typedef QMap<QPair<QString,QString>, FloatType> PairCutoffsList;

private:

	/// Holds the modifier's results.
	class BondsEngineResults : public ComputeEngineResults
	{
	public:

		/// Constructor.
		using ComputeEngineResults::ComputeEngineResults;

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
	
		/// Indicates whether the outdated computation results may be reused and tentatively applied to 
		/// changing inputs from the pipeline without recomputation while the calculation of new results is 
		/// still in progress.
		virtual bool isReapplicable() const override { return false; }

		/// Returns the list of generated bonds.
		std::vector<Bond>& bonds() { return _bonds; }

	private:

		/// The list of generated bonds.
		std::vector<Bond> _bonds;
	};

	/// Compute engine that creates bonds between particles.
	class BondsEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		BondsEngine(ConstPropertyPtr positions, ConstPropertyPtr particleTypes, 
				const SimulationCell& simCell, CutoffMode cutoffMode, FloatType maxCutoff, FloatType minCutoff, std::vector<std::vector<FloatType>> pairCutoffsSquared, 
				ConstPropertyPtr moleculeIDs) :
					_positions(std::move(positions)), 
					_particleTypes(std::move(particleTypes)), 
					_simCell(simCell), 
					_cutoffMode(cutoffMode),
					_maxCutoff(maxCutoff), 
					_minCutoff(minCutoff), 
					_pairCutoffsSquared(std::move(pairCutoffsSquared)), 
					_moleculeIDs(std::move(moleculeIDs)) {}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Returns the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

	private:

		const CutoffMode _cutoffMode;
		const FloatType _maxCutoff;
		const FloatType _minCutoff;
		const std::vector<std::vector<FloatType>> _pairCutoffsSquared;
		const ConstPropertyPtr _positions;
		const ConstPropertyPtr _particleTypes;
		const ConstPropertyPtr _moleculeIDs;
		const SimulationCell _simCell;
	};

public:

	/// Constructor.
	Q_INVOKABLE CreateBondsModifier(DataSet* dataset);

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// Returns the cutoff radii for pairs of particle types.
	const PairCutoffsList& pairCutoffs() const { return _pairCutoffs; }

	/// Sets the cutoff radii for pairs of particle types.
	void setPairCutoffs(const PairCutoffsList& pairCutoffs);

	/// Sets the cutoff radius for a pair of particle types.
	void setPairCutoff(const QString& typeA, const QString& typeB, FloatType cutoff);

	/// Returns the pair-wise cutoff radius for a pair of particle types.
	FloatType getPairCutoff(const QString& typeA, const QString& typeB) const;

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Creates a copy of this object.
	virtual OORef<RefTarget> clone(bool deepCopy, CloneHelper& cloneHelper) override;

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
		
private:

	/// The mode of choosing the cutoff radius.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(CutoffMode, cutoffMode, setCutoffMode);

	/// The cutoff radius for bond generation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, uniformCutoff, setUniformCutoff, PROPERTY_FIELD_MEMORIZE);

	/// The minimum bond length.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, minimumCutoff, setMinimumCutoff);

	/// The cutoff radii for pairs of particle types.
	PairCutoffsList _pairCutoffs;

	/// If true, bonds will only be created between atoms from the same molecule.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, onlyIntraMoleculeBonds, setOnlyIntraMoleculeBonds, PROPERTY_FIELD_MEMORIZE);

	/// The display object for rendering the bonds.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(BondsDisplay, bondsDisplay, setBondsDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY | PROPERTY_FIELD_MEMORIZE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::CreateBondsModifier::CutoffMode);
Q_DECLARE_TYPEINFO(Ovito::Particles::CreateBondsModifier::CutoffMode, Q_PRIMITIVE_TYPE);
