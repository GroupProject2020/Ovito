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
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/io/FileSource.h>
#include <core/dataset/animation/AnimationSettings.h>
#include "CombineParticleSetsModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(CombineParticleSetsModifier);
DEFINE_REFERENCE_FIELD(CombineParticleSetsModifier, secondaryDataSource);
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
		throwException(tr("No particle data has been provided that can be merged."));

	// Get the state.
	SharedFuture<PipelineFlowState> secondaryStateFuture = secondaryDataSource()->evaluate(time);
	
	// Wait for the data to become available.
	return secondaryStateFuture.then(executor(), [this, input = input, time](const PipelineFlowState& secondaryState) {

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

		size_t primaryParticleCount = posProperty->size();
		size_t secondaryParticleCount = secondaryPosProperty->size();
		size_t totalParticleCount = primaryParticleCount + secondaryParticleCount;

		// Extend all property arrays of primary dataset and copy data from secondary set if it contains a matching property.
		if(secondaryParticleCount != 0) {
			for(DataObject* obj : output.objects()) {
				if(OORef<ParticleProperty> prop = dynamic_object_cast<ParticleProperty>(obj)) {
					if(prop->size() != primaryParticleCount) continue;

					OORef<ParticleProperty> newProperty = poh.cloneIfNeeded(prop.get());
					newProperty->resize(totalParticleCount, true);

					// Find corresponding property in second dataset.
					ParticleProperty* secondProp;
					if(prop->type() != ParticleProperty::UserProperty)
						secondProp = ParticleProperty::findInState(secondaryState, prop->type());
					else
						secondProp = ParticleProperty::findInState(secondaryState, prop->name());
					if(secondProp && secondProp->size() == secondaryParticleCount && secondProp->componentCount() == newProperty->componentCount() && secondProp->dataType() == newProperty->dataType()) {
						OVITO_ASSERT(newProperty->stride() == secondProp->stride());
						memcpy(static_cast<char*>(newProperty->data()) + newProperty->stride() * primaryParticleCount, secondProp->constData(), newProperty->stride() * secondaryParticleCount);
					}

					// Combine particle types based on their names.
					if(secondProp && secondProp->elementTypes().empty() == false && newProperty->componentCount() == 1 && newProperty->dataType() == PropertyStorage::Int) {
						std::map<int,int> typeMap;
						for(ElementType* type2 : secondProp->elementTypes()) {
							ElementType* type1 = newProperty->elementType(type2->name());
							if(type1 == nullptr) {
								OORef<ElementType> type2clone = poh.cloneHelper().cloneObject(type2, false);
								type2clone->setId(newProperty->generateUniqueElementTypeId());
								newProperty->addElementType(type2clone);															
								typeMap.insert(std::make_pair(type2->id(), type2clone->id()));
							}
							else if(type1->id() != type2->id()) {
								typeMap.insert(std::make_pair(type2->id(), type1->id()));
							}
						}
						// Remap particle property values.
						if(typeMap.empty() == false) {
							for(int* p = newProperty->dataInt() + primaryParticleCount; p != newProperty->dataInt() + totalParticleCount; ++p) {
								auto iter = typeMap.find(*p);
								if(iter != typeMap.end()) *p = iter->second;
							}
						}
					}

					// Assign unique particle and molecule IDs.
					if(newProperty->type() == ParticleProperty::IdentifierProperty && primaryParticleCount != 0) {
						qlonglong maxId = *std::max_element(newProperty->constDataInt64(), newProperty->constDataInt64() + primaryParticleCount);
						std::iota(newProperty->dataInt64() + primaryParticleCount, newProperty->dataInt64() + totalParticleCount, maxId+1);
					}
					else if(newProperty->type() == ParticleProperty::MoleculeProperty && primaryParticleCount != 0) {
						qlonglong maxId = *std::max_element(newProperty->constDataInt64(), newProperty->constDataInt64() + primaryParticleCount);
						for(qlonglong* mol_id = newProperty->dataInt64() + primaryParticleCount; mol_id != newProperty->dataInt64() + totalParticleCount; ++mol_id)
							*mol_id += maxId;
					}
				}
			}
		}

		// Copy particle properties from second dataset which do not exist in the primary dataset yet.
		for(DataObject* obj : secondaryState.objects()) {
			if(OORef<ParticleProperty> prop = dynamic_object_cast<ParticleProperty>(obj)) {
				if(prop->size() != secondaryParticleCount) continue;

				// Check if the property already exists in the output.
				if(prop->type() != ParticleProperty::UserProperty) {
					if(ParticleProperty::findInState(output, prop->type()))
						continue;
				}
				else {
					if(ParticleProperty::findInState(output, prop->name()))
						continue;
				}

				// Put the property into the output.
				output.addObject(prop);
				OORef<ParticleProperty> newProperty = poh.cloneIfNeeded(prop.get());
				newProperty->resize(totalParticleCount, true);

				// Shift values of second dataset and reset values of first dataset to zero:
				if(primaryParticleCount != 0) {
					memmove(static_cast<char*>(newProperty->data()) + newProperty->stride() * primaryParticleCount, newProperty->constData(), newProperty->stride() * secondaryParticleCount);
					memset(newProperty->data(), 0, newProperty->stride() * primaryParticleCount);
				}
			}
		}

		// Merge bonds.
		BondProperty* primaryBondTopology = BondProperty::findInState(output, BondProperty::TopologyProperty);
		BondProperty* secondaryBondTopology = BondProperty::findInState(secondaryState, BondProperty::TopologyProperty);			

		// Merge bonds if present.
		if(primaryBondTopology || secondaryBondTopology) {
			
			size_t primaryBondCount = primaryBondTopology ? primaryBondTopology->size() : 0;
			size_t secondaryBondCount = secondaryBondTopology ? secondaryBondTopology->size() : 0;
			size_t totalBondCount = primaryBondCount + secondaryBondCount;
			poh.setOutputBondCount(totalBondCount);
			
			// Extend all property arrays of primary dataset and copy data from secondary set if it contains a matching property.
			if(secondaryBondCount != 0) {
				for(DataObject* obj : output.objects()) {
					if(OORef<BondProperty> prop = dynamic_object_cast<BondProperty>(obj)) {
						if(prop->size() != primaryBondCount) continue;

						OORef<BondProperty> newProperty = poh.cloneIfNeeded(prop.get());
						newProperty->resize(totalBondCount, true);

						// Find corresponding property in second dataset.
						BondProperty* secondProp;
						if(prop->type() != BondProperty::UserProperty)
							secondProp = BondProperty::findInState(secondaryState, prop->type());
						else
							secondProp = BondProperty::findInState(secondaryState, prop->name());
						if(secondProp && secondProp->size() == secondaryBondCount && secondProp->componentCount() == newProperty->componentCount() && secondProp->dataType() == newProperty->dataType()) {
							OVITO_ASSERT(newProperty->stride() == secondProp->stride());
							memcpy(static_cast<char*>(newProperty->data()) + newProperty->stride() * primaryBondCount, secondProp->constData(), newProperty->stride() * secondaryBondCount);
						}

						// Combine bond types based on their names.
						if(secondProp && secondProp->elementTypes().empty() == false && newProperty->componentCount() == 1 && newProperty->dataType() == PropertyStorage::Int) {
							std::map<int,int> typeMap;
							for(ElementType* type2 : secondProp->elementTypes()) {
								ElementType* type1 = newProperty->elementType(type2->name());
								if(type1 == nullptr) {
									OORef<ElementType> type2clone = poh.cloneHelper().cloneObject(type2, false);
									type2clone->setId(newProperty->generateUniqueElementTypeId());
									newProperty->addElementType(type2clone);															
									typeMap.insert(std::make_pair(type2->id(), type2clone->id()));
								}
								else if(type1->id() != type2->id()) {
									typeMap.insert(std::make_pair(type2->id(), type1->id()));
								}
							}
							// Remap bond property values.
							if(typeMap.empty() == false) {
								for(int* p = newProperty->dataInt() + primaryBondCount; p != newProperty->dataInt() + totalBondCount; ++p) {
									auto iter = typeMap.find(*p);
									if(iter != typeMap.end()) *p = iter->second;
								}
							}
						}

						// Shift particle indices.
						if(newProperty->type() == BondProperty::TopologyProperty && primaryParticleCount != 0) {
							for(size_t i = primaryBondCount; i < totalBondCount; i++) {
								newProperty->setInt64Component(i, 0, newProperty->getInt64Component(i, 0) + primaryParticleCount);
								newProperty->setInt64Component(i, 1, newProperty->getInt64Component(i, 1) + primaryParticleCount);
							}
						}
					}
				}
			}

			// Copy bond properties from second dataset which do not exist in the primary dataset yet.
			for(DataObject* obj : secondaryState.objects()) {
				if(OORef<BondProperty> prop = dynamic_object_cast<BondProperty>(obj)) {
					if(prop->size() != secondaryBondCount) continue;

					// Check if the property already exists in the output.
					if(prop->type() != BondProperty::UserProperty) {
						if(BondProperty::findInState(output, prop->type()))
							continue;
					}
					else {
						if(BondProperty::findInState(output, prop->name()))
							continue;
					}

					// Put the property into the output.
					output.addObject(prop);
					OORef<BondProperty> newProperty = poh.cloneIfNeeded(prop.get());
					newProperty->resize(totalBondCount, true);

					// Shift values of second dataset and reset values of first dataset to zero:
					if(primaryBondCount != 0) {
						memmove(static_cast<char*>(newProperty->data()) + newProperty->stride() * primaryBondCount, newProperty->constData(), newProperty->stride() * secondaryBondCount);
						memset(newProperty->data(), 0, newProperty->stride() * primaryBondCount);
					}
				}
			}
		}

		int secondaryFrame = secondaryState.sourceFrame();
		if(secondaryFrame < 0)
			secondaryFrame = dataset()->animationSettings()->timeToFrame(time);

		QString statusMessage = tr("Merged %1 existing particles with %2 particles from frame %3 of second dataset.")
				.arg(primaryParticleCount)
				.arg(secondaryParticleCount)
				.arg(secondaryFrame);
		output.setStatus(PipelineStatus(secondaryState.status().type(), statusMessage));
		return output;
	});
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
