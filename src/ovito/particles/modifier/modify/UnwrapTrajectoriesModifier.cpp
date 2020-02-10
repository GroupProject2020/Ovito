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
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/concurrent/AsyncOperation.h>
#include "UnwrapTrajectoriesModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(UnwrapTrajectoriesModifier);

IMPLEMENT_OVITO_CLASS(UnwrapTrajectoriesModifierApplication);
SET_MODIFIER_APPLICATION_TYPE(UnwrapTrajectoriesModifier, UnwrapTrajectoriesModifierApplication);

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool UnwrapTrajectoriesModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Modifies the input data.
******************************************************************************/
Future<PipelineFlowState> UnwrapTrajectoriesModifier::evaluate(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input)
{
	if(input) {
		if(UnwrapTrajectoriesModifierApplication* unwrapModApp = dynamic_object_cast<UnwrapTrajectoriesModifierApplication>(modApp)) {

			// If the periodic image flags particle property is present, use it to unwrap particle positions.
			const ParticlesObject* inputParticles = input.expectObject<ParticlesObject>();
			if(inputParticles->getProperty(ParticlesObject::PeriodicImageProperty)) {
				PipelineFlowState output = input;
				unwrapModApp->unwrapParticleCoordinates(request.time(), output);
				return output;
			}

			// Without the periodic image flags information, we need to scan the entire particle trajectories
			// to make them continuous.
			return unwrapModApp->detectPeriodicCrossings(request.time()).then(unwrapModApp->executor(), [unwrapModApp, state = input, time = request.time()]() mutable {
				unwrapModApp->unwrapParticleCoordinates(time, state);
				return std::move(state);
			});
		}
	}
	return input;
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void UnwrapTrajectoriesModifier::evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	if(!state) return;

	// The pipeline system may call evaluateSynchronous() with an outdated trajectory frame, which doesn't match the current
	// animation time. This would lead to artifacts, because particles might get unwrapped even though they haven't crossed
	// a periodic cell boundary yet. To avoid this from happening, we try to determine the true animation time of the
	// current input data collection and use it for looking up the unwrap information.
	int sourceFrame = state.data()->sourceFrame();
	if(sourceFrame != -1)
		time = modApp->sourceFrameToAnimationTime(sourceFrame);

	if(UnwrapTrajectoriesModifierApplication* unwrapModApp = dynamic_object_cast<UnwrapTrajectoriesModifierApplication>(modApp)) {
		unwrapModApp->unwrapParticleCoordinates(time, state);
	}
}

/******************************************************************************
* Processes all frames of the input trajectory to detect periodic crossings 
* of the particles.
******************************************************************************/
SharedFuture<> UnwrapTrajectoriesModifierApplication::detectPeriodicCrossings(TimePoint time)
{
	if(_unwrapOperation.isValid() == false) {
		_unwrapOperation = AsyncOperation(dataset()->taskManager());
		_unwrapOperation.setProgressText(tr("Unwrapping particle trajectories"));

		// Reset the async operation when it gets canceled by the system.
		connect(_unwrapOperation.watcher(), &TaskWatcher::canceled, [this]() {
			_unwrapOperation.reset();
		});
		
		// Determine the remaining number of animation frames that need to be processed.
		_unwrapOperation.setProgressMaximum(numberOfSourceFrames());
		if(unwrappedUpToTime() != TimeNegativeInfinity()) {
			_unwrapOperation.setProgressValue(animationTimeToSourceFrame(unwrappedUpToTime()) + 1);
		}
		else {
			// Initialize working data structures.
			_previousPositions.clear();
			_previousCell = SimulationCell();
			_currentFlipState.fill(0);
		}

		// Starting the unwrap operation.
		_unwrapOperation.setStarted();
		fetchNextFrame();
	}
	return _unwrapOperation.sharedFuture();
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool UnwrapTrajectoriesModifierApplication::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::TargetChanged && source == input()) {
		_unwrappedUpToTime = TimeNegativeInfinity();
		_unwrapRecords.clear();
		_unflipRecords.clear();
		if(_unwrapOperation.isValid()) {
			_previousPositions.clear();
			_unwrapOperation.cancel();
			_unwrapOperation.reset();
		}
	}
	return ModifierApplication::referenceEvent(source, event);
}

/******************************************************************************
* Gets called when the data object of the node has been replaced.
******************************************************************************/
void UnwrapTrajectoriesModifierApplication::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(input)) {
		_unwrappedUpToTime = TimeNegativeInfinity();
		_unwrapRecords.clear();
		_unflipRecords.clear();
		if(_unwrapOperation.isValid()) {
			_previousPositions.clear();
			_unwrapOperation.cancel();
			_unwrapOperation.reset();
		}
	}
	ModifierApplication::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void UnwrapTrajectoriesModifierApplication::unwrapParticleCoordinates(TimePoint time, PipelineFlowState& state)
{
	const ParticlesObject* inputParticles = state.expectObject<ParticlesObject>();
	inputParticles->verifyIntegrity();

	// If the periodic image flags particle property is present, use it to unwrap particle positions.
	if(ConstPropertyAccess<Vector3I> particlePeriodicImageProperty = inputParticles->getProperty(ParticlesObject::PeriodicImageProperty)) {
		// Get current simulation cell.
		const SimulationCellObject* simCellObj = state.expectObject<SimulationCellObject>();
		const SimulationCell cell = simCellObj->data();

		// Make a modifiable copy of the particles object.
		ParticlesObject* outputParticles = state.expectMutableObject<ParticlesObject>();

		// Make a modifiable copy of the particle position property.
		PropertyAccess<Point3> posProperty = outputParticles->expectMutableProperty(ParticlesObject::PositionProperty);
		const Vector3I* pbcShift = particlePeriodicImageProperty.cbegin();
		for(Point3& p : posProperty) {
			p += cell.matrix() * Vector3(*pbcShift++);
		}

		// Unwrap bonds by adjusting their PBC shift vectors.
		if(outputParticles->bonds()) {
			if(ConstPropertyAccess<ParticleIndexPair> topologyProperty = outputParticles->bonds()->getProperty(BondsObject::TopologyProperty)) {
				outputParticles->makeBondsMutable();
				PropertyAccess<Vector3I> periodicImageProperty = outputParticles->bonds()->createProperty(BondsObject::PeriodicImageProperty, true);
				for(size_t bondIndex = 0; bondIndex < topologyProperty.size(); bondIndex++) {
					size_t particleIndex1 = topologyProperty[bondIndex][0];
					size_t particleIndex2 = topologyProperty[bondIndex][1];
					if(particleIndex1 >= particlePeriodicImageProperty.size() || particleIndex2 >= particlePeriodicImageProperty.size())
						continue;
					const Vector3I& particleShift1 = particlePeriodicImageProperty[particleIndex1];
					const Vector3I& particleShift2 = particlePeriodicImageProperty[particleIndex2];
					periodicImageProperty[bondIndex] += particleShift1 - particleShift2;
				}
			}
		}

		// After unwrapping the particles, the PBC image flags are obsolete.
		// It's time to remove the particle property.
		outputParticles->removeProperty(outputParticles->getProperty(ParticlesObject::PeriodicImageProperty));

		state.setStatus(tr("Unwrapping particle positions using stored PBC image information."));

		return;
	}

	// Check if periodic cell boundary crossing have been precomputed or not.
	if(time > unwrappedUpToTime()) {
		if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
			state.setStatus(PipelineStatus(PipelineStatus::Warning, tr("Please press 'Update' to unwrap the particle trajectories now.")));
		else
			throwException(tr("Particle crossings of periodic cell boundaries have not been determined yet. Cannot unwrap trajectories. Did you forget to call UnwrapTrajectoriesModifier.update()?"));
		return;
	}

	// Reverse any cell shear flips made by LAMMPS.
	if(!unflipRecords().empty() && time >= unflipRecords().front().first) {
		auto iter = unflipRecords().rbegin();
		while(iter->first > time) {
			++iter;
			OVITO_ASSERT(iter != unflipRecords().rend());
		}
		const std::array<int,3>& flipState = iter->second;
		SimulationCellObject* simCellObj = state.expectMutableObject<SimulationCellObject>();
		AffineTransformation cell = simCellObj->cellMatrix();
		cell.column(2) += cell.column(0) * flipState[1] + cell.column(1) * flipState[2];
		cell.column(1) += cell.column(0) * flipState[0];
		simCellObj->setCellMatrix(cell);
	}

	if(unwrapRecords().empty())
		return;

	// Get current simulation cell.
	const SimulationCellObject* simCellObj = state.expectObject<SimulationCellObject>();
	const SimulationCell cell = simCellObj->data();

	// Make a modifiable copy of the particles object.
	ParticlesObject* outputParticles = state.expectMutableObject<ParticlesObject>();

	// Make a modifiable copy of the particle position property.
	PropertyAccess<Point3> posProperty = outputParticles->expectMutableProperty(ParticlesObject::PositionProperty);

	// Get particle identifiers.
	ConstPropertyAccess<qlonglong> identifierProperty = outputParticles->getProperty(ParticlesObject::IdentifierProperty);
	if(identifierProperty && identifierProperty.size() != posProperty.size())
		identifierProperty.reset();

	// Compute unwrapped particle coordinates.
	qlonglong index = 0;
	for(Point3& p : posProperty) {
		auto range = unwrapRecords().equal_range(identifierProperty ? identifierProperty[index] : index);
		bool shifted = false;
		Vector3 pbcShift = Vector3::Zero();
		for(auto iter = range.first; iter != range.second; ++iter) {
			if(std::get<0>(iter->second) <= time) {
				pbcShift[std::get<1>(iter->second)] += std::get<2>(iter->second);
				shifted = true;
			}
		}
		if(shifted) {
			p += cell.matrix() * pbcShift;
		}
		index++;
	}

	// Unwrap bonds by adjusting their PBC shift vectors.
	if(outputParticles->bonds()) {
		if(ConstPropertyAccess<ParticleIndexPair> topologyProperty = outputParticles->bonds()->getProperty(BondsObject::TopologyProperty)) {
			outputParticles->makeBondsMutable();
			PropertyAccess<Vector3I> periodicImageProperty = outputParticles->bonds()->createProperty(BondsObject::PeriodicImageProperty, true);
			for(size_t bondIndex = 0; bondIndex < topologyProperty.size(); bondIndex++) {
				size_t particleIndex1 = topologyProperty[bondIndex][0];
				size_t particleIndex2 = topologyProperty[bondIndex][1];
				if(particleIndex1 >= posProperty.size() || particleIndex2 >= posProperty.size())
					continue;

				Vector3I& pbcShift = periodicImageProperty[bondIndex];
				auto range1 = unwrapRecords().equal_range(identifierProperty ? identifierProperty[particleIndex1] : particleIndex1);
				auto range2 = unwrapRecords().equal_range(identifierProperty ? identifierProperty[particleIndex2] : particleIndex2);
				for(auto iter = range1.first; iter != range1.second; ++iter) {
					if(std::get<0>(iter->second) <= time) {
						pbcShift[std::get<1>(iter->second)] += std::get<2>(iter->second);
					}
				}
				for(auto iter = range2.first; iter != range2.second; ++iter) {
					if(std::get<0>(iter->second) <= time) {
						pbcShift[std::get<1>(iter->second)] -= std::get<2>(iter->second);
					}
				}
			}
		}
	}
}

/******************************************************************************
* Requests the next trajectory frame from the downstream pipeline.
******************************************************************************/
void UnwrapTrajectoriesModifierApplication::fetchNextFrame()
{
	OVITO_ASSERT(_unwrapOperation.isValid());

	// Stop fetching frames if the operation has been canceled.
	if(_unwrapOperation.isCanceled()) {
		_unwrapOperation.reset();
		return;
	}

	// Determine the next frame number to fetch from the input trajectory.
	int nextFrame = 0;
	if(unwrappedUpToTime() != TimeNegativeInfinity())
		nextFrame = animationTimeToSourceFrame(unwrappedUpToTime()) + 1;

	// When we have reached the end of the input trajectory, we can stop the operation.
	if(nextFrame >= numberOfSourceFrames()) {
		_previousPositions.clear();
		_unwrapOperation.setFinished();
		return;
	}

	// Request the next frame from the input trajectory.
	TimePoint nextFrameTime = sourceFrameToAnimationTime(nextFrame);
	SharedFuture<PipelineFlowState> frameFuture = evaluateInput(PipelineEvaluationRequest(nextFrameTime));

	// Wait until input frame is ready.
	_unwrapOperation.waitForFutureAsync(std::move(frameFuture), executor(), true, [this, nextFrame, nextFrameTime](const SharedFuture<PipelineFlowState>& future) {
		try {	
			// If the pipeline evaluation has been canceled for some reason, we cancel the unwrapping
			// operation as well.
			if(future.isCanceled() || !_unwrapOperation.isValid() || _unwrapOperation.isFinished()) {
				_previousPositions.clear();
				if(_unwrapOperation.isValid())
					_unwrapOperation.cancel();
				_unwrapOperation.reset();
				return;
			}

			// Get the next frame and process it.
			const PipelineFlowState& state = future.result();
			processNextFrame(nextFrame, nextFrameTime, state);
			_unwrapOperation.incrementProgressValue(1);

			// Schedule the pipeline evaluation for the next frame.
			fetchNextFrame();
		}
		catch(const Exception& ex) {
			// In case of an error during pipeline evaluation or the unwrapping calculation, 
			// abort the operation and forward the exception to the pipeline.
			_unwrapOperation.captureException();
			_previousPositions.clear();
			_unwrapOperation.setFinished();
		}
	});
}

/******************************************************************************
* Calculates the information that is needed to unwrap particle coordinates.
******************************************************************************/
void UnwrapTrajectoriesModifierApplication::processNextFrame(int frame, TimePoint time, const PipelineFlowState& state)
{
	const SimulationCellObject* simCellObj = state.getObject<SimulationCellObject>();
	if(!simCellObj)
		throwException(tr("Input data contains no simulation cell information at frame %1.").arg(frame));
	SimulationCell cell = simCellObj->data();
	if(!cell.pbcFlags()[0] && !cell.pbcFlags()[1] && !cell.pbcFlags()[2])
		throwException(tr("No periodic boundary conditions set for the simulation cell."));
	const ParticlesObject* particles = state.getObject<ParticlesObject>();
	if(!particles)
		throwException(tr("Input data contains no particles at frame %1.").arg(frame));
	ConstPropertyAccess<Point3> posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	ConstPropertyAccess<qlonglong> identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);
	if(identifierProperty && identifierProperty.size() != posProperty.size())
		identifierProperty.reset();

	// Special handling of cell flips in LAMMPS, which occur whenever a tilt factor exceeds +/-50%.
	if(cell.matrix()(1,0) == 0 && cell.matrix()(2,0) == 0 && cell.matrix()(2,1) == 0 && cell.matrix()(0,0) > 0 && cell.matrix()(1,1) > 0) {
		if(_previousCell.matrix() != AffineTransformation::Zero()) {
			std::array<int,3> flipState = _currentFlipState;
			// Detect discontinuities in the three tilt factors of the cell.
			if(cell.pbcFlags()[0]) {
				FloatType xy1 = _previousCell.matrix()(0,1) / _previousCell.matrix()(0,0);
				FloatType xy2 = cell.matrix()(0,1) / cell.matrix()(0,0);
				if(int flip_xy = (int)std::round(xy2 - xy1))
					flipState[0] -= flip_xy;
				if(!cell.is2D()) {
					FloatType xz1 = _previousCell.matrix()(0,2) / _previousCell.matrix()(0,0);
					FloatType xz2 = cell.matrix()(0,2) / cell.matrix()(0,0);
					if(int flip_xz = (int)std::round(xz2 - xz1))
						flipState[1] -= flip_xz;
				}
			}
			if(cell.pbcFlags()[1] && !cell.is2D()) {
				FloatType yz1 = _previousCell.matrix()(1,2) / _previousCell.matrix()(1,1);
				FloatType yz2 = cell.matrix()(1,2) / cell.matrix()(1,1);
				if(int flip_yz = (int)std::round(yz2 - yz1))
					flipState[2] -= flip_yz;
			}
			// Emit a timeline record whever a flipping occurred.
			if(flipState != _currentFlipState)
				_unflipRecords.emplace_back(time, flipState);
			_currentFlipState = flipState;
		}
		_previousCell = cell;
		// Unflip current simulation cell.
		if(_currentFlipState != std::array<int,3>{{0,0,0}}) {
			AffineTransformation newCellMatrix = cell.matrix();
			newCellMatrix(0,1) += cell.matrix()(0,0) * _currentFlipState[0];
			newCellMatrix(0,2) += cell.matrix()(0,0) * _currentFlipState[1];
			newCellMatrix(1,2) += cell.matrix()(1,1) * _currentFlipState[2];
			cell.setMatrix(newCellMatrix);
		}
	}

	qlonglong index = 0;
	for(const Point3& p : posProperty) {
		Point3 rp = cell.absoluteToReduced(p);
		// Try to insert new position of particle into map.
		// If an old position already exists, insertion will fail and we can
		// test if the particle has crossed a periodic cell boundary.
		auto result = _previousPositions.insert(std::make_pair(identifierProperty ? identifierProperty[index] : index, rp));
		if(!result.second) {
			Vector3 delta = result.first->second - rp;
			for(size_t dim = 0; dim < 3; dim++) {
				if(cell.pbcFlags()[dim]) {
					int shift = (int)std::round(delta[dim]);
					if(shift != 0) {
						// Create a new record when particle has crossed a periodic cell boundary.
						_unwrapRecords.emplace(result.first->first, std::make_tuple(time, (qint8)dim, (qint16)shift));
					}
				}
			}
			result.first->second = rp;
		}
		index++;
	}

	_unwrappedUpToTime = time;
	setStatus(tr("Processed input trajectory frame %1 of %2.").arg(frame).arg(numberOfSourceFrames()));
}

/******************************************************************************
* Saves the class' contents to an output stream.
******************************************************************************/
void UnwrapTrajectoriesModifierApplication::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	ModifierApplication::saveToStream(stream, excludeRecomputableData);
	stream.beginChunk(0x02);
	stream << unwrappedUpToTime();
	stream.endChunk();
	stream.beginChunk(0x02);
	stream.writeSizeT(unwrapRecords().size());
	for(const auto& item : unwrapRecords()) {
		OVITO_STATIC_ASSERT((std::is_same<qlonglong, qint64>::value));
		stream << item.first;
		stream << std::get<0>(item.second);
		stream << std::get<1>(item.second);
		stream << std::get<2>(item.second);
	}
	stream.writeSizeT(unflipRecords().size());
	for(const auto& item : unflipRecords()) {
		stream << item.first;
		stream << std::get<0>(item.second);
		stream << std::get<1>(item.second);
		stream << std::get<2>(item.second);
	}
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from an input stream.
******************************************************************************/
void UnwrapTrajectoriesModifierApplication::loadFromStream(ObjectLoadStream& stream)
{
	ModifierApplication::loadFromStream(stream);
	stream.expectChunk(0x02);
	stream >> _unwrappedUpToTime;
	stream.closeChunk();
	int version = stream.expectChunkRange(0x01, 1);
	size_t numItems;
	stream.readSizeT(numItems);
	_unwrapRecords.reserve(numItems);
	for(size_t i = 0; i < numItems; i++) {
		UnwrapData::key_type particleId;
		std::tuple_element_t<0, UnwrapData::mapped_type> time;
		std::tuple_element_t<1, UnwrapData::mapped_type> dim;
		std::tuple_element_t<2, UnwrapData::mapped_type> direction;
		stream >> particleId >> time >> dim >> direction;
		_unwrapRecords.emplace(particleId, std::make_tuple(time, dim, direction));
	}
	if(version >= 1) {
		stream.readSizeT(numItems);
		_unflipRecords.reserve(numItems);
		for(size_t i = 0; i < numItems; i++) {
			UnflipData::value_type item;
			stream >> item.first >> item.second[0] >> item.second[1] >> item.second[2];
			_unflipRecords.push_back(item);
		}
	}
	stream.closeChunk();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
