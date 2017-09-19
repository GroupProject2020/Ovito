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
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/objects/ParticleDisplay.h>
#include <plugins/particles/objects/ParticleType.h>
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/particles/objects/BondType.h>
#include <plugins/grid/data/VoxelProperty.h>
#include <core/dataset/data/simcell/SimulationCellObject.h>
#include <core/dataset/data/simcell/SimulationCellDisplay.h>
#include <core/dataset/data/properties/PropertyStorage.h>
#include "ParticleFrameData.h"
#include "ParticleImporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import)

/******************************************************************************
* Sorts the particle types w.r.t. their name. Reassigns the per-particle type IDs.
* This method is used by file parsers that create particle types on the go while the read the particle data.
* In such a case, the assignment of IDs to types depends on the storage order of particles in the file, which is not desirable.
******************************************************************************/
void ParticleFrameData::ParticleTypeList::sortParticleTypesByName(PropertyStorage* typeProperty)
{
	// Check if type IDs form a consecutive sequence starting at 1.
	for(size_t index = 0; index < _particleTypes.size(); index++) {
		if(_particleTypes[index].id != index + 1)
			return;
	}

	// Check if types are already in the correct order.
	auto compare = [](const ParticleTypeDefinition& a, const ParticleTypeDefinition& b) -> bool { return a.name.compare(b.name) < 0; };
	if(std::is_sorted(_particleTypes.begin(), _particleTypes.end(), compare))
		return;

	// Reorder types.
	std::sort(_particleTypes.begin(), _particleTypes.end(), compare);

	// Build map of IDs.
	std::vector<int> mapping(_particleTypes.size() + 1);
	for(size_t index = 0; index < _particleTypes.size(); index++) {
		mapping[_particleTypes[index].id] = index + 1;
		_particleTypes[index].id = index + 1;
	}

	// Remap particle type IDs.
	if(typeProperty) {
		for(int& t : typeProperty->intRange()) {
			OVITO_ASSERT(t >= 1 && t < mapping.size());
			t = mapping[t];
		}
	}
}

/******************************************************************************
* Sorts particle types with ascending identifier.
******************************************************************************/
void ParticleFrameData::ParticleTypeList::sortParticleTypesById()
{
	auto compare = [](const ParticleTypeDefinition& a, const ParticleTypeDefinition& b) -> bool { return a.id < b.id; };
	std::sort(_particleTypes.begin(), _particleTypes.end(), compare);
}

/******************************************************************************
* Inserts the loaded data into the provided pipeline state structure.
* This function is called by the system from the main thread after the
* asynchronous loading task has finished.
******************************************************************************/
PipelineFlowState ParticleFrameData::handOver(DataSet* dataset, const PipelineFlowState& existing, bool isNewFile)
{
	PipelineFlowState output;

	// Transfer simulation cell.
	OORef<SimulationCellObject> cell = existing.findObject<SimulationCellObject>();
	if(!cell) {
		cell = new SimulationCellObject(dataset, simulationCell());

		// Create the display object for the simulation cell.
		if(SimulationCellDisplay* cellDisplay = dynamic_object_cast<SimulationCellDisplay>(cell->displayObject())) {
			cellDisplay->loadUserDefaults();

			// Choose an appropriate line width depending on the cell's size.
			FloatType cellDiameter = (
					simulationCell().matrix().column(0) +
					simulationCell().matrix().column(1) +
					simulationCell().matrix().column(2)).length();
			cellDisplay->setCellLineWidth(cellDiameter * FloatType(1.4e-3));
		}
	}
	else {
		// Adopt pbc flags from input file only if it is a new file.
		// This gives the user the option to change the pbc flags without them
		// being overwritten when a new frame from a simulation sequence is loaded.
		cell->setData(simulationCell(), isNewFile);
	}
	output.addObject(cell);

	// Transfer particle properties.
	for(auto& property : _particleProperties) {

		// Look for existing property object.
		OORef<ParticleProperty> propertyObj;
		for(DataObject* dataObj : existing.objects()) {
			ParticleProperty* po = dynamic_object_cast<ParticleProperty>(dataObj);
			if(po && po->type() == property->type() && po->name() == property->name()) {
				propertyObj = po;
				break;
			}
		}

		if(propertyObj) {
			propertyObj->setStorage(std::move(property));
		}
		else {
			propertyObj = ParticleProperty::createFromStorage(dataset, std::move(property));
		}

		// Transfer particle types.
		if(ParticleTypeList* typeList = getTypeListOfParticleProperty(propertyObj->storage().get())) {
			insertParticleTypes(propertyObj, typeList, isNewFile);
		}

		output.addObject(propertyObj);
	}

	// Transfer bonds.
	if(bonds()) {
		OORef<BondsObject> bondsObj = existing.findObject<BondsObject>();
		if(!bondsObj) {
			bondsObj = new BondsObject(dataset);
			bondsObj->setStorage(std::move(_bonds));

			// Set up display object for the bonds.
			if(!bondsObj->displayObjects().empty()) {
				if(BondsDisplay* bondsDisplay = dynamic_object_cast<BondsDisplay>(bondsObj->displayObjects().front())) {
					bondsDisplay->loadUserDefaults();
				}
			}
		}
		else {
			bondsObj->setStorage(std::move(_bonds));
		}
		output.addObject(bondsObj);

		// Transfer bond properties.
		for(auto& property : _bondProperties) {

			// Look for existing property object.
			OORef<BondProperty> propertyObj;
			for(DataObject* dataObj : existing.objects()) {
				BondProperty* po = dynamic_object_cast<BondProperty>(dataObj);
				if(po && po->type() == property->type() && po->name() == property->name()) {
					propertyObj = po;
					break;
				}
			}

			if(propertyObj) {
				propertyObj->setStorage(std::move(property));
			}
			else {
				propertyObj = BondProperty::createFromStorage(dataset, std::move(property));
			}

			// Transfer bond types.
			if(BondTypeList* typeList = getTypeListOfBondProperty(propertyObj->storage().get())) {
				insertBondTypes(propertyObj, typeList);
			}

			output.addObject(propertyObj);
		}
	}

	// Transfer field quantities.
	for(auto& fq : _fieldQuantities) {

		// Look for existing field quantity object.
		OORef<VoxelProperty> fqObj;
		for(DataObject* dataObj : existing.objects()) {
			VoxelProperty* po = dynamic_object_cast<VoxelProperty>(dataObj);
			if(po && po->name() == fq->name()) {
				fqObj = po;
				break;
			}
		}

		if(fqObj) {
			fqObj->setStorage(std::move(fq));
		}
		else {
			fqObj = static_object_cast<VoxelProperty>(VoxelProperty::OOClass().createFromStorage(dataset, std::move(fq)));
		}

		output.addObject(fqObj);
	}

	// Pass timestep information and other metadata to modification pipeline.
	output.attributes() = std::move(_attributes);
	return output;
}

/******************************************************************************
* Inserts the stores particle types into the given destination object.
******************************************************************************/
void ParticleFrameData::insertParticleTypes(ParticleProperty* typeProperty, ParticleTypeList* typeList, bool isNewFile)
{
	QSet<ElementType*> activeTypes;
	if(typeList) {
		for(const auto& item : typeList->particleTypes()) {
			QString name = item.name;
			if(name.isEmpty())
				name = ParticleImporter::tr("Type %1").arg(item.id);
			OORef<ParticleType> ptype = static_object_cast<ParticleType>(typeProperty->elementType(name));
			if(ptype) {
				ptype->setId(item.id);
			}
			else {
				ptype = static_object_cast<ParticleType>(typeProperty->elementType(item.id));
				if(ptype) {
					if(item.name.isEmpty() == false)
						ptype->setName(item.name);
				}
				else {
					ptype = new ParticleType(typeProperty->dataset());
					ptype->setId(item.id);
					ptype->setName(name);

					// Assign initial standard color to new particle types.
					if(item.color != Color(0,0,0))
						ptype->setColor(item.color);
					else
						ptype->setColor(ParticleType::getDefaultParticleColor(typeProperty->type(), name, ptype->id()));

					if(item.radius == 0)
						ptype->setRadius(ParticleType::getDefaultParticleRadius(typeProperty->type(), name, ptype->id()));

					typeProperty->addElementType(ptype);
				}
			}
			activeTypes.insert(ptype);

			if(item.color != Color(0,0,0))
				ptype->setColor(item.color);

			if(item.radius != 0)
				ptype->setRadius(item.radius);
		}
	}

	if(isNewFile) {
		// Remove unused particle types.
		for(int index = typeProperty->elementTypes().size() - 1; index >= 0; index--) {
			if(!activeTypes.contains(typeProperty->elementTypes()[index]))
				typeProperty->removeElementType(index);
		}
	}
}

/******************************************************************************
* Inserts the stores bond types into the given destination object.
******************************************************************************/
void ParticleFrameData::insertBondTypes(BondProperty* typeProperty, BondTypeList* typeList)
{
	QSet<ElementType*> activeTypes;
	if(typeList) {
		for(const auto& item : typeList->bondTypes()) {
			OORef<BondType> type = static_object_cast<BondType>(typeProperty->elementType(item.id));
			QString name = item.name;
			if(name.isEmpty())
				name = ParticleImporter::tr("Type %1").arg(item.id);

			if(type == nullptr) {
				type = new BondType(typeProperty->dataset());
				type->setId(item.id);

				// Assign initial standard color to new bond type.
				if(item.color != Color(0,0,0))
					type->setColor(item.color);
				else
					type->setColor(BondType::getDefaultBondColor(typeProperty->type(), name, type->id()));

				if(item.radius == 0)
					type->setRadius(BondType::getDefaultBondRadius(typeProperty->type(), name, type->id()));

				typeProperty->addElementType(type);
			}
			activeTypes.insert(type);

			if(type->name().isEmpty())
				type->setName(name);

			if(item.color != Color(0,0,0))
				type->setColor(item.color);

			if(item.radius != 0)
				type->setRadius(item.radius);
		}
	}

	// Remove unused bond types.
	for(int index = typeProperty->elementTypes().size() - 1; index >= 0; index--) {
		if(!activeTypes.contains(typeProperty->elementTypes()[index]))
			typeProperty->removeElementType(index);
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
