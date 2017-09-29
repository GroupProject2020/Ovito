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

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <core/dataset/io/FileSource.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/animation/AnimationSettings.h>
#include "ReferenceConfigurationModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(ReferenceConfigurationModifier);
IMPLEMENT_OVITO_CLASS(ReferenceConfigurationModifierApplication);
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
bool ReferenceConfigurationModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Create a new modifier application that refers to this modifier instance.
******************************************************************************/
OORef<ModifierApplication> ReferenceConfigurationModifier::createModifierApplication()
{
	OORef<ModifierApplication> modApp = new ReferenceConfigurationModifierApplication(dataset());
	modApp->setModifier(this);
	return modApp;
}
	
/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> ReferenceConfigurationModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// What is the reference frame number to use?
	TimeInterval validityInterval = input.stateValidity();
	int referenceFrame;
	if(useReferenceFrameOffset()) {
		// Determine the current frame, preferably from the attribute stored with the pipeline flow state.
		// If the source frame attribute is not present, fall back to inferring it from the current animation time.
		int currentFrame = input.sourceFrame();
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

	// Get the reference positions of the particles.
	SharedFuture<PipelineFlowState> refState;

	// First, check our state cache in the ModifierApplication.
	if(ReferenceConfigurationModifierApplication* myModApp = dynamic_object_cast<ReferenceConfigurationModifierApplication>(modApp)) {
		if(myModApp->referenceCacheValidity().contains(time)) {
			refState = myModApp->referenceCache();
		}
	}

	// If not in cache, we need to obtain the reference state from the source.
	if(!refState.isValid()) {
		if(!referenceConfiguration()) {
			// Convert frame to animation time.
			refState = modApp->evaluateInput(modApp->sourceFrameToAnimationTime(referenceFrame));
		}
		else {
			// Special handling of FileSources, which allow us to directly access specific frames (not animation times).
			if(FileSource* fileSource = dynamic_object_cast<FileSource>(referenceConfiguration())) {
				if(fileSource->numberOfFrames() > 0) {
					if(referenceFrame < 0 || referenceFrame >= fileSource->numberOfFrames()) {
						if(referenceFrame > 0)
							throwException(tr("Requested reference frame number %1 is out of range. "
								"The loaded reference configuration contains only %2 frame(s).").arg(referenceFrame).arg(fileSource->numberOfFrames()));
						else
							throwException(tr("Requested reference frame %1 is out of range. Cannot perform calculation at the current animation time.").arg(referenceFrame));
					}
					refState = fileSource->requestFrame(referenceFrame);
				}
				else {
					// Create an empty state for the reference configuration if it is yet to be specified by the user.
					refState = Future<PipelineFlowState>::createImmediateEmplace();
				}
			}
			else {
				// General case: We have an arbitrary pipeline that produces the reference configuration.
				refState = referenceConfiguration()->evaluate(referenceConfiguration()->sourceFrameToAnimationTime(referenceFrame));
			}
		}
	}

	// Wait for the reference configuration to become available.
	return refState.then(executor(), [this, time, modApp, input = input, referenceFrame, validityInterval](const PipelineFlowState& referenceInput) {

		// Cache the reference configuration state in ModifierApplication.
		if(ReferenceConfigurationModifierApplication* myModApp = dynamic_object_cast<ReferenceConfigurationModifierApplication>(modApp)) {
			myModApp->updateReferenceCache(referenceInput, useReferenceFrameOffset() ? validityInterval : TimeInterval::infinite());
		}

		// Make sure the obtained reference configuration is valid and ready to use.
		if(referenceInput.status().type() == PipelineStatus::Error)
			throwException(tr("Reference configuration is not available: %1").arg(referenceInput.status().text()));
		if(referenceInput.isEmpty())
			throwException(tr("Reference configuration has not been specified yet or is empty. Please pick a reference simulation file."));

		// Make sure we really got back the requested reference frame.
		if(referenceInput.sourceFrame() != referenceFrame) {
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
	ConstPropertyPtr positions, const SimulationCell& simCell,
	ConstPropertyPtr refPositions, const SimulationCell& simCellRef,
	ConstPropertyPtr identifiers, ConstPropertyPtr refIdentifiers,
	AffineMappingType affineMapping, bool useMinimumImageConvention) :
	ComputeEngine(),
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
bool ReferenceConfigurationModifier::RefConfigEngineBase::buildParticleMapping()
{
	// Build particle-to-particle index maps.
	_currentToRefIndexMap.resize(positions()->size());
	_refToCurrentIndexMap.resize(refPositions()->size());
	if(identifiers() && refIdentifiers()) {
		OVITO_ASSERT(identifiers()->size() == positions()->size());
		OVITO_ASSERT(refIdentifiers()->size() == refPositions()->size());

		// Build map of particle identifiers in reference configuration.
		std::map<int, int> refMap;
		int index = 0;
		for(int id : refIdentifiers()->constIntRange()) {
			if(refMap.insert(std::make_pair(id, index)).second == false)
				throw Exception(tr("Particles with duplicate identifiers detected in reference configuration."));
			index++;
		}

		if(isCanceled())
			return false;

		// Check for duplicate identifiers in current configuration
		std::map<int, int> currentMap;
		index = 0;
		for(int id : identifiers()->constIntRange()) {
			if(currentMap.insert(std::make_pair(id, index)).second == false)
				throw Exception(tr("Particles with duplicate identifiers detected in current configuration."));
			index++;
		}

		if(isCanceled())
			return false;

		// Build index maps.
		const int* id = identifiers()->constDataInt();
		for(auto& mappedIndex : _currentToRefIndexMap) {
			auto iter = refMap.find(*id);
			if(iter != refMap.end())
				mappedIndex = iter->second;
			else
				mappedIndex = -1;
			++id;
		}

		if(isCanceled())
			return false;

		id = refIdentifiers()->constDataInt();
		for(auto& mappedIndex : _refToCurrentIndexMap) {
			auto iter = currentMap.find(*id);
			if(iter != currentMap.end())
				mappedIndex = iter->second;
			else
				mappedIndex = -1;
			++id;
		}
	}
	else {
		// Deformed and reference configuration must contain the same number of particles.
		if(positions()->size() != refPositions()->size())
			throw Exception(tr("Cannot perform calculation. Numbers of particles in reference configuration and current configuration do not match."));
		
		// When particle identifiers are not available, assume the storage order of particles in the 
		// reference configuration and the current configuration are the same and use trivial 1-to-1 mapping.
		std::iota(_refToCurrentIndexMap.begin(), _refToCurrentIndexMap.end(), 0);
		std::iota(_currentToRefIndexMap.begin(), _currentToRefIndexMap.end(), 0);
	}

	return !isCanceled();
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool ReferenceConfigurationModifierApplication::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(event->type() == ReferenceEvent::TargetChanged) {
		// Invalidate cached state.
		_referenceCache.clear();
		_cacheValidity.setEmpty();
	}
	return AsynchronousModifierApplication::referenceEvent(source, event);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
