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
#include <core/dataset/animation/AnimationSettings.h>
#include <core/dataset/scene/ObjectNode.h>
#include <core/dataset/data/simcell/SimulationCellObject.h>
#include <core/app/Application.h>
#include <core/dataset/io/FileSource.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/utilities/concurrent/Promise.h>
#include <core/utilities/concurrent/TaskManager.h>
#include "TrajectoryGenerator.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(TrajectoryGenerator);	
DEFINE_REFERENCE_FIELD(TrajectoryGenerator, source);
DEFINE_PROPERTY_FIELD(TrajectoryGenerator, onlySelectedParticles);
DEFINE_PROPERTY_FIELD(TrajectoryGenerator, useCustomInterval);
DEFINE_PROPERTY_FIELD(TrajectoryGenerator, customIntervalStart);
DEFINE_PROPERTY_FIELD(TrajectoryGenerator, customIntervalEnd);
DEFINE_PROPERTY_FIELD(TrajectoryGenerator, everyNthFrame);
DEFINE_PROPERTY_FIELD(TrajectoryGenerator, unwrapTrajectories);
SET_PROPERTY_FIELD_LABEL(TrajectoryGenerator, source, "Source");
SET_PROPERTY_FIELD_LABEL(TrajectoryGenerator, onlySelectedParticles, "Only selected particles");
SET_PROPERTY_FIELD_LABEL(TrajectoryGenerator, useCustomInterval, "Custom time interval");
SET_PROPERTY_FIELD_LABEL(TrajectoryGenerator, customIntervalStart, "Custom interval start");
SET_PROPERTY_FIELD_LABEL(TrajectoryGenerator, customIntervalEnd, "Custom interval end");
SET_PROPERTY_FIELD_LABEL(TrajectoryGenerator, everyNthFrame, "Every Nth frame");
SET_PROPERTY_FIELD_LABEL(TrajectoryGenerator, unwrapTrajectories, "Unwrap trajectories");
SET_PROPERTY_FIELD_UNITS(TrajectoryGenerator, customIntervalStart, TimeParameterUnit);
SET_PROPERTY_FIELD_UNITS(TrajectoryGenerator, customIntervalEnd, TimeParameterUnit);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(TrajectoryGenerator, everyNthFrame, IntegerParameterUnit, 1);

/******************************************************************************
* Default constructor.
******************************************************************************/
TrajectoryGenerator::TrajectoryGenerator(DataSet* dataset) : StaticSource(dataset),
	_onlySelectedParticles(true), 
	_useCustomInterval(false),
	_customIntervalStart(dataset->animationSettings()->animationInterval().start()),
	_customIntervalEnd(dataset->animationSettings()->animationInterval().end()),
	_everyNthFrame(1), 
	_unwrapTrajectories(true)
{
}

/******************************************************************************
* Updates the stored trajectories from the source particle object.
******************************************************************************/
TrajectoryObject* TrajectoryGenerator::generateTrajectories(TaskManager& taskManager)
{
	Promise<> trajectoryTask = Promise<>::createSynchronous(&taskManager, true, true);
	TimePoint currentTime = dataset()->animationSettings()->time();

	// Get input particles.
	if(!source())
		throwException(tr("No input particle data object is selected from which trajectory lines can be generated."));
	SharedFuture<PipelineFlowState> stateFuture = source()->evaluatePipeline(currentTime);
	if(!taskManager.waitForTask(stateFuture))
		return nullptr;

	const PipelineFlowState& state = stateFuture.result();
	ParticleProperty* posProperty = ParticleProperty::findInState(state, ParticleProperty::PositionProperty);
	ParticleProperty* selectionProperty = ParticleProperty::findInState(state, ParticleProperty::SelectionProperty);
	ParticleProperty* identifierProperty = ParticleProperty::findInState(state, ParticleProperty::IdentifierProperty);

	if(!posProperty)
		throwException(tr("The input object contains no particles."));

	// Determine set of input particles.
	std::vector<int> selectedIndices;
	std::set<int> selectedIdentifiers;
	int particleCount = 0;
	if(onlySelectedParticles()) {
		if(selectionProperty) {
			if(identifierProperty && identifierProperty->size() == selectionProperty->size()) {
				const int* s = selectionProperty->constDataInt();
				for(int id : identifierProperty->constIntRange())
					if(*s++) selectedIdentifiers.insert(id);
				particleCount = selectedIdentifiers.size();
			}
			else {
				const int* s = selectionProperty->constDataInt();
				for(int index = 0; index < selectionProperty->size(); index++)
					if(*s++) selectedIndices.push_back(index);
				particleCount = selectedIndices.size();
			}
		}
	}
	else {
		if(identifierProperty) {
			for(int id : identifierProperty->constIntRange())
				selectedIdentifiers.insert(id);
			particleCount = selectedIdentifiers.size();
		}
		else {
			selectedIndices.resize(posProperty->size());
			std::iota(selectedIndices.begin(), selectedIndices.end(), 0);
			particleCount = selectedIndices.size();
		}
	}

	// Determine time interval over which trajectories should be generated.
	TimeInterval interval;
	if(useCustomInterval())
		interval = customInterval();
	else if(FileSource* fs = dynamic_object_cast<FileSource>(source()->sourceObject()))
		interval = TimeInterval(0, dataset()->animationSettings()->frameToTime(fs->numberOfFrames() - 1));
	else 
		interval = dataset()->animationSettings()->animationInterval();

	// Generate list of simulation frames at which particle positions should be sampled.
	QVector<TimePoint> sampleTimes;
	for(TimePoint time = interval.start(); time <= interval.end(); time += everyNthFrame() * dataset()->animationSettings()->ticksPerFrame()) {
		sampleTimes.push_back(time);
	}
	trajectoryTask.setProgressMaximum(sampleTimes.size());
	trajectoryTask.setProgressValue(0);

	// Sample particle positions to generate trajectory points.
	QVector<Point3> points;
	points.reserve(particleCount * sampleTimes.size());
	for(TimePoint time : sampleTimes) {
		trajectoryTask.setProgressText(tr("Loading frame %1").arg(dataset()->animationSettings()->timeToFrame(time)));

		SharedFuture<PipelineFlowState> stateFuture = source()->evaluatePipeline(time);
		if(!taskManager.waitForTask(stateFuture))
			return nullptr;
	
		const PipelineFlowState& state = stateFuture.result();
		ParticleProperty* posProperty = ParticleProperty::findInState(state, ParticleProperty::PositionProperty);
		if(!posProperty)
			throwException(tr("Input particle set is empty at frame %1.").arg(dataset()->animationSettings()->timeToFrame(time)));

		if(!onlySelectedParticles() && posProperty->size() != particleCount)
			throwException(tr("The current program version cannot create trajectory lines when the number of particles changes over time."));

		if(!selectedIdentifiers.empty()) {
			ParticleProperty* identifierProperty = ParticleProperty::findInState(state, ParticleProperty::IdentifierProperty);
			if(!identifierProperty || identifierProperty->size() != posProperty->size())
				throwException(tr("Input particles do not possess identifiers at frame %1.").arg(dataset()->animationSettings()->timeToFrame(time)));

			// Create a mapping from IDs to indices.
			std::map<int,int> idmap;
			int index = 0;
			for(int id : identifierProperty->constIntRange())
				idmap.insert(std::make_pair(id, index++));

			for(int id : selectedIdentifiers) {
				auto entry = idmap.find(id);
				if(entry == idmap.end())
					throwException(tr("Input particle with ID=%1 does not exist at frame %2. This program version cannot create trajectory lines when the number of particles changes over time.").arg(id).arg(dataset()->animationSettings()->timeToFrame(time)));
				points.push_back(posProperty->getPoint3(entry->second));
			}
		}
		else {
			for(int index : selectedIndices) {
				if(index >= posProperty->size())
					throwException(tr("Input particle at index %1 does not exist at frame %2. This program version cannot create trajectory lines when the number of particles changes over time.").arg(index+1).arg(dataset()->animationSettings()->timeToFrame(time)));
				points.push_back(posProperty->getPoint3(index));
			}
		}

		// Unwrap trajectory points at periodic boundaries of the simulation cell.
		if(unwrapTrajectories() && points.size() > particleCount) {
			if(SimulationCellObject* simCellObj = state.findObject<SimulationCellObject>()) {
				SimulationCell cell = simCellObj->data();
				if(cell.pbcFlags() != std::array<bool,3>{false, false, false}) {
					auto previousPos = points.cbegin() + (points.size() - 2 * particleCount);
					auto currentPos = points.begin() + (points.size() - particleCount);
					for(int i = 0; i < particleCount; i++, ++previousPos, ++currentPos) {
						Vector3 delta = cell.wrapVector(*currentPos - *previousPos);
						*currentPos = *previousPos + delta;
					}
					OVITO_ASSERT(currentPos == points.end());
				}
			}
		}

		trajectoryTask.setProgressValue(trajectoryTask.progressValue() + 1);
		if(trajectoryTask.isCanceled()) 
			return nullptr;
	}

	// Store generated trajectory lines.
	TrajectoryObject* trajObj = this->findObject<TrajectoryObject>();
	if(!trajObj) {
		trajObj = new TrajectoryObject(dataset());
		addDataObject(trajObj);
	}
	trajObj->setTrajectories(particleCount, points, sampleTimes);

	return trajObj;
}

}	// End of namespace
}	// End of namespace
