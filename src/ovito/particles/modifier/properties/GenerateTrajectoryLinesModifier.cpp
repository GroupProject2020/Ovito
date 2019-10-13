////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/particles/objects/TrajectoryObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/utilities/concurrent/Promise.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "GenerateTrajectoryLinesModifier.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(GenerateTrajectoryLinesModifier);
DEFINE_PROPERTY_FIELD(GenerateTrajectoryLinesModifier, onlySelectedParticles);
DEFINE_PROPERTY_FIELD(GenerateTrajectoryLinesModifier, useCustomInterval);
DEFINE_PROPERTY_FIELD(GenerateTrajectoryLinesModifier, customIntervalStart);
DEFINE_PROPERTY_FIELD(GenerateTrajectoryLinesModifier, customIntervalEnd);
DEFINE_PROPERTY_FIELD(GenerateTrajectoryLinesModifier, everyNthFrame);
DEFINE_PROPERTY_FIELD(GenerateTrajectoryLinesModifier, unwrapTrajectories);
DEFINE_REFERENCE_FIELD(GenerateTrajectoryLinesModifier, trajectoryVis);
SET_PROPERTY_FIELD_LABEL(GenerateTrajectoryLinesModifier, onlySelectedParticles, "Only selected particles");
SET_PROPERTY_FIELD_LABEL(GenerateTrajectoryLinesModifier, useCustomInterval, "Custom time interval");
SET_PROPERTY_FIELD_LABEL(GenerateTrajectoryLinesModifier, customIntervalStart, "Custom interval start");
SET_PROPERTY_FIELD_LABEL(GenerateTrajectoryLinesModifier, customIntervalEnd, "Custom interval end");
SET_PROPERTY_FIELD_LABEL(GenerateTrajectoryLinesModifier, everyNthFrame, "Every Nth frame");
SET_PROPERTY_FIELD_LABEL(GenerateTrajectoryLinesModifier, unwrapTrajectories, "Unwrap trajectories");
SET_PROPERTY_FIELD_UNITS(GenerateTrajectoryLinesModifier, customIntervalStart, TimeParameterUnit);
SET_PROPERTY_FIELD_UNITS(GenerateTrajectoryLinesModifier, customIntervalEnd, TimeParameterUnit);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GenerateTrajectoryLinesModifier, everyNthFrame, IntegerParameterUnit, 1);

IMPLEMENT_OVITO_CLASS(GenerateTrajectoryLinesModifierApplication);
DEFINE_REFERENCE_FIELD(GenerateTrajectoryLinesModifierApplication, trajectoryData);
SET_MODIFIER_APPLICATION_TYPE(GenerateTrajectoryLinesModifier, GenerateTrajectoryLinesModifierApplication);

/******************************************************************************
* Constructor.
******************************************************************************/
GenerateTrajectoryLinesModifier::GenerateTrajectoryLinesModifier(DataSet* dataset) : Modifier(dataset),
	_onlySelectedParticles(true),
	_useCustomInterval(false),
	_customIntervalStart(dataset->animationSettings()->animationInterval().start()),
	_customIntervalEnd(dataset->animationSettings()->animationInterval().end()),
	_everyNthFrame(1),
	_unwrapTrajectories(true)
{
	// Create the vis element for rendering the trajectories created by the modifier.
	setTrajectoryVis(new TrajectoryVis(dataset));
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool GenerateTrajectoryLinesModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
void GenerateTrajectoryLinesModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	// Inject the precomputed trajectory lines, which are stored in the modifier application, into the pipeline.
	if(GenerateTrajectoryLinesModifierApplication* myModApp = dynamic_object_cast<GenerateTrajectoryLinesModifierApplication>(modApp)) {
		if(myModApp->trajectoryData()) {
			state.addObject(myModApp->trajectoryData());
		}
	}
}

/******************************************************************************
* Updates the stored trajectories from the source particle object.
******************************************************************************/
bool GenerateTrajectoryLinesModifier::generateTrajectories(AsyncOperation&& operation)
{
	TimePoint currentTime = dataset()->animationSettings()->time();

	for(ModifierApplication* modApp : modifierApplications()) {
		GenerateTrajectoryLinesModifierApplication* myModApp = dynamic_object_cast<GenerateTrajectoryLinesModifierApplication>(modApp);
		if(!myModApp) continue;

		// Get input particles.
		SharedFuture<PipelineFlowState> stateFuture = myModApp->evaluateInput(currentTime);
		if(!operation.waitForFuture(stateFuture))
			return false;

		const PipelineFlowState& state = stateFuture.result();
		const ParticlesObject* particles = state.getObject<ParticlesObject>();
		if(!particles)
			throwException(tr("Cannot generate trajectory lines. The pipeline data contains no particles."));
		const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
		const PropertyObject* selectionProperty = particles->getProperty(ParticlesObject::SelectionProperty);
		const PropertyObject* identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);

		// Determine set of input particles in the current frame.
		std::vector<size_t> selectedIndices;
		std::set<qlonglong> selectedIdentifiers;
		if(onlySelectedParticles()) {
			if(selectionProperty) {
				if(identifierProperty && identifierProperty->size() == selectionProperty->size()) {
					const int* s = selectionProperty->constDataInt();
					for(auto id : identifierProperty->constInt64Range())
						if(*s++) selectedIdentifiers.insert(id);
				}
				else {
					const int* s = selectionProperty->constDataInt();
					for(size_t index = 0; index < selectionProperty->size(); index++)
						if(*s++) selectedIndices.push_back(index);
				}
			}
			if(selectedIndices.empty() && selectedIdentifiers.empty())
				throwException(tr("Cannot generate trajectory lines for selected particles. Particle selection has not been defined or selection set is empty."));
		}

		// Determine time interval over which trajectories should be generated.
		TimeInterval interval;
		if(useCustomInterval())
			interval = customInterval();
		else
			interval = TimeInterval(myModApp->sourceFrameToAnimationTime(0), myModApp->sourceFrameToAnimationTime(myModApp->numberOfSourceFrames() - 1));

		if(interval.duration() <= 0)
			throwException(tr("The current simulation sequence consists only of a single frame. Thus, no trajectory lines were created."));

		// Generate list of animation times at which particle positions should be sampled.
		QVector<TimePoint> sampleTimes;
		QVector<int> sampleFrames;
		for(TimePoint time = interval.start(); time <= interval.end(); time += everyNthFrame() * dataset()->animationSettings()->ticksPerFrame()) {
			sampleTimes.push_back(time);
			sampleFrames.push_back(dataset()->animationSettings()->timeToFrame(time));
		}
		operation.setProgressMaximum(sampleTimes.size());

		// Collect particle positions to generate trajectory line vertices.
		std::vector<Point3> pointData;
		std::vector<int> timeData;
		std::vector<qlonglong> idData;
		std::vector<SimulationCell> cells;
		int timeIndex = 0;
		for(TimePoint time : sampleTimes) {
			operation.setProgressText(tr("Generating trajectory lines (frame %1 of %2)").arg(operation.progressValue()+1).arg(operation.progressMaximum()));

			SharedFuture<PipelineFlowState> stateFuture = myModApp->evaluateInput(time);
			if(!operation.waitForFuture(stateFuture))
				return false;

			const PipelineFlowState& state = stateFuture.result();
			const ParticlesObject* particles = state.getObject<ParticlesObject>();
			if(!particles)
				throwException(tr("Input data contains no particles at frame %1.").arg(dataset()->animationSettings()->timeToFrame(time)));
			const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

			if(onlySelectedParticles()) {
				if(!selectedIdentifiers.empty()) {
					const PropertyObject* identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);
					if(!identifierProperty || identifierProperty->size() != posProperty->size())
						throwException(tr("Input particles do not possess identifiers at frame %1.").arg(dataset()->animationSettings()->timeToFrame(time)));

					// Create a mapping from IDs to indices.
					std::map<qlonglong,size_t> idmap;
					size_t index = 0;
					for(auto id : identifierProperty->constInt64Range())
						idmap.insert(std::make_pair(id, index++));

					for(auto id : selectedIdentifiers) {
						auto entry = idmap.find(id);
						if(entry != idmap.end()) {
							pointData.push_back(posProperty->getPoint3(entry->second));
							timeData.push_back(timeIndex);
							idData.push_back(id);
						}
					}
				}
				else {
					// Add coordinates of selected particles by index.
					for(auto index : selectedIndices) {
						if(index < posProperty->size()) {
							pointData.push_back(posProperty->getPoint3(index));
							timeData.push_back(timeIndex);
							idData.push_back(index);
						}
					}
				}
			}
			else {
				// Add coordinates of all particles.
				const PropertyObject* identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);
				if(identifierProperty && identifierProperty->size() == posProperty->size()) {
					// Particles with IDs.
					pointData.insert(pointData.end(), posProperty->constDataPoint3(), posProperty->constDataPoint3() + posProperty->size());
					idData.insert(idData.end(), identifierProperty->constDataInt64(), identifierProperty->constDataInt64() + identifierProperty->size());
					timeData.resize(timeData.size() + posProperty->size(), timeIndex);
				}
				else {
					// Particles without IDs.
					pointData.insert(pointData.end(), posProperty->constDataPoint3(), posProperty->constDataPoint3() + posProperty->size());
					idData.resize(idData.size() + posProperty->size());
					std::iota(idData.begin() + timeData.size(), idData.end(), 0);
					timeData.resize(timeData.size() + posProperty->size(), timeIndex);
				}
			}

			// Onbtain the simulation cell geometry at the current animation time.
			if(unwrapTrajectories()) {
				if(const SimulationCellObject* simCellObj = state.getObject<SimulationCellObject>()) {
					cells.push_back(simCellObj->data());
				}
				else {
					cells.push_back(SimulationCell());
				}
			}

			operation.incrementProgressValue(1);
			if(operation.isCanceled())
				return false;
			timeIndex++;
		}

		// Sort vertex data to obtain contineous trajectories.
		operation.setProgressMaximum(0);
		operation.setProgressText(tr("Sorting trajectory data"));
		std::vector<size_t> permutation(pointData.size());
		std::iota(permutation.begin(), permutation.end(), (size_t)0);
		std::sort(permutation.begin(), permutation.end(), [&idData, &timeData](size_t a, size_t b) {
			if(idData[a] < idData[b]) return true;
			if(idData[a] > idData[b]) return false;
			return timeData[a] < timeData[b];
		});
		if(operation.isCanceled())
			return false;

		// Do not create undo records for the following operations.
		UndoSuspender noUndo(dataset());

		// Store generated trajectory lines in the ModifierApplication.
		OORef<TrajectoryObject> trajObj = new TrajectoryObject(dataset());

		// Copy re-ordered trajectory points.
		PropertyObject* trajPosProperty = trajObj->createProperty(TrajectoryObject::PositionProperty, false, {}, pointData.size());
		auto piter = permutation.cbegin();
		for(Point3& p : trajPosProperty->point3Range()) {
			p = pointData[*piter++];
		}

		// Copy re-ordered trajectory time stamps.
		PropertyObject* trajTimeProperty = trajObj->createProperty(TrajectoryObject::SampleTimeProperty, false);
		piter = permutation.cbegin();
		for(int& t : trajTimeProperty->intRange()) {
			t = sampleFrames[timeData[*piter++]];
		}

		// Copy re-ordered trajectory ids.
		PropertyObject* trajIdProperty = trajObj->createProperty(TrajectoryObject::ParticleIdentifierProperty, false);
		piter = permutation.cbegin();
		for(qlonglong& id : trajIdProperty->int64Range()) {
			id = idData[*piter++];
		}

		if(operation.isCanceled())
			return false;

		// Unwrap trajectory vertices at periodic boundaries of the simulation cell.
		if(unwrapTrajectories() && pointData.size() >= 2 && !cells.empty() && cells.front().pbcFlags() != std::array<bool,3>{false, false, false}) {
			operation.setProgressText(tr("Unwrapping trajectory lines"));
			operation.setProgressMaximum(trajPosProperty->size() - 1);
			Point3* pos = trajPosProperty->dataPoint3();
			piter = permutation.cbegin();
			const qlonglong* id = trajIdProperty->constDataInt64();
			for(auto pos_end = pos + trajPosProperty->size() - 1; pos != pos_end; ++pos, ++piter, ++id) {
				if(!operation.incrementProgressValue())
					return false;
				if(id[0] == id[1]) {
					const SimulationCell& cell1 = cells[timeData[piter[0]]];
					const SimulationCell& cell2 = cells[timeData[piter[1]]];
					const Point3& p1 = pos[0];
					Point3 p2 = pos[1];
					for(size_t dim = 0; dim < 3; dim++) {
						if(cell1.pbcFlags()[dim]) {
							FloatType reduced1 = cell1.inverseMatrix().prodrow(p1, dim);
							FloatType reduced2 = cell2.inverseMatrix().prodrow(p2, dim);
							FloatType delta = reduced2 - reduced1;
							FloatType shift = std::floor(delta + FloatType(0.5));
							if(shift != 0) {
								pos[1] -= cell2.matrix().column(dim) * shift;
							}
						}
					}
				}
			}
		}

		trajObj->setVisElement(trajectoryVis());

		noUndo.reset(); // The trajectory line generation should be an undoable operation.

		myModApp->setTrajectoryData(trajObj);
	}
	return true;
}

}	// End of namespace
}	// End of namespace
