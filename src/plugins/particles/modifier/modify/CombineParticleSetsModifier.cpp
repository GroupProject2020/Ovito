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
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/io/FileSource.h>
#include <core/dataset/animation/AnimationSettings.h>
#include "CombineParticleSetsModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)


DEFINE_FLAGS_REFERENCE_FIELD(CombineParticleSetsModifier, secondaryDataSource, "SecondarySource", PipelineObject, PROPERTY_FIELD_NO_SUB_ANIM);
SET_PROPERTY_FIELD_LABEL(CombineParticleSetsModifier, secondaryDataSource, "Secondary source");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CombineParticleSetsModifier::CombineParticleSetsModifier(DataSet* dataset) : Modifier(dataset)
{


	// Create the file source object, which will be responsible for loading
	// and caching the data to be merged.
	OORef<FileSource> fileSource(new FileSource(dataset));

	// Disable automatic adjustment of animation length for the source object.
	fileSource->setAdjustAnimationIntervalEnabled(false);

	setSecondaryDataSource(fileSource);
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool CombineParticleSetsModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Modifies the input data.
******************************************************************************/
Future<PipelineFlowState> CombineParticleSetsModifier::evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get the secondary data source.
	if(!secondaryDataSource())
		throwException(tr("No particle data to be merged has been provided."));

	// Get the state.
	SharedFuture<PipelineFlowState> secondaryStateFuture = secondaryDataSource()->evaluate(time);
	
	// Wait for the data to become available.
	return secondaryStateFuture.then(executor(), [this, input = std::move(input), time](const PipelineFlowState& secondaryState) {

		PipelineFlowState output = input;
		ParticleInputHelper pih(dataset(), input);
		ParticleOutputHelper poh(dataset(), output);
		
		// Make sure the obtained dataset is valid and ready to use.
		if(secondaryState.status().type() == PipelineStatus::Error) {
			if(FileSource* fileSource = dynamic_object_cast<FileSource>(secondaryDataSource())) {
				if(fileSource->sourceUrl().isEmpty())
					throwException(tr("Please pick an input file to be merged."));
			}
			output.setStatus(secondaryState.status());
			return output;
		}

		if(secondaryState.isEmpty())
			throwException(tr("Secondary data source has not been specified yet or is empty. Please pick an input file to be merged."));

		// Merge validity intervals of primary and secondary datasets.
		output.intersectStateValidity(secondaryState.stateValidity());

		// Merge attributes of primary and secondary dataset.
		for(auto a = secondaryState.attributes().cbegin(); a != secondaryState.attributes().cend(); ++a)
			output.attributes().insert(a.key(), a.value());

		// Get the particle positions of secondary set.
		ParticleProperty* secondaryPosProperty = ParticleProperty::findInState(secondaryState, ParticleProperty::PositionProperty);
		if(!secondaryPosProperty)
			throwException(tr("Second dataset does not contain any particles."));

		// Get the positions from the primary dataset.
		ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);

		size_t primaryCount = posProperty->size();
		size_t secondaryCount = secondaryPosProperty->size();
		size_t finalCount = primaryCount + secondaryCount;

		// Extend all existing property arrays and copy data from secondary set if present.
		if(secondaryCount != 0) {
			for(DataObject* obj : output.objects()) {
				if(OORef<ParticleProperty> prop = dynamic_object_cast<ParticleProperty>(obj)) {
					if(prop->size() == primaryCount) {
						OORef<ParticleProperty> newProperty = poh.cloneIfNeeded(prop.get());
						newProperty->resize(finalCount, true);

						// Find corresponding property in second dataset.
						ParticleProperty* secondProp;
						if(prop->type() != ParticleProperty::UserProperty)
							secondProp = ParticleProperty::findInState(secondaryState, prop->type());
						else
							secondProp = ParticleProperty::findInState(secondaryState, prop->name());
						if(secondProp && secondProp->size() == secondaryCount && secondProp->componentCount() == newProperty->componentCount() && secondProp->dataType() == newProperty->dataType()) {
							OVITO_ASSERT(newProperty->stride() == secondProp->stride());
							memcpy(static_cast<char*>(newProperty->data()) + newProperty->stride() * primaryCount, secondProp->constData(), newProperty->stride() * secondaryCount);
						}

						// Assign unique IDs.
						if(newProperty->type() == ParticleProperty::IdentifierProperty && primaryCount != 0) {
							int maxId = *std::max_element(newProperty->constDataInt(), newProperty->constDataInt() + primaryCount);
							std::iota(newProperty->dataInt() + primaryCount, newProperty->dataInt() + finalCount, maxId+1);
						}
					}
				}
			}
		}

		// Merge bonds if present.
		if(BondsObject* secondaryBonds = secondaryState.findObject<BondsObject>()) {

			// Collect bond properties.
			std::vector<PropertyPtr> bondProperties;
			for(DataObject* obj : secondaryState.objects()) {
				if(BondProperty* prop = dynamic_object_cast<BondProperty>(obj)) {
					bondProperties.push_back(prop->storage());
				}
			}

			// Shift bond particle indices.
			BondsPtr shiftedBonds = std::make_shared<BondsStorage>(*secondaryBonds->storage());
			for(Bond& bond : *shiftedBonds) {
				bond.index1 += primaryCount;
				bond.index2 += primaryCount;
			}

			BondsDisplay* bondsDisplay = secondaryBonds->displayObjects().empty() ? nullptr : dynamic_object_cast<BondsDisplay>(secondaryBonds->displayObjects().front());
			poh.addBonds(std::move(shiftedBonds), bondsDisplay, std::move(bondProperties));
		}

		int secondaryFrame = secondaryState.sourceFrame();
		if(secondaryFrame < 0)
			secondaryFrame = dataset()->animationSettings()->timeToFrame(time);

		QString statusMessage = tr("Combined %1 existing particles with %2 particles from frame %3 of second dataset.")
				.arg(primaryCount)
				.arg(secondaryCount)
				.arg(secondaryFrame);
		output.setStatus(PipelineStatus(secondaryState.status().type(), statusMessage));
		return output;
	});
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
