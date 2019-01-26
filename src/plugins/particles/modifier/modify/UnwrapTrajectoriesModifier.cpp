///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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
#include <plugins/particles/objects/ParticlesObject.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/DataSet.h>
#include <core/app/Application.h>
#include "UnwrapTrajectoriesModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(UnwrapTrajectoriesModifier);

IMPLEMENT_OVITO_CLASS(UnwrapTrajectoriesModifierApplication);
DEFINE_PROPERTY_FIELD(UnwrapTrajectoriesModifierApplication, unwrappedUpToTime);
DEFINE_PROPERTY_FIELD(UnwrapTrajectoriesModifierApplication, unwrapRecords);
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
Future<PipelineFlowState> UnwrapTrajectoriesModifier::evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) 
{
	PipelineFlowState output = input;
	if(!output.isEmpty())
		unwrapParticleCoordinates(time, modApp, output);
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
	// Obtain the precomputed list of periodic cell crossings from the modifier application that is needed to determine
	// the unwrapped particle positions.
	UnwrapTrajectoriesModifierApplication* myModApp = dynamic_object_cast<UnwrapTrajectoriesModifierApplication>(modApp);
	if(!myModApp) return;

	// Check if periodic cell boundary crossing have been precomputed or not.
	if(time > myModApp->unwrappedUpToTime()) {
		if(!Application::instance()->scriptMode())
			state.setStatus(PipelineStatus(PipelineStatus::Warning, tr("Please press 'Update' to unwrap the particle trajectories now.")));
		else
			throwException(tr("Particle crossings of periodic cell boundaries have not been determined yet. Cannot unwrap trajectories. Did you forget to call UnwrapTrajectoriesModifier.update()?"));
		return;
	}

	const UnwrapTrajectoriesModifierApplication::UnwrapData& unwrapRecords = myModApp->unwrapRecords();
	state.setStatus(PipelineStatus(PipelineStatus::Success, tr("Detected %1 periodic cell boundary crossing(s) of particle trajectories.").arg(unwrapRecords.size())));
	if(unwrapRecords.empty()) return;

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
		TimeInterval interval = dataset()->animationSettings()->animationInterval();
		operation.setProgressMaximum(dataset()->animationSettings()->lastFrame() - dataset()->animationSettings()->firstFrame() + 1);
		std::unordered_map<qlonglong,Point3> previousPositions;
		UnwrapTrajectoriesModifierApplication::UnwrapData unwrapRecords; 
		for(TimePoint time = interval.start(); time <= interval.end(); time += dataset()->animationSettings()->ticksPerFrame()) {
			operation.setProgressText(tr("Unwrapping particle trajectories (frame %1 of %2)").arg(operation.progressValue()+1).arg(operation.progressMaximum()));

			SharedFuture<PipelineFlowState> stateFuture = myModApp->evaluateInput(time);
			if(!operation.waitForFuture(stateFuture))
				return false;
		
			const PipelineFlowState& state = stateFuture.result();
			const SimulationCellObject* simCellObj = state.getObject<SimulationCellObject>();
			if(!simCellObj)
				throwException(tr("Input data contains no simulation cell information at frame %1.").arg(dataset()->animationSettings()->timeToFrame(time)));
			const SimulationCell cell = simCellObj->data();
			if(!cell.pbcFlags()[0] && !cell.pbcFlags()[1] && !cell.pbcFlags()[2])
				throwException(tr("No periodic boundary conditions set for the simulation cell."));
			const ParticlesObject* particles = state.getObject<ParticlesObject>();
			if(!particles)
				throwException(tr("Input data contains no particles at frame %1.").arg(dataset()->animationSettings()->timeToFrame(time)));
			const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
			ConstPropertyPtr identifierProperty = particles->getPropertyStorage(ParticlesObject::IdentifierProperty);
			if(identifierProperty && identifierProperty->size() != posProperty->size())
				identifierProperty = nullptr;

			qlonglong index = 0;
			for(const Point3& p : posProperty->constPoint3Range()) {
				Point3 rp = cell.absoluteToReduced(p);
				// Try to insert new position of particle into map. 
				// If an old position already exists, insertion will fail and we can 
				// test if the particle has crossed a periodic cell boundary.
				auto result = previousPositions.insert(std::make_pair(identifierProperty ? identifierProperty->getInt64(index) : index, rp));
				if(!result.second) {
					for(size_t dim = 0; dim < 3; dim++) {
						if(cell.pbcFlags()[dim]) {
							int shift = (int)std::round(result.first->second[dim] - rp[dim]);
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
	stream.beginChunk(0x01);
	stream.writeSizeT(unwrapRecords().size());
	for(const auto& item : unwrapRecords()) {
		OVITO_STATIC_ASSERT((std::is_same<qlonglong, qint64>::value));
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
	stream.expectChunk(0x01);
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
	stream.closeChunk();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
