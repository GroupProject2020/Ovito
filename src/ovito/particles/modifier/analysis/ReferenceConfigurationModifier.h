////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/stdobj/simcell/SimulationCell.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifierApplication.h>

namespace Ovito { namespace Particles {

/**
 * \brief Base class for analysis modifiers that require a reference configuration.
 */
class OVITO_PARTICLES_EXPORT ReferenceConfigurationModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class ReferenceConfigurationModifierClass : public ModifierClass
	{
	public:

		/// Inherit constructor from base metaclass.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(ReferenceConfigurationModifier, ReferenceConfigurationModifierClass)

public:

	/// Controls the type of coordinate mapping used in the calculation of displacement vectors.
    enum AffineMappingType {
		NO_MAPPING,
		TO_REFERENCE_CELL,
		TO_CURRENT_CELL
	};
	Q_ENUMS(AffineMappingType);

public:

	/// Constructor.
	ReferenceConfigurationModifier(DataSet* dataset);

	/// Determines the time interval over which a computed pipeline state will remain valid.
	virtual TimeInterval validityInterval(const PipelineEvaluationRequest& request, const ModifierApplication* modApp) const override;

	/// Asks the modifier for the set of animation time intervals that should be cached by the upstream pipeline.
	virtual void inputCachingHints(TimeIntervalUnion& cachingIntervals, ModifierApplication* modApp) override;

	/// Is called by the ModifierApplication to let the modifier adjust the time interval of a TargetChanged event 
	/// received from the upstream pipeline before it is propagated to the downstream pipeline.
	virtual void restrictInputValidityInterval(TimeInterval& iv) const override;

protected:

	/// Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngineInternal(const PipelineEvaluationRequest& request, ModifierApplication* modApp, PipelineFlowState input, const PipelineFlowState& referenceState, TimeInterval validityInterval) = 0;

	/// Base class for compute engines that make use of a reference configuration.
	class OVITO_PARTICLES_EXPORT RefConfigEngineBase : public ComputeEngine
	{
	public:

		/// Constructor.
		RefConfigEngineBase(const TimeInterval& validityInterval, ConstPropertyPtr positions, const SimulationCell& simCell,
				ConstPropertyPtr refPositions, const SimulationCell& simCellRef,
				ConstPropertyPtr identifiers, ConstPropertyPtr refIdentifiers,
				AffineMappingType affineMapping, bool useMinimumImageConvention);

		/// Releases data that is no longer needed.
		void releaseWorkingData() {
			_positions.reset();
			_refPositions.reset();
			_identifiers.reset();
			_refIdentifiers.reset();
			decltype(_currentToRefIndexMap){}.swap(_currentToRefIndexMap);
			decltype(_refToCurrentIndexMap){}.swap(_refToCurrentIndexMap);
		}

		/// Determines the mapping between particles in the reference configuration and
		/// the current configuration and vice versa.
		bool buildParticleMapping(bool requireCompleteCurrentToRefMapping, bool requireCompleteRefToCurrentMapping);

		/// Returns the property storage that contains the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Returns the property storage that contains the reference particle positions.
		const ConstPropertyPtr& refPositions() const { return _refPositions; }

		/// Returns the property storage that contains the input particle identifiers of the current configuration.
		const ConstPropertyPtr& identifiers() const { return _identifiers; }

		/// Returns the property storage that contains the input particle identifier of the reference configuration.
		const ConstPropertyPtr& refIdentifiers() const { return _refIdentifiers; }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

		/// Returns the reference simulation cell data.
		const SimulationCell& refCell() const { return _simCellRef; }

		AffineMappingType affineMapping() const { return _affineMapping; }

		bool useMinimumImageConvention() const { return _useMinimumImageConvention; }

		/// Returns the matrix for transforming points/vectors from the reference configuration to the current configuration.
		const AffineTransformation& refToCurTM() const { return _refToCurTM; }

		/// Returns the matrix for transforming points/vectors from the current configuration to the reference configuration.
		const AffineTransformation& curToRefTM() const { return _curToRefTM; }

		/// Returns the mapping of atom indices in the current config to the reference config.
		const std::vector<size_t>& currentToRefIndexMap() const { return _currentToRefIndexMap; }

		/// Returns the mapping of atom indices in the reference config to the current config.
		const std::vector<size_t>& refToCurrentIndexMap() const { return _refToCurrentIndexMap; }

	private:

		SimulationCell _simCell;
		SimulationCell _simCellRef;
		AffineTransformation _refToCurTM;
		AffineTransformation _curToRefTM;
		ConstPropertyPtr _positions;
		ConstPropertyPtr _refPositions;
		ConstPropertyPtr _identifiers;
		ConstPropertyPtr _refIdentifiers;
		const AffineMappingType _affineMapping;
		const bool _useMinimumImageConvention;
		std::vector<size_t> _currentToRefIndexMap;
		std::vector<size_t> _refToCurrentIndexMap;
	};

protected:

	/// The reference configuration.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(PipelineObject, referenceConfiguration, setReferenceConfiguration, PROPERTY_FIELD_NO_SUB_ANIM);

	/// Controls the whether the homogeneous deformation of the simulation cell is eliminated from the calculated displacement vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(AffineMappingType, affineMapping, setAffineMapping, PROPERTY_FIELD_MEMORIZE);

	/// Controls the whether the minimum image convention is used when calculating displacements.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useMinimumImageConvention, setUseMinimumImageConvention);

	/// Specify reference frame relative to current frame.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useReferenceFrameOffset, setUseReferenceFrameOffset);

	/// Absolute frame number from reference file to use when calculating displacement vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, referenceFrameNumber, setReferenceFrameNumber);

	/// Relative frame offset for reference coordinates.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, referenceFrameOffset, setReferenceFrameOffset, PROPERTY_FIELD_MEMORIZE);
};

/**
 * This class is no longer used as 02/2020. It's only here for backward compatibility with files written by older OVITO versions.
 * The class can be removed in the future.
 */
class OVITO_PARTICLES_EXPORT ReferenceConfigurationModifierApplication : public AsynchronousModifierApplication
{
	Q_OBJECT
	OVITO_CLASS(ReferenceConfigurationModifierApplication)

public:

	/// Constructor.
	Q_INVOKABLE ReferenceConfigurationModifierApplication(DataSet* dataset) : AsynchronousModifierApplication(dataset) {}
};

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::ReferenceConfigurationModifier::AffineMappingType);
Q_DECLARE_TYPEINFO(Ovito::Particles::ReferenceConfigurationModifier::AffineMappingType, Q_PRIMITIVE_TYPE);
