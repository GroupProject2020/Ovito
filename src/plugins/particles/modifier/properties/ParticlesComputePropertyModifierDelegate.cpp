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
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/stdobj/util/OutputHelper.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "ParticlesComputePropertyModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(ParticlesComputePropertyModifierDelegate);
DEFINE_PROPERTY_FIELD(ParticlesComputePropertyModifierDelegate, neighborModeEnabled);
DEFINE_PROPERTY_FIELD(ParticlesComputePropertyModifierDelegate, neighborExpressions);
DEFINE_PROPERTY_FIELD(ParticlesComputePropertyModifierDelegate, cutoff);
DEFINE_PROPERTY_FIELD(ParticlesComputePropertyModifierDelegate, useMultilineFields);
SET_PROPERTY_FIELD_LABEL(ParticlesComputePropertyModifierDelegate, neighborModeEnabled, "Include neighbor terms");
SET_PROPERTY_FIELD_LABEL(ParticlesComputePropertyModifierDelegate, neighborExpressions, "Neighbor expressions");
SET_PROPERTY_FIELD_LABEL(ParticlesComputePropertyModifierDelegate, cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(ParticlesComputePropertyModifierDelegate, useMultilineFields, "Expand input fields");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ParticlesComputePropertyModifierDelegate, cutoff, WorldParameterUnit, 0);

/******************************************************************************
* Constructs a new instance of this class.
******************************************************************************/
ParticlesComputePropertyModifierDelegate::ParticlesComputePropertyModifierDelegate(DataSet* dataset) : ComputePropertyModifierDelegate(dataset),
	_neighborExpressions(QStringList("0")), 
	_cutoff(3), 
	_neighborModeEnabled(false),
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
			newList.append("0");
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
				PropertyPtr outputProperty, 
				ConstPropertyPtr selectionProperty, 
				QStringList expressions, 
				bool initializeOutputProperty)
{
	// Get the particle positions.
	ParticleInputHelper pih(dataset(), input);
	ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);

	if(neighborModeEnabled() && neighborExpressions().size() != outputProperty->componentCount())
		throwException(tr("Number of neighbor expressions does not match component count of output property."));

	TimeInterval validityInterval = input.stateValidity();

	// Initialize output property with original values when computation is restricted to selected elements.
	if(initializeOutputProperty) {
		if(outputProperty->type() == ParticleProperty::ColorProperty) {
			std::vector<Color> colors = pih.inputParticleColors(time, validityInterval);
			OVITO_ASSERT(outputProperty->stride() == sizeof(Color) && outputProperty->size() == colors.size());
			memcpy(outputProperty->data(), colors.data(), outputProperty->stride() * outputProperty->size());
		}
		else if(outputProperty->type() == ParticleProperty::RadiusProperty) {
			std::vector<FloatType> radii = pih.inputParticleRadii(time, validityInterval);
			OVITO_ASSERT(outputProperty->stride() == sizeof(FloatType) && outputProperty->size() == radii.size());
			memcpy(outputProperty->data(), radii.data(), outputProperty->stride() * outputProperty->size());
		}
	}

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ComputeEngine>(
			validityInterval, 
			time, 
			std::move(outputProperty), 
			std::move(selectionProperty), 
			std::move(expressions), 
			dataset()->animationSettings()->timeToFrame(time), 
			input,
			posProperty->storage(),
			neighborExpressions(),
			neighborModeEnabled() ? cutoff() : 0);
}

/******************************************************************************
* Constructor.
******************************************************************************/
ParticlesComputePropertyModifierDelegate::ComputeEngine::ComputeEngine(
		const TimeInterval& validityInterval, 
		TimePoint time,
		PropertyPtr outputProperty, 
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
			ParticleProperty::OOClass(),
			std::move(outputProperty), 
			std::move(selectionProperty), 
			std::move(expressions), 
			frameNumber, 
			std::make_unique<ParticleExpressionEvaluator>()),
	_inputFingerprint(input),
	_positions(std::move(positions)),
	_neighborExpressions(std::move(neighborExpressions)), 
	_cutoff(cutoff),
	_neighborEvaluator(std::make_unique<ParticleExpressionEvaluator>())
{
	// Only used when neighbor mode is active.
	if(neighborMode()) {
		_evaluator->registerGlobalParameter("Cutoff", _cutoff);
		_evaluator->registerGlobalParameter("NumNeighbors", 0);
		OVITO_ASSERT(_neighborExpressions.size() == this->outputProperty()->componentCount());
		_neighborEvaluator->initialize(_neighborExpressions, input, _frameNumber);
		_neighborEvaluator->registerGlobalParameter("Cutoff", _cutoff);
		_neighborEvaluator->registerGlobalParameter("NumNeighbors", 0);
		_neighborEvaluator->registerGlobalParameter("Distance", 0);
		_neighborEvaluator->registerGlobalParameter("Delta.X", 0);
		_neighborEvaluator->registerGlobalParameter("Delta.Y", 0);
		_neighborEvaluator->registerGlobalParameter("Delta.Z", 0);

		_inputVariableTable.append(QStringLiteral("<p><b>Neighbor-related values:</b><ul>"));
		_inputVariableTable.append(QStringLiteral("<li>Cutoff (<i style=\"color: #555;\">radius</i>)</li>"));
		_inputVariableTable.append(QStringLiteral("<li>NumNeighbors (<i style=\"color: #555;\">of central particle</i>)</li>"));
		_inputVariableTable.append(QStringLiteral("<li>Distance (<i style=\"color: #555;\">from central particle</i>)</li>"));
		_inputVariableTable.append(QStringLiteral("<li>Delta.X (<i style=\"color: #555;\">neighbor vector component</i>)</li>"));
		_inputVariableTable.append(QStringLiteral("<li>Delta.Y (<i style=\"color: #555;\">neighbor vector component</i>)</li>"));
		_inputVariableTable.append(QStringLiteral("<li>Delta.X (<i style=\"color: #555;\">neighbor vector component</i>)</li>"));
		_inputVariableTable.append(QStringLiteral("</ul></p>"));
	}
}

/******************************************************************************
* Determines whether any of the math expressions is explicitly time-dependent.
******************************************************************************/
QStringList ParticlesComputePropertyModifierDelegate::ComputeEngine::delegateInputVariableNames() const
{
	if(neighborMode()) {
		return _neighborEvaluator->inputVariableNames();
	}
	else {
		return ComputePropertyModifierDelegate::PropertyComputeEngine::delegateInputVariableNames();
	}
}

/******************************************************************************
* Determines whether the math expressions are time-dependent, 
* i.e. if they reference the animation frame number.
******************************************************************************/
bool ParticlesComputePropertyModifierDelegate::ComputeEngine::isTimeDependent()
{
	if(ComputePropertyModifierDelegate::PropertyComputeEngine::isTimeDependent())
		return true;

	if(neighborMode()) {
		ParticleExpressionEvaluator::Worker worker(*_neighborEvaluator);
		if(worker.isVariableUsed("Frame"))
			return true;
	}

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
	parallelForChunks(positions()->size(), *task(), [this, &neighborFinder](size_t startIndex, size_t count, PromiseState& promise) {
		ParticleExpressionEvaluator::Worker worker(*_evaluator);
		ParticleExpressionEvaluator::Worker neighborWorker(*_neighborEvaluator);

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

			// Stop loop if canceled.
			if(promise.isCanceled())
				return;

			// Skip unselected particles if requested.
			if(selection() && !selection()->getInt(particleIndex))
				continue;

			if(selfNumNeighbors != nullptr) {
				// Determine number of neighbors.
				int nneigh = 0;
				for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndex); !neighQuery.atEnd(); neighQuery.next())
					nneigh++;
				*selfNumNeighbors = *neighNumNeighbors = nneigh;
			}

			for(size_t component = 0; component < componentCount; component++) {

				// Compute self term.
				FloatType value = worker.evaluate(particleIndex, component);

				if(neighborMode()) {
					// Compute sum of neighbor terms.
					for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndex); !neighQuery.atEnd(); neighQuery.next()) {
						*distanceVar = sqrt(neighQuery.distanceSquared());
						*deltaX = neighQuery.delta().x();
						*deltaY = neighQuery.delta().y();
						*deltaZ = neighQuery.delta().z();
						value += neighborWorker.evaluate(neighQuery.current(), component);
					}
				}

				// Store results.
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
PipelineFlowState ParticlesComputePropertyModifierDelegate::ComputeEngine::emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	if(_inputFingerprint.hasChanged(input))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	return PropertyComputeEngine::emitResults(time, modApp, input);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
