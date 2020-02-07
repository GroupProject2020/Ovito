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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/pipeline/PipelineEvaluation.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "ReferenceConfigurationModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(ReferenceConfigurationModifier);
DEFINE_REFERENCE_FIELD(ReferenceConfigurationModifier, referenceConfiguration);
DEFINE_PROPERTY_FIELD(ReferenceConfigurationModifier, affineMapping);
DEFINE_PROPERTY_FIELD(ReferenceConfigurationModifier, useMinimumImageConvention);
DEFINE_PROPERTY_FIELD(ReferenceConfigurationModifier, useReferenceFrameOffset);
DEFINE_PROPERTY_FIELD(ReferenceConfigurationModifier, referenceFrameNumber);
DEFINE_PROPERTY_FIELD(ReferenceConfigurationModifier, referenceFrameOffset);
SET_PROPERTY_FIELD_LABEL(ReferenceConfigurationModifier, referenceConfiguration, "Reference Configuration");
SET_PROPERTY_FIELD_LABEL(ReferenceConfigurationModifier, affineMapping, "Affine mapping");
SET_PROPERTY_FIELD_LABEL(ReferenceConfigurationModifier, useMinimumImageConvention, "Use minimum image convention");
SET_PROPERTY_FIELD_LABEL(ReferenceConfigurationModifier, useReferenceFrameOffset, "Use reference frame offset");
SET_PROPERTY_FIELD_LABEL(ReferenceConfigurationModifier, referenceFrameNumber, "Reference frame number");
SET_PROPERTY_FIELD_LABEL(ReferenceConfigurationModifier, referenceFrameOffset, "Reference frame offset");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ReferenceConfigurationModifier, referenceFrameNumber, IntegerParameterUnit, 0);

// May be removed in a future version of OVITO:
IMPLEMENT_OVITO_CLASS(ReferenceConfigurationModifierApplication);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ReferenceConfigurationModifier::ReferenceConfigurationModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_affineMapping(NO_MAPPING),
    _useReferenceFrameOffset(false),
	_referenceFrameNumber(0),
	_referenceFrameOffset(-1),
	_useMinimumImageConvention(true)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool ReferenceConfigurationModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Determines the time interval over which a computed pipeline state will remain valid.
******************************************************************************/
TimeInterval ReferenceConfigurationModifier::validityInterval(const PipelineEvaluationRequest& request, const ModifierApplication* modApp) const
{
	TimeInterval iv = AsynchronousModifier::validityInterval(request, modApp);

	if(useReferenceFrameOffset()) {
		// Results will only be valid for the duration of the current frame when using a relative offset.
		iv.intersect(request.time());
	}

	return iv;
}

/******************************************************************************
* Asks the modifier for the set of animation time intervals that should be 
* cached by the downstream pipeline.
******************************************************************************/
void ReferenceConfigurationModifier::inputCachingHints(TimeIntervalUnion& cachingIntervals, ModifierApplication* modApp)
{
	AsynchronousModifier::inputCachingHints(cachingIntervals);

	// Only need to communicate caching hints when reference configuration is provided by the downstream pipeline.
	if(!referenceConfiguration()) {
		if(useReferenceFrameOffset()) {
			// When using a relative reference configuration, we need to build the corresponding set of shifted time intervals. 
			PipelineEvaluationRequest::CachingHintsList originalIntervals = cachingIntervals;
			for(const TimeInterval& iv : originalIntervals) {
				int startFrame = modApp->animationTimeToSourceFrame(iv.start());
				int endFrame = modApp->animationTimeToSourceFrame(iv.end());
				TimePoint shiftedStartTime = modApp->sourceFrameToAnimationTime(startFrame + referenceFrameOffset());
				TimePoint shiftedEndTime = modApp->sourceFrameToAnimationTime(endFrame + referenceFrameOffset());
				cachingIntervals.add(TimeInterval(shiftedStartTime, shiftedEndTime));
			}
		}
		else if() {
			// When using a static reference configuration, ask the downstream pipeline to cache the corresponding animation frame.
			cachingIntervals.add(modApp->sourceFrameToAnimationTime(referenceFrameNumber()));
		}
	}
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the 
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> ReferenceConfigurationModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// What is the reference frame number to use?
	TimeInterval validityInterval = input.stateValidity();
	int referenceFrame;
	if(useReferenceFrameOffset()) {
		// Determine the current frame, preferably from the attribute stored with the pipeline flow state.
		// If the source frame attribute is not present, fall back to inferring it from the current animation time.
		int currentFrame = input.data() ? input.data()->sourceFrame() : -1;
		if(currentFrame < 0)
			currentFrame = modApp->animationTimeToSourceFrame(time);

		// Use frame offset relative to current configuration.
		referenceFrame = currentFrame + referenceFrameOffset();

		// Results will only be valid for the duration of the current frame.
		validityInterval.intersect(time);
	}
	else {
		// Use a constant, user-specified frame as reference configuration.
		referenceFrame = referenceFrameNumber();
	}

	// Obtain the reference positions of the particles, either from the downstream pipeline or from a user-specified reference data source.
	SharedFuture<PipelineFlowState> refState;
	if(!referenceConfiguration()) {
		// Convert frame to animation time.
		TimePoint referenceTime = modApp->sourceFrameToAnimationTime(referenceFrame);
		
		// Set up the pipeline request for obtaining.
		PipelineEvaluationRequest referenceRequest(referenceTime, request.breakOnError());
		referenceRequest.

		// Issue the request to the downstream pipeline.
		refState = modApp->evaluateInput(referenceRequest);
	}
	else {
		if(referenceConfiguration()->numberOfSourceFrames() > 0) {
			if(referenceFrame < 0 || referenceFrame >= referenceConfiguration()->numberOfSourceFrames()) {
				if(referenceFrame > 0)
					throwException(tr("Requested reference frame number %1 is out of range. "
						"The loaded reference configuration contains only %2 frame(s).").arg(referenceFrame).arg(referenceConfiguration()->numberOfSourceFrames()));
				else
					throwException(tr("Requested reference frame %1 is out of range. Cannot perform calculation at the current animation time.").arg(referenceFrame));
			}
			refState = referenceConfiguration()->evaluate(PipelineEvaluationRequest(referenceConfiguration()->sourceFrameToAnimationTime(referenceFrame)));
		}
		else {
			// Create an empty state for the reference configuration if it is yet to be specified by the user.
			refState = Future<PipelineFlowState>::createImmediateEmplace();
		}
	}

	// Wait for the reference configuration to become available.
	return refState.then(executor(), [this, time, modApp, input = input, referenceFrame, validityInterval](const PipelineFlowState& referenceInput) {

		// Make sure the obtained reference configuration is valid and ready to use.
		if(referenceInput.status().type() == PipelineStatus::Error)
			throwException(tr("Reference configuration is not available: %1").arg(referenceInput.status().text()));
		if(!referenceInput)
			throwException(tr("Reference configuration has not been specified yet or is empty. Please pick a reference simulation file."));

		// Make sure we really got back the requested reference frame.
		if(referenceInput.data()->sourceFrame() != referenceFrame) {
			if(referenceFrame > 0)
				throwException(tr("Requested reference frame %1 is out of range. Make sure the loaded reference configuration file contains a sufficent number of frames.").arg(referenceFrame));
			else
				throwException(tr("Requested reference frame %1 is out of range. Cannot perform calculation at the current animation time.").arg(referenceFrame));
		}

		// Let subclass create the compute engine.
		return createEngineWithReference(time, modApp, std::move(input), referenceInput, validityInterval);
	});
}

/******************************************************************************
* Constructor.
******************************************************************************/
ReferenceConfigurationModifier::RefConfigEngineBase::RefConfigEngineBase(
	const TimeInterval& validityInterval,
	ConstPropertyPtr positions, const SimulationCell& simCell,
	ConstPropertyPtr refPositions, const SimulationCell& simCellRef,
	ConstPropertyPtr identifiers, ConstPropertyPtr refIdentifiers,
	AffineMappingType affineMapping, bool useMinimumImageConvention) :
	ComputeEngine(validityInterval),
	_positions(std::move(positions)),
	_simCell(simCell),
	_refPositions(std::move(refPositions)),
	_simCellRef(simCellRef),
	_identifiers(std::move(identifiers)),
	_refIdentifiers(std::move(refIdentifiers)),
	_affineMapping(affineMapping),
	_useMinimumImageConvention(useMinimumImageConvention)
{
	// Automatically disable PBCs in Z direction for 2D systems.
	if(_simCell.is2D()) {
		_simCell.setPbcFlags(_simCell.pbcFlags()[0], _simCell.pbcFlags()[1], false);
		// Make sure the matrix is invertible.
		AffineTransformation m = _simCell.matrix();
		m.column(2) = Vector3(0,0,1);
		_simCell.setMatrix(m);
		m = _simCellRef.matrix();
		m.column(2) = Vector3(0,0,1);
		_simCellRef.setMatrix(m);
	}

	if(affineMapping != NO_MAPPING) {
		if(std::abs(cell().matrix().determinant()) < FLOATTYPE_EPSILON || std::abs(refCell().matrix().determinant()) < FLOATTYPE_EPSILON)
			throw Exception(tr("Simulation cell is degenerate in either the deformed or the reference configuration."));
	}

	// PBCs flags of the current configuration always override PBCs flags
	// of the reference config.
	_simCellRef.setPbcFlags(_simCell.pbcFlags());
	_simCellRef.set2D(_simCell.is2D());

	// Precompute matrices for transforming points/vector between the two configurations.
	_refToCurTM = cell().matrix() * refCell().inverseMatrix();
	_curToRefTM = refCell().matrix() * cell().inverseMatrix();
}

/******************************************************************************
* Determines the mapping between particles in the reference configuration and
* the current configuration and vice versa.
******************************************************************************/
bool ReferenceConfigurationModifier::RefConfigEngineBase::buildParticleMapping(bool requireCompleteCurrentToRefMapping, bool requireCompleteRefToCurrentMapping)
{
	OVITO_ASSERT(task());

	// Build particle-to-particle index maps.
	_currentToRefIndexMap.resize(positions()->size());
	_refToCurrentIndexMap.resize(refPositions()->size());
	if(identifiers() && refIdentifiers()) {
		OVITO_ASSERT(identifiers()->size() == positions()->size());
		OVITO_ASSERT(refIdentifiers()->size() == refPositions()->size());

		// Build map of particle identifiers in reference configuration.
		std::map<qlonglong, size_t> refMap;
		size_t index = 0;
		ConstPropertyAccess<qlonglong> refIdentifiersArray(refIdentifiers());
		for(auto id : refIdentifiersArray) {
			if(refMap.insert(std::make_pair(id, index)).second == false)
				throw Exception(tr("Particles with duplicate identifiers detected in reference configuration."));
			index++;
		}

		if(task()->isCanceled())
			return false;

		// Check for duplicate identifiers in current configuration
		std::map<qlonglong, size_t> currentMap;
		index = 0;
		ConstPropertyAccess<qlonglong> identifiersArray(identifiers());
		for(auto id : identifiersArray) {
			if(currentMap.insert(std::make_pair(id, index)).second == false)
				throw Exception(tr("Particles with duplicate identifiers detected in current configuration."));
			index++;
		}

		if(task()->isCanceled())
			return false;

		// Build index maps.
		auto id = identifiersArray.cbegin();
		for(auto& mappedIndex : _currentToRefIndexMap) {
			auto iter = refMap.find(*id);
			if(iter != refMap.end())
				mappedIndex = iter->second;
			else if(requireCompleteCurrentToRefMapping)
				throw Exception(tr("Particle ID %1 does exist in the current configuration but not in the reference configuration.").arg(*id));
			else
				mappedIndex = std::numeric_limits<size_t>::max();
			++id;
		}

		if(task()->isCanceled())
			return false;

		id = refIdentifiersArray.cbegin();
		for(auto& mappedIndex : _refToCurrentIndexMap) {
			auto iter = currentMap.find(*id);
			if(iter != currentMap.end())
				mappedIndex = iter->second;
			else if(requireCompleteRefToCurrentMapping)
				throw Exception(tr("Particle ID %1 does exist in the reference configuration but not in the current configuration.").arg(*id));
			else
				mappedIndex = std::numeric_limits<size_t>::max();
			++id;
		}
	}
	else {
		// Deformed and reference configuration must contain the same number of particles.
		if(positions()->size() != refPositions()->size())
			throw Exception(tr("Cannot perform calculation. Numbers of particles in reference configuration and current configuration do not match."));

		// When particle identifiers are not available, assume the storage order of particles in the
		// reference configuration and the current configuration are the same and use trivial 1-to-1 mapping.
		std::iota(_refToCurrentIndexMap.begin(), _refToCurrentIndexMap.end(), size_t(0));
		std::iota(_currentToRefIndexMap.begin(), _currentToRefIndexMap.end(), size_t(0));
	}

	return !task()->isCanceled();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
