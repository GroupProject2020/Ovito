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
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/core/dataset/DataSet.h>
#include "BondsComputePropertyModifierDelegate.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(BondsComputePropertyModifierDelegate);

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> BondsComputePropertyModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
    if(const ParticlesObject* particles = input.getObject<ParticlesObject>()) {
        if(particles->bonds())
       		return { DataObjectReference(&ParticlesObject::OOClass()) };
    }
    return {};
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
std::shared_ptr<ComputePropertyModifierDelegate::PropertyComputeEngine> BondsComputePropertyModifierDelegate::createEngine(
				TimePoint time,
				const PipelineFlowState& input,
				const PropertyContainer* container,
				PropertyPtr outputProperty,
				ConstPropertyPtr selectionProperty,
				QStringList expressions)
{
	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ComputeEngine>(
			input.stateValidity(),
			time,
			std::move(outputProperty),
			container,
			std::move(selectionProperty),
			std::move(expressions),
			dataset()->animationSettings()->timeToFrame(time),
			input);
}

/******************************************************************************
* Constructor.
******************************************************************************/
BondsComputePropertyModifierDelegate::ComputeEngine::ComputeEngine(
		const TimeInterval& validityInterval,
		TimePoint time,
		PropertyPtr outputProperty,
		const PropertyContainer* container,
		ConstPropertyPtr selectionProperty,
		QStringList expressions,
		int frameNumber,
		const PipelineFlowState& input) :
	ComputePropertyModifierDelegate::PropertyComputeEngine(
			validityInterval,
			time,
			input,
			container,
			std::move(outputProperty),
			std::move(selectionProperty),
			std::move(expressions),
			frameNumber,
			std::make_unique<BondExpressionEvaluator>()),
	_inputFingerprint(input.expectObject<ParticlesObject>())
{
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	const BondsObject* bonds = particles->expectBonds();
	_topology = bonds->getPropertyStorage(BondsObject::TopologyProperty);

	// Define 'BondLength' computed variable which yields the length of the current bond.
	if(ConstPropertyAccessAndRef<Point3> positions = particles->getProperty(ParticlesObject::PositionProperty)) {
		if(ConstPropertyAccessAndRef<ParticleIndexPair> topology = bonds->getProperty(BondsObject::TopologyProperty)) {
			ConstPropertyAccessAndRef<Vector3I> periodicImages = bonds->getProperty(BondsObject::PeriodicImageProperty);
			SimulationCell simCell;
			if(const SimulationCellObject* simCellObj = input.getObject<SimulationCellObject>())
				simCell = simCellObj->data();
			else
				periodicImages.reset();

			_evaluator->registerComputedVariable("BondLength", [positions=std::move(positions),topology=std::move(topology),periodicImages=std::move(periodicImages),simCell](size_t bondIndex) -> double {
				size_t index1 = topology[bondIndex][0];
				size_t index2 = topology[bondIndex][1];
				if(positions.size() > index1 && positions.size() > index2) {
					const Point3& p1 = positions[index1];
					const Point3& p2 = positions[index2];
					Vector3 delta = p2 - p1;
					if(periodicImages) {
						if(int dx = periodicImages[bondIndex][0]) delta += simCell.matrix().column(0) * (FloatType)dx;
						if(int dy = periodicImages[bondIndex][1]) delta += simCell.matrix().column(1) * (FloatType)dy;
						if(int dz = periodicImages[bondIndex][2]) delta += simCell.matrix().column(2) * (FloatType)dz;
					}
					return delta.length();
				}
				else return 0;
			},
			tr("dynamically calculated"));
		}
	}

	// Build list of particle properties that will be made available as expression variables.
	std::vector<ConstPropertyPtr> inputParticleProperties;
	for(const PropertyObject* prop : particles->properties()) {
		inputParticleProperties.push_back(prop->storage());
	}
	_evaluator->registerPropertyVariables(inputParticleProperties, 1, "@1.");
	_evaluator->registerPropertyVariables(inputParticleProperties, 2, "@2.");
}

/********************************§**********************************************
* Returns a human-readable text listing the input variables.
******************************************************************************/
QString BondsComputePropertyModifierDelegate::ComputeEngine::inputVariableTable() const
{
	QString table = ComputePropertyModifierDelegate::PropertyComputeEngine::inputVariableTable();
	table.append(QStringLiteral("<p><b>Accessing particle properties:</b><ul>"));
	table.append(QStringLiteral("<li>@1... (<i style=\"color: #555;\">property of first particle</i>)</li>"));
	table.append(QStringLiteral("<li>@2... (<i style=\"color: #555;\">property of second particle</i>)</li>"));
	table.append(QStringLiteral("</ul></p>"));
	return table;
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void BondsComputePropertyModifierDelegate::ComputeEngine::perform()
{
	task()->setProgressText(tr("Computing property '%1'").arg(outputProperty()->name()));

	task()->setProgressValue(0);
	task()->setProgressMaximum(outputProperty()->size());

	// Parallelized loop over all bonds.
	parallelForChunks(outputProperty()->size(), *task(), [this](size_t startIndex, size_t count, Task& promise) {
		ParticleExpressionEvaluator::Worker worker(*_evaluator);
		ConstPropertyAccess<ParticleIndexPair> topologyArray(_topology);

		size_t endIndex = startIndex + count;
		size_t componentCount = outputProperty()->componentCount();
		for(size_t bondIndex = startIndex; bondIndex < endIndex; bondIndex++) {

			// Update progress indicator.
			if((bondIndex % 1024) == 0)
				promise.incrementProgressValue(1024);

			// Exit if operation was canceled.
			if(promise.isCanceled())
				return;

			// Skip unselected bonds if requested.
			if(selectionArray() && !selectionArray()[bondIndex])
				continue;

			// Update values of particle property variables.
			if(topologyArray) {
				size_t particleIndex1 = topologyArray[bondIndex][0];
				size_t particleIndex2 = topologyArray[bondIndex][1];
				worker.updateVariables(1, particleIndex1);
				worker.updateVariables(2, particleIndex2);
			}

			for(size_t component = 0; component < componentCount; component++) {

				// Compute expression value.
				FloatType value = worker.evaluate(bondIndex, component);

				// Store results in output property.
				outputArray().set(bondIndex, component, value);
			}
		}
	});
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void BondsComputePropertyModifierDelegate::ComputeEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	if(_inputFingerprint.hasChanged(state.expectObject<ParticlesObject>()))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	PropertyComputeEngine::emitResults(time, modApp, state);
}

}	// End of namespace
}	// End of namespace
