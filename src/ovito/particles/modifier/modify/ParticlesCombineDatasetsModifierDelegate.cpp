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
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/BondsVis.h>
#include <ovito/core/oo/CloneHelper.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "ParticlesCombineDatasetsModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(ParticlesCombineDatasetsModifierDelegate);

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> ParticlesCombineDatasetsModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
	if(input.containsObject<ParticlesObject>())
		return { DataObjectReference(&ParticlesObject::OOClass()) };
	return {};
}

/******************************************************************************
* Modifies the input data.
******************************************************************************/
PipelineStatus ParticlesCombineDatasetsModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	// Get the secondary dataset.
	if(additionalInputs.empty())
		throwException(tr("No second dataset has been provided."));
	const PipelineFlowState& secondaryState = additionalInputs.front();

	// Get the particles from secondary dataset.
	const ParticlesObject* secondaryParticles = secondaryState.getObject<ParticlesObject>();
	if(!secondaryParticles)
		throwException(tr("Second dataset does not contain any particles."));
	const PropertyObject* secondaryPosProperty = secondaryParticles->expectProperty(ParticlesObject::PositionProperty);

	// Get the positions from the primary dataset.
	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();

	size_t primaryParticleCount = particles->elementCount();
	size_t secondaryParticleCount = secondaryParticles->elementCount();
	size_t totalParticleCount = primaryParticleCount + secondaryParticleCount;

	CloneHelper cloneHelper;

	// Extend all property arrays of primary dataset and copy data from secondary set if it contains a matching property.
	if(secondaryParticleCount != 0) {
		particles->setElementCount(totalParticleCount);
		for(PropertyObject* prop : particles->properties()) {
			OVITO_ASSERT(prop->size() == totalParticleCount);

			// Find corresponding property in second dataset.
			const PropertyObject* secondProp;
			if(prop->type() != ParticlesObject::UserProperty)
				secondProp = secondaryParticles->getProperty(prop->type());
			else
				secondProp = secondaryParticles->getProperty(prop->name());
			if(secondProp && secondProp->size() == secondaryParticleCount && secondProp->componentCount() == prop->componentCount() && secondProp->dataType() == prop->dataType()) {
				OVITO_ASSERT(prop->stride() == secondProp->stride());
				std::memcpy(static_cast<char*>(prop->data()) + prop->stride() * primaryParticleCount, secondProp->constData(), prop->stride() * secondaryParticleCount);
			}
			else if(prop->type() != ParticlesObject::UserProperty) {
				ConstDataObjectPath containerPath = { secondaryParticles };
				PropertyPtr temporaryProp = ParticlesObject::OOClass().createStandardStorage(secondaryParticles->elementCount(), prop->type(), true, containerPath);
				OVITO_ASSERT(temporaryProp->stride() == prop->stride());
				std::memcpy(static_cast<char*>(prop->data()) + prop->stride() * primaryParticleCount, temporaryProp->constData(), prop->stride() * secondaryParticleCount);
			}

			// Combine particle types based on their names.
			if(secondProp && secondProp->elementTypes().empty() == false && prop->componentCount() == 1 && prop->dataType() == PropertyStorage::Int) {
				std::map<int,int> typeMap;
				for(const ElementType* type2 : secondProp->elementTypes()) {
					if(!type2->name().isEmpty()) {
						const ElementType* type1 = prop->elementType(type2->name());
						if(type1 == nullptr) {
							OORef<ElementType> type2clone = cloneHelper.cloneObject(type2, false);
							type2clone->setNumericId(prop->generateUniqueElementTypeId());
							prop->addElementType(type2clone);
							typeMap.insert(std::make_pair(type2->numericId(), type2clone->numericId()));
						}
						else if(type1->numericId() != type2->numericId()) {
							typeMap.insert(std::make_pair(type2->numericId(), type1->numericId()));
						}
					}
					else if(!prop->elementType(type2->numericId())) {
						OORef<ElementType> type2clone = cloneHelper.cloneObject(type2, false);
						prop->addElementType(type2clone);
						OVITO_ASSERT(type2clone->numericId() == type2->numericId());
					}
				}
				// Remap particle property values.
				if(typeMap.empty() == false) {
					for(int* p = prop->dataInt() + primaryParticleCount; p != prop->dataInt() + totalParticleCount; ++p) {
						auto iter = typeMap.find(*p);
						if(iter != typeMap.end()) *p = iter->second;
					}
				}
			}

			// Assign unique particle and molecule IDs.
			if(prop->type() == ParticlesObject::IdentifierProperty && primaryParticleCount != 0) {
				qlonglong maxId = *std::max_element(prop->constDataInt64(), prop->constDataInt64() + primaryParticleCount);
				std::iota(prop->dataInt64() + primaryParticleCount, prop->dataInt64() + totalParticleCount, maxId+1);
			}
			else if(prop->type() == ParticlesObject::MoleculeProperty && primaryParticleCount != 0) {
				qlonglong maxId = *std::max_element(prop->constDataInt64(), prop->constDataInt64() + primaryParticleCount);
				for(qlonglong* mol_id = prop->dataInt64() + primaryParticleCount; mol_id != prop->dataInt64() + totalParticleCount; ++mol_id)
					*mol_id += maxId;
			}
		}
	}

	// Copy particle properties from second dataset which do not exist in the primary dataset yet.
	for(const PropertyObject* prop : secondaryParticles->properties()) {
		if(prop->size() != secondaryParticleCount) continue;

		// Check if the property already exists in the output.
		if(prop->type() != ParticlesObject::UserProperty) {
			if(particles->getProperty(prop->type()))
				continue;
		}
		else {
			if(particles->getProperty(prop->name()))
				continue;
		}

		// Put the property into the output.
		OORef<PropertyObject> clonedProperty = cloneHelper.cloneObject(prop, false);
		clonedProperty->resize(totalParticleCount, true);
		particles->addProperty(clonedProperty);

		// Shift values of second dataset and reset values of first dataset to zero:
		if(primaryParticleCount != 0) {
			std::memmove(static_cast<char*>(clonedProperty->data()) + clonedProperty->stride() * primaryParticleCount, clonedProperty->constData(), clonedProperty->stride() * secondaryParticleCount);
			std::memset(clonedProperty->data(), 0, clonedProperty->stride() * primaryParticleCount);
		}
	}

	// Merge bonds.
	const BondsObject* primaryBonds = particles->bonds();
	const BondsObject* secondaryBonds = secondaryParticles->bonds();
	const PropertyObject* primaryBondTopology = primaryBonds ? primaryBonds->getTopology() : nullptr;
	const PropertyObject* secondaryBondTopology = secondaryBonds ? secondaryBonds->getTopology() : nullptr;

	// Merge bonds if present.
	if(primaryBondTopology || secondaryBondTopology) {

		size_t primaryBondCount = primaryBonds ? primaryBonds->elementCount() : 0;
		size_t secondaryBondCount = secondaryBonds ? secondaryBonds->elementCount() : 0;
		size_t totalBondCount = primaryBondCount + secondaryBondCount;

		// Extend all property arrays of primary dataset and copy data from secondary set if it contains a matching property.
		if(secondaryBondCount != 0) {
			BondsObject* primaryMutableBonds = particles->makeBondsMutable();
			primaryMutableBonds->makePropertiesMutable();
			primaryMutableBonds->setElementCount(totalBondCount);
			for(PropertyObject* prop : primaryMutableBonds->properties()) {
				OVITO_ASSERT(prop->size() == totalBondCount);

				// Find corresponding property in second dataset.
				const PropertyObject* secondProp;
				if(prop->type() != BondsObject::UserProperty)
					secondProp = secondaryBonds->getProperty(prop->type());
				else
					secondProp = secondaryBonds->getProperty(prop->name());
				if(secondProp && secondProp->size() == secondaryBondCount && secondProp->componentCount() == prop->componentCount() && secondProp->dataType() == prop->dataType()) {
					OVITO_ASSERT(prop->stride() == secondProp->stride());
					std::memcpy(static_cast<char*>(prop->data()) + prop->stride() * primaryBondCount, secondProp->constData(), prop->stride() * secondaryBondCount);
				}

				// Combine bond types based on their names.
				if(secondProp && secondProp->elementTypes().empty() == false && prop->componentCount() == 1 && prop->dataType() == PropertyStorage::Int) {
					std::map<int,int> typeMap;
					for(const ElementType* type2 : secondProp->elementTypes()) {
						if(!type2->name().isEmpty()) {
							const ElementType* type1 = prop->elementType(type2->name());
							if(type1 == nullptr) {
								OORef<ElementType> type2clone = cloneHelper.cloneObject(type2, false);
								type2clone->setNumericId(prop->generateUniqueElementTypeId());
								prop->addElementType(type2clone);
								typeMap.insert(std::make_pair(type2->numericId(), type2clone->numericId()));
							}
							else if(type1->numericId() != type2->numericId()) {
								typeMap.insert(std::make_pair(type2->numericId(), type1->numericId()));
							}
						}
						else if(!prop->elementType(type2->numericId())) {
							OORef<ElementType> type2clone = cloneHelper.cloneObject(type2, false);
							prop->addElementType(type2clone);
							OVITO_ASSERT(type2clone->numericId() == type2->numericId());
						}
					}
					// Remap bond property values.
					if(typeMap.empty() == false) {
						for(int* p = prop->dataInt() + primaryBondCount; p != prop->dataInt() + totalBondCount; ++p) {
							auto iter = typeMap.find(*p);
							if(iter != typeMap.end()) *p = iter->second;
						}
					}
				}

				// Shift particle indices.
				if(prop->type() == BondsObject::TopologyProperty && primaryParticleCount != 0) {
					for(size_t i = primaryBondCount; i < totalBondCount; i++) {
						prop->setInt64Component(i, 0, prop->getInt64Component(i, 0) + primaryParticleCount);
						prop->setInt64Component(i, 1, prop->getInt64Component(i, 1) + primaryParticleCount);
					}
				}
			}
		}

		// Copy bond properties from second dataset which do not exist in the primary dataset yet.
		if(secondaryBonds) {
			BondsObject* primaryMutableBonds = particles->makeBondsMutable();
			for(const PropertyObject* prop : secondaryBonds->properties()) {
				if(prop->size() != secondaryBondCount) continue;

				// Check if the property already exists in the output.
				if(prop->type() != BondsObject::UserProperty) {
					if(primaryMutableBonds->getProperty(prop->type()))
						continue;
				}
				else {
					if(primaryMutableBonds->getProperty(prop->name()))
						continue;
				}

				// Put the property into the output.
				OORef<PropertyObject> clonedProperty = cloneHelper.cloneObject(prop, false);
				clonedProperty->resize(totalBondCount, true);
				primaryMutableBonds->addProperty(clonedProperty);

				// Shift values of second dataset and reset values of first dataset to zero:
				if(primaryBondCount != 0) {
					std::memmove(static_cast<char*>(clonedProperty->data()) + clonedProperty->stride() * primaryBondCount, clonedProperty->constData(), clonedProperty->stride() * secondaryBondCount);
					std::memset(clonedProperty->data(), 0, clonedProperty->stride() * primaryBondCount);
				}
			}
		}
	}

	int secondaryFrame = secondaryState.data() ? secondaryState.data()->sourceFrame() : 1;
	if(secondaryFrame < 0)
		secondaryFrame = dataset()->animationSettings()->timeToFrame(time);

	QString statusMessage = tr("Merged %1 existing particles with %2 particles from frame %3 of second dataset.")
			.arg(primaryParticleCount)
			.arg(secondaryParticleCount)
			.arg(secondaryFrame);
	return PipelineStatus(secondaryState.status().type(), statusMessage);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
