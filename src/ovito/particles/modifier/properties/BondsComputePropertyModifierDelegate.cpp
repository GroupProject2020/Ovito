///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/core/dataset/DataSet.h>
#include "BondsComputePropertyModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(BondsComputePropertyModifierDelegate);

/******************************************************************************
* Constructs a new instance of this class.
******************************************************************************/
BondsComputePropertyModifierDelegate::BondsComputePropertyModifierDelegate(DataSet* dataset) : ComputePropertyModifierDelegate(dataset)
{
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
	ConstPropertyPtr positions = particles->getPropertyStorage(ParticlesObject::PositionProperty);
	const BondsObject* bonds = particles->expectBonds();
	_topology = bonds->getPropertyStorage(BondsObject::TopologyProperty);
	ConstPropertyPtr periodicImages = bonds->getPropertyStorage(BondsObject::PeriodicImageProperty);

	// Define 'BondLength' computed variable which yields the length of the current bond.
	if(positions) {
		SimulationCell simCell;
		if(const SimulationCellObject* simCellObj = input.getObject<SimulationCellObject>())
			simCell = simCellObj->data();
		else
			periodicImages.reset();
		_evaluator->registerComputedVariable("BondLength", [positions,topology=_topology,periodicImages,simCell](size_t bondIndex) -> double {
			size_t index1 = topology->getInt64Component(bondIndex, 0);
			size_t index2 = topology->getInt64Component(bondIndex, 1);
			if(positions->size() > index1 && positions->size() > index2) {
				const Point3& p1 = positions->getPoint3(index1);
				const Point3& p2 = positions->getPoint3(index2);
				Vector3 delta = p2 - p1;
				if(periodicImages) {
					if(int dx = periodicImages->getIntComponent(bondIndex, 0)) delta += simCell.matrix().column(0) * (FloatType)dx;
					if(int dy = periodicImages->getIntComponent(bondIndex, 1)) delta += simCell.matrix().column(1) * (FloatType)dy;
					if(int dz = periodicImages->getIntComponent(bondIndex, 2)) delta += simCell.matrix().column(2) * (FloatType)dz;
				}
				return delta.length();
			}
			else return 0;
		},
		tr("dynamically calculated"));
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
			if(selection() && !selection()->getInt(bondIndex))
				continue;

			// Update values of particle property variables.
			if(_topology) {
				size_t particleIndex1 = _topology->getInt64Component(bondIndex, 0);
				size_t particleIndex2 = _topology->getInt64Component(bondIndex, 1);
				worker.updateVariables(1, particleIndex1);
				worker.updateVariables(2, particleIndex2);
			}

			for(size_t component = 0; component < componentCount; component++) {

				// Compute expression value.
				FloatType value = worker.evaluate(bondIndex, component);

				// Store results in output property.
				if(outputProperty()->dataType() == PropertyStorage::Int) {
					outputProperty()->setIntComponent(bondIndex, component, (int)value);
				}
				else if(outputProperty()->dataType() == PropertyStorage::Int64) {
					outputProperty()->setInt64Component(bondIndex, component, (qlonglong)value);
				}
				else if(outputProperty()->dataType() == PropertyStorage::Float) {
					outputProperty()->setFloatComponent(bondIndex, component, value);
				}
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

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
