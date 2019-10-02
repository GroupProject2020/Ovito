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

#include <ovito/grid/Grid.h>
#include <ovito/stdobj/properties/PropertyExpressionEvaluator.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/core/dataset/DataSet.h>
#include "VoxelGridComputePropertyModifierDelegate.h"

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(VoxelGridComputePropertyModifierDelegate);

/******************************************************************************
* Constructs a new instance of this class.
******************************************************************************/
VoxelGridComputePropertyModifierDelegate::VoxelGridComputePropertyModifierDelegate(DataSet* dataset) : ComputePropertyModifierDelegate(dataset)
{
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
std::shared_ptr<ComputePropertyModifierDelegate::PropertyComputeEngine> VoxelGridComputePropertyModifierDelegate::createEngine(
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
VoxelGridComputePropertyModifierDelegate::ComputeEngine::ComputeEngine(
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
			std::make_unique<PropertyExpressionEvaluator>())
{
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void VoxelGridComputePropertyModifierDelegate::ComputeEngine::perform()
{
	task()->setProgressText(tr("Computing property '%1'").arg(outputProperty()->name()));

	task()->setProgressValue(0);
	task()->setProgressMaximum(outputProperty()->size());

	// Parallelized loop over all voxels.
	parallelForChunks(outputProperty()->size(), *task(), [this](size_t startIndex, size_t count, Task& promise) {
		PropertyExpressionEvaluator::Worker worker(*_evaluator);

		size_t endIndex = startIndex + count;
		size_t componentCount = outputProperty()->componentCount();
		for(size_t voxelIndex = startIndex; voxelIndex < endIndex; voxelIndex++) {

			// Update progress indicator.
			if((voxelIndex % 1024) == 0)
				promise.incrementProgressValue(1024);

			// Exit if operation was canceled.
			if(promise.isCanceled())
				return;

			for(size_t component = 0; component < componentCount; component++) {

				// Compute expression value.
				FloatType value = worker.evaluate(voxelIndex, component);

				// Store results in output property.
				if(outputProperty()->dataType() == PropertyStorage::Int) {
					outputProperty()->setIntComponent(voxelIndex, component, (int)value);
				}
				else if(outputProperty()->dataType() == PropertyStorage::Int64) {
					outputProperty()->setInt64Component(voxelIndex, component, (qlonglong)value);
				}
				else if(outputProperty()->dataType() == PropertyStorage::Float) {
					outputProperty()->setFloatComponent(voxelIndex, component, value);
				}
			}
		}
	});
}

}	// End of namespace
}	// End of namespace
