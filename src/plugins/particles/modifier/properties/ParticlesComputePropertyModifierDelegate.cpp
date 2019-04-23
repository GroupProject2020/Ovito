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

#include <plugins/particles/Particles.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <core/dataset/DataSet.h>
#include "ParticlesComputePropertyModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(ParticlesComputePropertyModifierDelegate);
DEFINE_PROPERTY_FIELD(ParticlesComputePropertyModifierDelegate, neighborExpressions);
DEFINE_PROPERTY_FIELD(ParticlesComputePropertyModifierDelegate, cutoff);
DEFINE_PROPERTY_FIELD(ParticlesComputePropertyModifierDelegate, useMultilineFields);
SET_PROPERTY_FIELD_LABEL(ParticlesComputePropertyModifierDelegate, neighborExpressions, "Neighbor expressions");
SET_PROPERTY_FIELD_LABEL(ParticlesComputePropertyModifierDelegate, cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(ParticlesComputePropertyModifierDelegate, useMultilineFields, "Expand field(s)");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ParticlesComputePropertyModifierDelegate, cutoff, WorldParameterUnit, 0);

/******************************************************************************
* Constructs a new instance of this class.
******************************************************************************/
ParticlesComputePropertyModifierDelegate::ParticlesComputePropertyModifierDelegate(DataSet* dataset) : ComputePropertyModifierDelegate(dataset),
	_cutoff(3),
	_useMultilineFields(false)
{
}

/******************************************************************************
* Sets the number of vector components of the property to compute.
******************************************************************************/
void ParticlesComputePropertyModifierDelegate::setComponentCount(int componentCount)
{
	if(componentCount < neighborExpressions().size()) {
		setNeighborExpressions(neighborExpressions().mid(0, componentCount));
	}
	else if(componentCount > neighborExpressions().size()) {
		QStringList newList = neighborExpressions();
		while(newList.size() < componentCount)
			newList.append(QString());
		setNeighborExpressions(newList);
	}
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
std::shared_ptr<ComputePropertyModifierDelegate::PropertyComputeEngine> ParticlesComputePropertyModifierDelegate::createEngine(
				TimePoint time,
				const PipelineFlowState& input,
				const PropertyContainer* container,
				PropertyPtr outputProperty,
				ConstPropertyPtr selectionProperty,
				QStringList expressions)
{
	if(!neighborExpressions().empty() && neighborExpressions().size() != outputProperty->componentCount() && (neighborExpressions().size() != 1 || !neighborExpressions().front().isEmpty()))
		throwException(tr("Number of neighbor expressions that have been specified (%1) does not match the number of components per particle (%2) of the output property '%3'.")
			.arg(neighborExpressions().size()).arg(outputProperty->componentCount()).arg(outputProperty->name()));

	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	const PropertyObject* positions = particles->expectProperty(ParticlesObject::PositionProperty);

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ComputeEngine>(
			input.stateValidity(),
			time,
			std::move(outputProperty),
			container,
			std::move(selectionProperty),
			std::move(expressions),
			dataset()->animationSettings()->timeToFrame(time),
			input,
			positions->storage(),
			neighborExpressions(),
			cutoff());
}

/******************************************************************************
* Constructor.
******************************************************************************/
ParticlesComputePropertyModifierDelegate::ComputeEngine::ComputeEngine(
		const TimeInterval& validityInterval,
		TimePoint time,
		PropertyPtr outputProperty,
		const PropertyContainer* container,
		ConstPropertyPtr selectionProperty,
		QStringList expressions,
		int frameNumber,
		const PipelineFlowState& input,
		ConstPropertyPtr positions,
		QStringList neighborExpressions,
		FloatType cutoff) :
	ComputePropertyModifierDelegate::PropertyComputeEngine(
			validityInterval,
			time,
			input,
			container,
			std::move(outputProperty),
			std::move(selectionProperty),
			std::move(expressions),
			frameNumber,
			std::make_unique<ParticleExpressionEvaluator>()),
	_inputFingerprint(input.expectObject<ParticlesObject>()),
	_positions(std::move(positions)),
	_neighborExpressions(std::move(neighborExpressions)),
	_cutoff(cutoff),
	_neighborEvaluator(std::make_unique<ParticleExpressionEvaluator>())
{
	// Make sure we have the right number of expression strings.
	while(_neighborExpressions.size() < this->outputProperty()->componentCount())
		_neighborExpressions.append(QString());
	while(_neighborExpressions.size() > this->outputProperty()->componentCount())
		_neighborExpressions.pop_back();

	// Determine whether any neighbor expressions are present.
	_neighborMode = false;
	for(QString& expr : _neighborExpressions) {
		if(expr.trimmed().isEmpty()) expr = QStringLiteral("0");
		else if(expr.trimmed() != QStringLiteral("0")) _neighborMode = true;
	}

	_evaluator->registerGlobalParameter("Cutoff", _cutoff);
	_evaluator->registerGlobalParameter("NumNeighbors", 0);

	_neighborEvaluator->initialize(_neighborExpressions, input, _frameNumber);
	_neighborEvaluator->registerGlobalParameter("Cutoff", _cutoff);
	_neighborEvaluator->registerGlobalParameter("NumNeighbors", 0);
	_neighborEvaluator->registerGlobalParameter("Distance", 0);
	_neighborEvaluator->registerGlobalParameter("Delta.X", 0);
	_neighborEvaluator->registerGlobalParameter("Delta.Y", 0);
	_neighborEvaluator->registerGlobalParameter("Delta.Z", 0);
	_neighborEvaluator->registerIndexVariable(QStringLiteral("@") + _neighborEvaluator->indexVarName(), 1);

	// Build list of properties that will be made available as expression variables.
	std::vector<ConstPropertyPtr> inputProperties;
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	for(const PropertyObject* prop : particles->properties()) {
		inputProperties.push_back(prop->storage());
	}
	_neighborEvaluator->registerPropertyVariables(inputProperties, 1, "@");

	// Activate neighbor mode if NumNeighbors variable is referenced in tghe central particle expression(s).
	if(_evaluator->isVariableUsed("NumNeighbors"))
		_neighborMode = true;
}

/********************************§**********************************************
* Returns a human-readable text listing the input variables.
******************************************************************************/
QString ParticlesComputePropertyModifierDelegate::ComputeEngine::inputVariableTable() const
{
	QString table = ComputePropertyModifierDelegate::PropertyComputeEngine::inputVariableTable();
	table.append(QStringLiteral("<p><b>Neighbor expression variables:</b><ul>"));
	table.append(QStringLiteral("<li>Cutoff (<i style=\"color: #555;\">radius</i>)</li>"));
	table.append(QStringLiteral("<li>NumNeighbors (<i style=\"color: #555;\">of central particle</i>)</li>"));
	table.append(QStringLiteral("<li>Distance (<i style=\"color: #555;\">from central particle</i>)</li>"));
	table.append(QStringLiteral("<li>Delta.X (<i style=\"color: #555;\">neighbor vector component</i>)</li>"));
	table.append(QStringLiteral("<li>Delta.Y (<i style=\"color: #555;\">neighbor vector component</i>)</li>"));
	table.append(QStringLiteral("<li>Delta.Z (<i style=\"color: #555;\">neighbor vector component</i>)</li>"));
	table.append(QStringLiteral("<li>@... (<i style=\"color: #555;\">central particle properties</i>)</li>"));
	table.append(QStringLiteral("</ul></p>"));
	return table;
}

/******************************************************************************
* Determines whether any of the math expressions is explicitly time-dependent.
******************************************************************************/
QStringList ParticlesComputePropertyModifierDelegate::ComputeEngine::delegateInputVariableNames() const
{
	return _neighborEvaluator->inputVariableNames();
}

/******************************************************************************
* Determines whether the math expressions are time-dependent,
* i.e. if they reference the animation frame number.
******************************************************************************/
bool ParticlesComputePropertyModifierDelegate::ComputeEngine::isTimeDependent()
{
	if(ComputePropertyModifierDelegate::PropertyComputeEngine::isTimeDependent())
		return true;

	if(neighborMode())
		return _neighborEvaluator->isTimeDependent();

	return false;
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void ParticlesComputePropertyModifierDelegate::ComputeEngine::perform()
{
	task()->setProgressText(tr("Computing property '%1'").arg(outputProperty()->name()));

	// Only used when neighbor mode is active.
	CutoffNeighborFinder neighborFinder;
	if(neighborMode()) {
		// Prepare the neighbor list.
		if(!neighborFinder.prepare(_cutoff, *positions(), _neighborEvaluator->simCell(), nullptr, task().get()))
			return;
	}

	task()->setProgressValue(0);
	task()->setProgressMaximum(positions()->size());

	// Parallelized loop over all particles.
	parallelForChunks(positions()->size(), *task(), [this, &neighborFinder](size_t startIndex, size_t count, Task& promise) {
		ParticleExpressionEvaluator::Worker worker(*_evaluator);
		ParticleExpressionEvaluator::Worker neighborWorker(*_neighborEvaluator);

		// Obtain addresses where variables are stored so we can update their values
		// quickly later in the loop.
		double* distanceVar;
		double* deltaX;
		double* deltaY;
		double* deltaZ;
		double* selfNumNeighbors = nullptr;
		double* neighNumNeighbors = nullptr;
		if(neighborMode()) {
			distanceVar = neighborWorker.variableAddress("Distance");
			deltaX = neighborWorker.variableAddress("Delta.X");
			deltaY = neighborWorker.variableAddress("Delta.Y");
			deltaZ = neighborWorker.variableAddress("Delta.Z");
			selfNumNeighbors = worker.variableAddress("NumNeighbors");
			neighNumNeighbors = neighborWorker.variableAddress("NumNeighbors");
			if(!worker.isVariableUsed("NumNeighbors") && !neighborWorker.isVariableUsed("NumNeighbors"))
				selfNumNeighbors = neighNumNeighbors = nullptr;
		}

		size_t endIndex = startIndex + count;
		size_t componentCount = outputProperty()->componentCount();
		for(size_t particleIndex = startIndex; particleIndex < endIndex; particleIndex++) {

			// Update progress indicator.
			if((particleIndex % 1024) == 0)
				promise.incrementProgressValue(1024);

			// Exit if operation was canceled.
			if(promise.isCanceled())
				return;

			// Skip unselected particles if requested.
			if(selection() && !selection()->getInt(particleIndex))
				continue;

			if(selfNumNeighbors != nullptr) {
				// Determine number of neighbors (only if this value is being referenced in the expressions).
				int nneigh = 0;
				for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndex); !neighQuery.atEnd(); neighQuery.next())
					nneigh++;
				*selfNumNeighbors = *neighNumNeighbors = nneigh;
			}

			// Update neighbor expression variables that provide access to the properties of the central particle.
			if(neighborMode()) {
				neighborWorker.updateVariables(1, particleIndex);
			}

			for(size_t component = 0; component < componentCount; component++) {

				// Compute central term.
				FloatType value = worker.evaluate(particleIndex, component);

				if(neighborMode()) {
					// Compute and add neighbor terms.
					for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndex); !neighQuery.atEnd(); neighQuery.next()) {
						*distanceVar = sqrt(neighQuery.distanceSquared());
						*deltaX = neighQuery.delta().x();
						*deltaY = neighQuery.delta().y();
						*deltaZ = neighQuery.delta().z();
						value += neighborWorker.evaluate(neighQuery.current(), component);
					}
				}

				// Store results in output property.
				if(outputProperty()->dataType() == PropertyStorage::Int) {
					outputProperty()->setIntComponent(particleIndex, component, (int)value);
				}
				else if(outputProperty()->dataType() == PropertyStorage::Int64) {
					outputProperty()->setInt64Component(particleIndex, component, (qlonglong)value);
				}
				else if(outputProperty()->dataType() == PropertyStorage::Float) {
					outputProperty()->setFloatComponent(particleIndex, component, value);
				}
			}
		}
	});
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void ParticlesComputePropertyModifierDelegate::ComputeEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	if(_inputFingerprint.hasChanged(state.expectObject<ParticlesObject>()))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	PropertyComputeEngine::emitResults(time, modApp, state);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
