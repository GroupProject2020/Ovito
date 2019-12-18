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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/concurrent/AsyncOperation.h>
#include "UnwrapTrajectoriesModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(UnwrapTrajectoriesModifier);

IMPLEMENT_OVITO_CLASS(UnwrapTrajectoriesModifierApplication);
DEFINE_PROPERTY_FIELD(UnwrapTrajectoriesModifierApplication, unwrappedUpToTime);
DEFINE_PROPERTY_FIELD(UnwrapTrajectoriesModifierApplication, unwrapRecords);
DEFINE_PROPERTY_FIELD(UnwrapTrajectoriesModifierApplication, unflipRecords);
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
	PipelineFlowState output = input;
	if(!output.isEmpty())
		unwrapParticleCoordinates(request.time(), modApp, output);
	return Future<PipelineFlowState>::createImmediate(std::move(output));
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
void UnwrapTrajectoriesModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	if(state.isEmpty()) return;

	// The pipeline system may call evaluatePreliminary() with an outdated trajectory frame, which doesn't match the current
	// animation time. This would lead to artifacts, because particles might get unwrapped even though they haven't crossed
	// a periodic cell boundary yet. To avoid this from happening, we try to determine the true animation time to which the
	// current input data collection belongs.
	int sourceFrame = state.data()->sourceFrame();
	if(sourceFrame != -1) {
		time = modApp->sourceFrameToAnimationTime(sourceFrame);
	}

	unwrapParticleCoordinates(time, modApp, state);
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
void UnwrapTrajectoriesModifier::unwrapParticleCoordinates(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	const ParticlesObject* inputParticles = state.expectObject<ParticlesObject>();
	inputParticles->verifyIntegrity();

	// If the periodic image flags particle property is present, use it to unwrap particle positions.
	if(const PropertyObject* particlePeriodicImageProperty = inputParticles->getProperty(ParticlesObject::PeriodicImageProperty)) {
		// Get current simulation cell.
		const SimulationCellObject* simCellObj = state.expectObject<SimulationCellObject>();
		const SimulationCell cell = simCellObj->data();

		// Make a modifiable copy of the particles object.
		ParticlesObject* outputParticles = state.expectMutableObject<ParticlesObject>();

		// Make a modifiable copy of the particle position property.
		PropertyPtr posProperty = outputParticles->expectMutableProperty(ParticlesObject::PositionProperty)->modifiableStorage();
		const Vector3I* pbcShift = particlePeriodicImageProperty->constDataVector3I();
		for(Point3& p : posProperty->point3Range()) {
			p += cell.matrix() * Vector3(*pbcShift++);
		}

		// Unwrap bonds by adjusting their PBC shift vectors.
		if(outputParticles->bonds()) {
			if(ConstPropertyPtr topologyProperty = outputParticles->bonds()->getPropertyStorage(BondsObject::TopologyProperty)) {
				outputParticles->makeBondsMutable();
				PropertyObject* periodicImageProperty = outputParticles->bonds()->createProperty(BondsObject::PeriodicImageProperty, true);
				for(size_t bondIndex = 0; bondIndex < topologyProperty->size(); bondIndex++) {
					size_t particleIndex1 = topologyProperty->getInt64Component(bondIndex, 0);
					size_t particleIndex2 = topologyProperty->getInt64Component(bondIndex, 1);
					if(particleIndex1 >= particlePeriodicImageProperty->size() || particleIndex2 >= particlePeriodicImageProperty->size())
						continue;
					const Vector3I& particleShift1 = particlePeriodicImageProperty->getVector3I(particleIndex1);
					const Vector3I& particleShift2 = particlePeriodicImageProperty->getVector3I(particleIndex2);
					periodicImageProperty->dataVector3I()[bondIndex] += particleShift1 - particleShift2;
				}
			}
		}

		// After unwrapping the particles, the PBC image flags are obsolete.
		// It's time to remove the particle property.
		outputParticles->removeProperty(particlePeriodicImageProperty);

		state.setStatus(tr("Unwrapping particle positions using stored image information."));

		return;
	}

	// Obtain the precomputed list of periodic cell crossings from the modifier application that is needed to determine
	// the unwrapped particle positions.
	UnwrapTrajectoriesModifierApplication* myModApp = dynamic_object_cast<UnwrapTrajectoriesModifierApplication>(modApp);
	if(!myModApp) return;

	// Check if periodic cell boundary crossing have been precomputed or not.
	if(time > myModApp->unwrappedUpToTime()) {
		if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
			state.setStatus(PipelineStatus(PipelineStatus::Warning, tr("Please press 'Update' to unwrap the particle trajectories now.")));
		else
			throwException(tr("Particle crossings of periodic cell boundaries have not been determined yet. Cannot unwrap trajectories. Did you forget to call UnwrapTrajectoriesModifier.update()?"));
		return;
	}

	// Reverse any cell shear flips made by LAMMPS.
	const UnwrapTrajectoriesModifierApplication::UnflipData& unflipRecords = myModApp->unflipRecords();
	if(!unflipRecords.empty() && time >= unflipRecords.front().first) {
		auto iter = unflipRecords.rbegin();
		while(iter->first > time) {
			++iter;
			OVITO_ASSERT(iter != unflipRecords.rend());
		}
		const std::array<int,3>& flipState = iter->second;
		SimulationCellObject* simCellObj = state.expectMutableObject<SimulationCellObject>();
		AffineTransformation cell = simCellObj->cellMatrix();
		cell.column(2) += cell.column(0) * flipState[1] + cell.column(1) * flipState[2];
		cell.column(1) += cell.column(0) * flipState[0];
		simCellObj->setCellMatrix(cell);
	}

	const UnwrapTrajectoriesModifierApplication::UnwrapData& unwrapRecords = myModApp->unwrapRecords();
	state.setStatus(PipelineStatus(PipelineStatus::Success, tr("Detected %1 periodic cell boundary crossing(s) of particle trajectories.").arg(unwrapRecords.size())));
	if(unwrapRecords.empty()) return;

	state.setStatus(tr("Unwrapping particle positions based on detected boundary traversals."));

	// Get current simulation cell.
	const SimulationCellObject* simCellObj = state.expectObject<SimulationCellObject>();
	const SimulationCell cell = simCellObj->data();

	// Make a modifiable copy of the particles object.
	ParticlesObject* outputParticles = state.expectMutableObject<ParticlesObject>();

	// Make a modifiable copy of the particle position property.
	PropertyPtr posProperty = outputParticles->expectMutableProperty(ParticlesObject::PositionProperty)->modifiableStorage();

	// Get particle identifiers.
	ConstPropertyPtr identifierProperty = outputParticles->getPropertyStorage(ParticlesObject::IdentifierProperty);
	if(identifierProperty && identifierProperty->size() != posProperty->size())
		identifierProperty = nullptr;

	// Compute unwrapped particle coordinates.
	qlonglong index = 0;
	for(Point3& p : posProperty->point3Range()) {
		auto range = unwrapRecords.equal_range(identifierProperty ? identifierProperty->getInt64(index) : index);
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
		if(ConstPropertyPtr topologyProperty = outputParticles->bonds()->getPropertyStorage(BondsObject::TopologyProperty)) {
			outputParticles->makeBondsMutable();
			PropertyObject* periodicImageProperty = outputParticles->bonds()->createProperty(BondsObject::PeriodicImageProperty, true);
			for(size_t bondIndex = 0; bondIndex < topologyProperty->size(); bondIndex++) {
				size_t particleIndex1 = topologyProperty->getInt64Component(bondIndex, 0);
				size_t particleIndex2 = topologyProperty->getInt64Component(bondIndex, 1);
				if(particleIndex1 >= posProperty->size() || particleIndex2 >= posProperty->size())
					continue;

				Vector3I& pbcShift = periodicImageProperty->dataVector3I()[bondIndex];
				auto range1 = unwrapRecords.equal_range(identifierProperty ? identifierProperty->getInt64(particleIndex1) : particleIndex1);
				auto range2 = unwrapRecords.equal_range(identifierProperty ? identifierProperty->getInt64(particleIndex2) : particleIndex2);
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
* Recalculates the information that is needed to unwrap particle coordinates
******************************************************************************/
bool UnwrapTrajectoriesModifier::detectPeriodicCrossings(AsyncOperation&& operation)
{
	for(ModifierApplication* modApp : modifierApplications()) {
		UnwrapTrajectoriesModifierApplication* myModApp = dynamic_object_cast<UnwrapTrajectoriesModifierApplication>(modApp);
		if(!myModApp) continue;

		// Step through the animation frames.
		int num_frames = modApp->numberOfSourceFrames();
		operation.setProgressMaximum(num_frames);
		TimeInterval interval = { modApp->sourceFrameToAnimationTime(0), modApp->sourceFrameToAnimationTime(num_frames-1) };
		std::unordered_map<qlonglong,Point3> previousPositions;
		SimulationCell previousCell;
		UnwrapTrajectoriesModifierApplication::UnwrapData unwrapRecords;
		UnwrapTrajectoriesModifierApplication::UnflipData unflipRecords;
		std::array<int,3> currentFlipState{{0,0,0}};
		for(int frame = 0; frame < num_frames; frame++) {
			TimePoint time = modApp->sourceFrameToAnimationTime(frame);
			operation.setProgressText(tr("Unwrapping particle trajectories (frame %1 of %2)").arg(frame+1).arg(num_frames));

			SharedFuture<PipelineFlowState> stateFuture = myModApp->evaluateInput(PipelineEvaluationRequest(time));
			if(!operation.waitForFuture(stateFuture))
				return false;

			const PipelineFlowState& state = stateFuture.result();
			const SimulationCellObject* simCellObj = state.getObject<SimulationCellObject>();
			if(!simCellObj)
				throwException(tr("Input data contains no simulation cell information at frame %1.").arg(frame));
			SimulationCell cell = simCellObj->data();
			if(!cell.pbcFlags()[0] && !cell.pbcFlags()[1] && !cell.pbcFlags()[2])
				throwException(tr("No periodic boundary conditions set for the simulation cell."));
			const ParticlesObject* particles = state.getObject<ParticlesObject>();
			if(!particles)
				throwException(tr("Input data contains no particles at frame %1.").arg(frame));
			const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
			ConstPropertyPtr identifierProperty = particles->getPropertyStorage(ParticlesObject::IdentifierProperty);
			if(identifierProperty && identifierProperty->size() != posProperty->size())
				identifierProperty = nullptr;

			// Special handling of cell flips in LAMMPS, which occur whenever a tilt factor exceeds +/-50%.
			if(cell.matrix()(1,0) == 0 && cell.matrix()(2,0) == 0 && cell.matrix()(2,1) == 0 && cell.matrix()(0,0) > 0 && cell.matrix()(1,1) > 0) {
				if(previousCell.matrix() != AffineTransformation::Zero()) {
					std::array<int,3> flipState = currentFlipState;
					// Detect discontinuities in the three tilt factors of the cell.
					if(cell.pbcFlags()[0]) {
						FloatType xy1 = previousCell.matrix()(0,1) / previousCell.matrix()(0,0);
						FloatType xy2 = cell.matrix()(0,1) / cell.matrix()(0,0);
						if(int flip_xy = (int)std::round(xy2 - xy1))
							flipState[0] -= flip_xy;
						if(!cell.is2D()) {
							FloatType xz1 = previousCell.matrix()(0,2) / previousCell.matrix()(0,0);
							FloatType xz2 = cell.matrix()(0,2) / cell.matrix()(0,0);
							if(int flip_xz = (int)std::round(xz2 - xz1))
								flipState[1] -= flip_xz;
						}
					}
					if(cell.pbcFlags()[1] && !cell.is2D()) {
						FloatType yz1 = previousCell.matrix()(1,2) / previousCell.matrix()(1,1);
						FloatType yz2 = cell.matrix()(1,2) / cell.matrix()(1,1);
						if(int flip_yz = (int)std::round(yz2 - yz1))
							flipState[2] -= flip_yz;
					}
					// Emit a timeline record whever a flipping occurred.
					if(flipState != currentFlipState)
						unflipRecords.emplace_back(time, flipState);
					currentFlipState = flipState;
				}
				previousCell = cell;
				// Unflip current simulation cell.
				if(currentFlipState != std::array<int,3>{{0,0,0}}) {
					AffineTransformation newCellMatrix = cell.matrix();
					newCellMatrix(0,1) += cell.matrix()(0,0) * currentFlipState[0];
					newCellMatrix(0,2) += cell.matrix()(0,0) * currentFlipState[1];
					newCellMatrix(1,2) += cell.matrix()(1,1) * currentFlipState[2];
					cell.setMatrix(newCellMatrix);
				}
			}

			qlonglong index = 0;
			for(const Point3& p : posProperty->constPoint3Range()) {
				Point3 rp = cell.absoluteToReduced(p);
				// Try to insert new position of particle into map.
				// If an old position already exists, insertion will fail and we can
				// test if the particle has crossed a periodic cell boundary.
				auto result = previousPositions.insert(std::make_pair(identifierProperty ? identifierProperty->getInt64(index) : index, rp));
				if(!result.second) {
					Vector3 delta = result.first->second - rp;
					for(size_t dim = 0; dim < 3; dim++) {
						if(cell.pbcFlags()[dim]) {
							int shift = (int)std::round(delta[dim]);
							if(shift != 0) {
								// Create a new record when particle has crossed a periodic cell boundary.
								unwrapRecords.emplace(result.first->first, std::make_tuple(time, (qint8)dim, (qint16)shift));
							}
						}
					}
					result.first->second = rp;
				}
				index++;
			}

			operation.incrementProgressValue(1);
			if(operation.isCanceled())
				return false;
		}
		myModApp->setUnwrapRecords(std::move(unwrapRecords));
		myModApp->setUnflipRecords(std::move(unflipRecords));
		myModApp->setUnwrappedUpToTime(interval.end());
	}
	return true;
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
	stream >> _unwrappedUpToTime.mutableValue();
	stream.closeChunk();
	int version = stream.expectChunkRange(0x01, 1);
	size_t numItems;
	stream.readSizeT(numItems);
	for(size_t i = 0; i < numItems; i++) {
		UnwrapData::key_type particleId;
		std::tuple_element_t<0, UnwrapData::mapped_type> time;
		std::tuple_element_t<1, UnwrapData::mapped_type> dim;
		std::tuple_element_t<2, UnwrapData::mapped_type> direction;
		stream >> particleId >> time >> dim >> direction;
		_unwrapRecords.mutableValue().emplace(particleId, std::make_tuple(time, dim, direction));
	}
	if(version >= 1) {
		stream.readSizeT(numItems);
		for(size_t i = 0; i < numItems; i++) {
			UnflipData::value_type item;
			stream >> item.first >> item.second[0] >> item.second[1] >> item.second[2];
			_unflipRecords.mutableValue().push_back(item);
		}
	}
	stream.closeChunk();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
