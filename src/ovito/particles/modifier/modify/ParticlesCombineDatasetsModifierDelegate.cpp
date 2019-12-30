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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/BondsVis.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
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
				prop->copyRangeFrom(secondProp, 0, primaryParticleCount, secondaryParticleCount);
			}
			else if(prop->type() != ParticlesObject::UserProperty) {
				ConstDataObjectPath containerPath = { secondaryParticles };
				PropertyPtr temporaryProp = ParticlesObject::OOClass().createStandardStorage(secondaryParticles->elementCount(), prop->type(), true, containerPath);
				prop->modifiableStorage()->copyRangeFrom(*temporaryProp, 0, primaryParticleCount, secondaryParticleCount);
			}

			// Combine particle types lists.
			mergeElementTypes(prop, secondProp, cloneHelper);

			// Assign unique particle and molecule IDs.
			if(prop->type() == ParticlesObject::IdentifierProperty && primaryParticleCount != 0) {
				PropertyAccess<qlonglong> identifiers(prop);
				qlonglong maxId = *std::max_element(identifiers.cbegin(), identifiers.cbegin() + primaryParticleCount);
				std::iota(identifiers.begin() + primaryParticleCount, identifiers.end(), maxId + 1);
			}
			else if(prop->type() == ParticlesObject::MoleculeProperty && primaryParticleCount != 0) {
				PropertyAccess<qlonglong> identifiers(prop);
				qlonglong maxId = *std::max_element(identifiers.cbegin(), identifiers.cbegin() + primaryParticleCount);
				for(qlonglong* mol_id = identifiers.begin() + primaryParticleCount; mol_id != identifiers.end(); ++mol_id)
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
			std::memmove(clonedProperty->modifiableStorage()->buffer() + primaryParticleCount * clonedProperty->stride(), clonedProperty->storage()->cbuffer(), clonedProperty->stride() * secondaryParticleCount);
			std::memset(clonedProperty->modifiableStorage()->buffer(), 0, clonedProperty->stride() * primaryParticleCount);
		}
	}

	// Merge bonds.
	const BondsObject* primaryBonds = particles->bonds();
	const BondsObject* secondaryBonds = secondaryParticles->bonds();

	// Merge bonds if present.
	if(primaryBonds || secondaryBonds) {

		// Create the primary bonds object if it doesn't exist yet.
		if(!primaryBonds) {
			primaryBonds = new BondsObject(dataset());
			particles->setBonds(primaryBonds);
			OVITO_ASSERT(secondaryBonds);
			particles->bonds()->setVisElement(secondaryBonds->visElement());
		}

		size_t primaryBondCount = primaryBonds->elementCount();
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
					prop->copyRangeFrom(secondProp, 0, primaryBondCount, secondaryBondCount);
				}
				else if(prop->type() != BondsObject::UserProperty) {
					ConstDataObjectPath containerPath = { secondaryParticles, secondaryBonds };
					PropertyPtr temporaryProp = BondsObject::OOClass().createStandardStorage(secondaryBondCount, prop->type(), true, containerPath);
					prop->modifiableStorage()->copyRangeFrom(*temporaryProp, 0, primaryBondCount, secondaryBondCount);
				}

				// Combine bond type lists.
				mergeElementTypes(prop, secondProp, cloneHelper);
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
					std::memmove(clonedProperty->modifiableStorage()->buffer() + primaryBondCount * clonedProperty->stride(), clonedProperty->storage()->cbuffer(), clonedProperty->stride() * secondaryBondCount);
					std::memset(clonedProperty->modifiableStorage()->buffer(), 0, clonedProperty->stride() * primaryBondCount);
				}
			}

			// Shift particle indices stored in the topology array of the second bonds object.
			const PropertyObject* topologyProperty = primaryMutableBonds->getProperty(BondsObject::TopologyProperty);
			if(topologyProperty && primaryParticleCount != 0) {
				PropertyAccess<ParticleIndexPair> mutableTopologyProperty = primaryMutableBonds->makeMutable(topologyProperty);
				for(auto bond = mutableTopologyProperty.begin() + primaryBondCount; bond != mutableTopologyProperty.end(); ++bond) {
					(*bond)[0] += primaryParticleCount;
					(*bond)[1] += primaryParticleCount;
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
