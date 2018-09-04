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

#include <plugins/stdmod/StdMod.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/data/AttributeDataObject.h>
#include <core/dataset/io/FileSource.h>
#include <core/dataset/animation/AnimationSettings.h>
#include "CombineDatasetsModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(CombineDatasetsModifier);
DEFINE_REFERENCE_FIELD(CombineDatasetsModifier, secondaryDataSource);
SET_PROPERTY_FIELD_LABEL(CombineDatasetsModifier, secondaryDataSource, "Secondary source");

IMPLEMENT_OVITO_CLASS(CombineDatasetsModifierDelegate);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CombineDatasetsModifier::CombineDatasetsModifier(DataSet* dataset) : MultiDelegatingModifier(dataset)
{
	// Generate the list of delegate objects.
	createModifierDelegates(CombineDatasetsModifierDelegate::OOClass());

	// Create the file source object, which will be responsible for loading
	// and caching the data to be merged.
	OORef<FileSource> fileSource(new FileSource(dataset));

	// Disable automatic adjustment of animation length for the source object.
	fileSource->setAdjustAnimationIntervalEnabled(false);

	setSecondaryDataSource(fileSource);
}

/******************************************************************************
* Modifies the input data.
******************************************************************************/
Future<PipelineFlowState> CombineDatasetsModifier::evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get the secondary data source.
	if(!secondaryDataSource())
		throwException(tr("No dataset to be merged has been provided."));

	// Get the state.
	SharedFuture<PipelineFlowState> secondaryStateFuture = secondaryDataSource()->evaluate(time);
	
	// Wait for the data to become available.
	return secondaryStateFuture.then(executor(), [this, input = input, time, modApp = OORef<ModifierApplication>(modApp)](const PipelineFlowState& secondaryState) {

		UndoSuspender noUndo(this);
		PipelineFlowState output = input;
		
		// Make sure the obtained dataset is valid and ready to use.
		if(secondaryState.status().type() == PipelineStatus::Error) {
			if(FileSource* fileSource = dynamic_object_cast<FileSource>(secondaryDataSource())) {
				if(fileSource->sourceUrls().empty())
					throwException(tr("Please pick an input file to be merged."));
			}
			output.setStatus(secondaryState.status());
			return output;
		}

		if(secondaryState.isEmpty())
			throwException(tr("Secondary data source has not been specified yet or is empty. Please pick an input file to be merged."));

		// Merge validity intervals of primary and secondary datasets.
		output.intersectStateValidity(secondaryState.stateValidity());

		// Merge global attributes of primary and secondary datasets.
		for(const DataObject* obj : secondaryState.objects()) {
			if(const AttributeDataObject* attribute = dynamic_object_cast<AttributeDataObject>(obj)) {
				if(!output.contains(attribute))
					output.addObject(attribute);
			}
		}

		// Let the delegates do their job.
		applyDelegates(input, output, time, modApp, { std::reference_wrapper<const PipelineFlowState>(secondaryState) });

		return output;
	});
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState CombineDatasetsModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = input;

	// Get the secondary data source.
	if(!secondaryDataSource())
		return output;

	// Acquire the state to be merged.
	const PipelineFlowState& secondaryState = secondaryDataSource()->evaluatePreliminary();
	if(secondaryState.isEmpty()) {
		return output;
	}

	// Merge validity intervals of primary and secondary datasets.
	output.intersectStateValidity(secondaryState.stateValidity());

	// Merge global attributes of primary and secondary datasets.
	for(const DataObject* obj : secondaryState.objects()) {
		if(const AttributeDataObject* attribute = dynamic_object_cast<AttributeDataObject>(obj)) {
			if(!output.contains(attribute))
				output.addObject(attribute);
		}
	}
	
	// Let the delegates do their job and merge the data objects of the two datasets.
	applyDelegates(input, output, time, modApp, { std::reference_wrapper<const PipelineFlowState>(secondaryState) });

	return output;
}

}	// End of namespace
}	// End of namespace
