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

#include <ovito/grid/Grid.h>
#include <ovito/stdobj/properties/PropertyExpressionEvaluator.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/core/dataset/DataSet.h>
#include "VoxelGridComputePropertyModifierDelegate.h"

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(VoxelGridComputePropertyModifierDelegate);

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> VoxelGridComputePropertyModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
	// Gather list of all VoxelGrid objects in the input data collection.
	QVector<DataObjectReference> objects;
	for(const ConstDataObjectPath& path : input.getObjectsRecursive(VoxelGrid::OOClass())) {
		objects.push_back(path);
	}
	return objects;
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
					outputProperty()->set<int>(voxelIndex, component, (int)value);
				}
				else if(outputProperty()->dataType() == PropertyStorage::Int64) {
					outputProperty()->set<qlonglong>(voxelIndex, component, (qlonglong)value);
				}
				else if(outputProperty()->dataType() == PropertyStorage::Float) {
					outputProperty()->set<FloatType>(voxelIndex, component, value);
				}
			}
		}
	});
}

}	// End of namespace
}	// End of namespace
