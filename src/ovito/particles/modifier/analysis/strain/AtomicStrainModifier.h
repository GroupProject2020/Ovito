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
#include <ovito/particles/modifier/analysis/ReferenceConfigurationModifier.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/util/ParticleOrderingFingerprint.h>
#include <ovito/stdobj/simcell/SimulationCell.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief Calculates the per-particle strain tensors based on a reference configuration.
 */
class OVITO_PARTICLES_EXPORT AtomicStrainModifier : public ReferenceConfigurationModifier
{
	Q_OBJECT
	OVITO_CLASS(AtomicStrainModifier)

	Q_CLASSINFO("DisplayName", "Atomic strain");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

	/// Constructor.
	Q_INVOKABLE AtomicStrainModifier(DataSet* dataset);

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngineInternal(const PipelineEvaluationRequest& request, ModifierApplication* modApp, PipelineFlowState input, const PipelineFlowState& referenceState, TimeInterval validityInterval) override;

private:

	/// Computes the modifier's results.
	class AtomicStrainEngine : public RefConfigEngineBase
	{
	public:

		/// Constructor.
		AtomicStrainEngine(const TimeInterval& validityInterval, ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions, const SimulationCell& simCell,
				ConstPropertyPtr refPositions, const SimulationCell& simCellRef,
				ConstPropertyPtr identifiers, ConstPropertyPtr refIdentifiers,
				FloatType cutoff, AffineMappingType affineMapping, bool useMinimumImageConvention,
				bool calculateDeformationGradients, bool calculateStrainTensors,
				bool calculateNonaffineSquaredDisplacements, bool calculateRotations, bool calculateStretchTensors,
				bool selectInvalidParticles) :
			RefConfigEngineBase(validityInterval, positions, simCell, refPositions, simCellRef,
				std::move(identifiers), std::move(refIdentifiers), affineMapping, useMinimumImageConvention),
			_cutoff(cutoff),
			_displacements(ParticlesObject::OOClass().createStandardStorage(refPositions->size(), ParticlesObject::DisplacementProperty, false)),
			_shearStrains(std::make_shared<PropertyStorage>(fingerprint.particleCount(), PropertyStorage::Float, 1, 0, tr("Shear Strain"), false)),
			_volumetricStrains(std::make_shared<PropertyStorage>(fingerprint.particleCount(), PropertyStorage::Float, 1, 0, tr("Volumetric Strain"), false)),
			_strainTensors(calculateStrainTensors ? ParticlesObject::OOClass().createStandardStorage(fingerprint.particleCount(), ParticlesObject::StrainTensorProperty, false) : nullptr),
			_deformationGradients(calculateDeformationGradients ? ParticlesObject::OOClass().createStandardStorage(fingerprint.particleCount(), ParticlesObject::DeformationGradientProperty, false) : nullptr),
			_nonaffineSquaredDisplacements(calculateNonaffineSquaredDisplacements ? std::make_shared<PropertyStorage>(fingerprint.particleCount(), PropertyStorage::Float, 1, 0, tr("Nonaffine Squared Displacement"), false) : nullptr),
			_invalidParticles(selectInvalidParticles ? ParticlesObject::OOClass().createStandardStorage(fingerprint.particleCount(), ParticlesObject::SelectionProperty, false) : nullptr),
			_rotations(calculateRotations ? ParticlesObject::OOClass().createStandardStorage(fingerprint.particleCount(), ParticlesObject::RotationProperty, false) : nullptr),
			_stretchTensors(calculateStretchTensors ? ParticlesObject::OOClass().createStandardStorage(fingerprint.particleCount(), ParticlesObject::StretchTensorProperty, false) : nullptr),
			_inputFingerprint(std::move(fingerprint)) {}

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_displacements.reset();
			RefConfigEngineBase::cleanup();
		}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		/// Returns the property storage that contains the computed per-particle shear strain values.
		const PropertyPtr& shearStrains() const { return _shearStrains; }

		/// Returns the property storage that contains the computed per-particle volumetric strain values.
		const PropertyPtr& volumetricStrains() const { return _volumetricStrains; }

		/// Returns the property storage that contains the computed per-particle strain tensors.
		const PropertyPtr& strainTensors() const { return _strainTensors; }

		/// Returns the property storage that contains the computed per-particle deformation gradient tensors.
		const PropertyPtr& deformationGradients() const { return _deformationGradients; }

		/// Returns the property storage that contains the computed per-particle deformation gradient tensors.
		const PropertyPtr& nonaffineSquaredDisplacements() const { return _nonaffineSquaredDisplacements; }

		/// Returns the property storage that contains the selection of invalid particles.
		const PropertyPtr& invalidParticles() const { return _invalidParticles; }

		/// Returns the property storage that contains the computed rotations.
		const PropertyPtr& rotations() const { return _rotations; }

		/// Returns the property storage that contains the computed stretch tensors.
		const PropertyPtr& stretchTensors() const { return _stretchTensors; }

		/// Returns the number of invalid particles for which the strain tensor could not be computed.
		size_t numInvalidParticles() const { return _numInvalidParticles.load(); }

		/// Increments the invalid particle counter by one.
		void addInvalidParticle() { _numInvalidParticles.fetchAndAddRelaxed(1); }

		/// Returns the property storage that contains the computed displacement vectors.
		const PropertyPtr& displacements() const { return _displacements; }

	private:

		const FloatType _cutoff;
		PropertyPtr _displacements;
		QAtomicInt _numInvalidParticles;
		const PropertyPtr _shearStrains;
		const PropertyPtr _volumetricStrains;
		const PropertyPtr _strainTensors;
		const PropertyPtr _deformationGradients;
		const PropertyPtr _nonaffineSquaredDisplacements;
		const PropertyPtr _invalidParticles;
		const PropertyPtr _rotations;
		const PropertyPtr _stretchTensors;
		ParticleOrderingFingerprint _inputFingerprint;
	};

	/// Controls the cutoff radius for the neighbor lists.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, cutoff, setCutoff, PROPERTY_FIELD_MEMORIZE);

	/// Controls the whether atomic deformation gradient tensors should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateDeformationGradients, setCalculateDeformationGradients);

	/// Controls the whether atomic strain tensors should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateStrainTensors, setCalculateStrainTensors);

	/// Controls the whether non-affine displacements should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateNonaffineSquaredDisplacements, setCalculateNonaffineSquaredDisplacements);

	/// Controls the whether local rotations should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateRotations, setCalculateRotations);

	/// Controls the whether atomic stretch tensors should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateStretchTensors, setCalculateStretchTensors);

	/// Controls the whether particles, for which the strain tensor could not be computed, are selected.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, selectInvalidParticles, setSelectInvalidParticles);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
