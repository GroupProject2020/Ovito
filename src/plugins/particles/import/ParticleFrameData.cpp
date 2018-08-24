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
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/particles/objects/BondsVis.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/objects/ParticlesVis.h>
#include <plugins/particles/objects/ParticleType.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/particles/objects/BondType.h>
#include <plugins/grid/objects/VoxelProperty.h>
#include <plugins/grid/objects/VoxelGrid.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/stdobj/simcell/SimulationCellVis.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <core/app/Application.h>
#include <core/dataset/io/FileSource.h>
#include <core/dataset/pipeline/PipelineOutputHelper.h>
#include "ParticleFrameData.h"
#include "ParticleImporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import)

/******************************************************************************
* Sorts the types w.r.t. their name. Reassigns the per-element type IDs too.
* This method is used by file parsers that create particle/bond types on the 
* go while the read the data. In such a case, the assignment of IDs to types 
* depends on the storage order of particles/bonds in the file, which is not desirable.
******************************************************************************/
void ParticleFrameData::TypeList::sortTypesByName(const PropertyPtr& typeProperty)
{
	// Check if type IDs form a consecutive sequence starting at 1.
	// If not, we leave the type order as it is.
	for(size_t index = 0; index < _types.size(); index++) {
		if(_types[index].id != index + 1)
			return;
	}

	// Check if types are already in the correct order.
	auto compare = [](const TypeDefinition& a, const TypeDefinition& b) -> bool { return a.name.compare(b.name) < 0; };
	if(std::is_sorted(_types.begin(), _types.end(), compare))
		return;

	// Reorder types by name.
	std::sort(_types.begin(), _types.end(), compare);

	// Build map of IDs.
	std::vector<int> mapping(_types.size() + 1);
	for(size_t index = 0; index < _types.size(); index++) {
		mapping[_types[index].id] = index + 1;
		_types[index].id = index + 1;
	}

	// Remap particle/bond type IDs.
	if(typeProperty) {
		for(int& t : typeProperty->intRange()) {
			OVITO_ASSERT(t >= 1 && t < mapping.size());
			t = mapping[t];
		}
	}
}

/******************************************************************************
* Sorts particle/bond types according numeric identifier.
******************************************************************************/
void ParticleFrameData::TypeList::sortTypesById()
{
	auto compare = [](const TypeDefinition& a, const TypeDefinition& b) -> bool { return a.id < b.id; };
	std::sort(_types.begin(), _types.end(), compare);
}

/******************************************************************************
* Determines the PBC shift vectors for bonds using the minimum image convention.
******************************************************************************/
void ParticleFrameData::generateBondPeriodicImageProperty()
{
	PropertyPtr posProperty = findStandardParticleProperty(ParticleProperty::PositionProperty);
	if(!posProperty) return;
	
	PropertyPtr bondTopologyProperty = findStandardBondProperty(BondProperty::TopologyProperty);
	if(!bondTopologyProperty) return;
		
	PropertyPtr bondPeriodicImageProperty = BondProperty::createStandardStorage(bondTopologyProperty->size(), BondProperty::PeriodicImageProperty, true);
	addBondProperty(bondPeriodicImageProperty);
	
	if(!simulationCell().pbcFlags()[0] && !simulationCell().pbcFlags()[1] && !simulationCell().pbcFlags()[2])
		return;

	for(size_t bondIndex = 0; bondIndex < bondTopologyProperty->size(); bondIndex++) {
		size_t index1 = bondTopologyProperty->getInt64Component(bondIndex, 0);
		size_t index2 = bondTopologyProperty->getInt64Component(bondIndex, 1);
		OVITO_ASSERT(index1 < posProperty->size() && index2 < posProperty->size());
		Vector3 delta = simulationCell().absoluteToReduced(posProperty->getPoint3(index2) - posProperty->getPoint3(index1));		
		for(size_t dim = 0; dim < 3; dim++) {
			if(simulationCell().pbcFlags()[dim])
				bondPeriodicImageProperty->setIntComponent(bondIndex, dim, -(int)floor(delta[dim] + FloatType(0.5)));
		}
	}
}

/******************************************************************************
* Inserts the loaded data into the provided pipeline state structure.
* This function is called by the system from the main thread after the
* asynchronous loading task has finished.
******************************************************************************/
void ParticleFrameData::handOver(PipelineOutputHelper& poh, const PipelineFlowState& existing, bool isNewFile, FileSource* fileSource)
{
	// Hand over simulation cell.
	OORef<SimulationCellObject> cell = existing.findObjectOfType<SimulationCellObject>();
	if(!cell) {
		cell = new SimulationCellObject(poh.dataset(), simulationCell());

		// Create the vis element for the simulation cell.
		if(SimulationCellVis* cellVis = dynamic_object_cast<SimulationCellVis>(cell->visElement())) {
			if(Application::instance()->guiMode())
				cellVis->loadUserDefaults();

			// Choose an appropriate line width depending on the cell's size.
			FloatType cellDiameter = (
					simulationCell().matrix().column(0) +
					simulationCell().matrix().column(1) +
					simulationCell().matrix().column(2)).length();
			cellVis->setCellLineWidth(std::max(cellDiameter * FloatType(1.4e-3), FloatType(1e-8)));
		}
	}
	else {
		// Adopt pbc flags from input file only if it is a new file.
		// This gives the user the option to change the pbc flags without them
		// being overwritten when a new frame from a simulation sequence is loaded.
		cell->setData(simulationCell(), isNewFile);
	}
	poh.outputObject(cell);

	if(!_particleProperties.empty()) {
	
		// Hand over particles.
		ParticlesObject* existingParticles = existing.findObjectOfType<ParticlesObject>();
		OORef<ParticlesObject> particles = new ParticlesObject(poh.dataset());
		poh.outputObject(particles);

		// Transfer particle properties.
		for(auto& property : _particleProperties) {

			// Look for existing property object.
			OORef<PropertyObject> propertyObj;
			if(existingParticles) {
				for(PropertyObject* po : existingParticles->properties()) {
					if(po->type() == property->type() && po->name() == property->name()) {
						propertyObj = po;
						break;
					}
				}
			}

			if(propertyObj) {
				propertyObj->setStorage(std::move(property));
			}
			else {
				propertyObj = ParticleProperty::createFromStorage(poh.dataset(), std::move(property));
			}
			particles->addProperty(propertyObj);

			// Auto-adjust particle display radius.
			if(isNewFile && propertyObj->type() == ParticleProperty::PositionProperty) {
				if(ParticlesVis* particleVis = dynamic_object_cast<ParticlesVis>(propertyObj->visElement())) {
					FloatType cellDiameter = (
							simulationCell().matrix().column(0) +
							simulationCell().matrix().column(1) +
							simulationCell().matrix().column(2)).length();
					// Limit particle radius to a fraction of the cell diameter. 
					// This is to avoid extremely large particles when the length scale of the simulation is <<1.
					cellDiameter /= 2;
					if(particleVis->defaultParticleRadius() > cellDiameter && cellDiameter != 0)
						particleVis->setDefaultParticleRadius(cellDiameter);
				}
			}

			// Transfer particle types.
			auto typeList = _typeLists.find(propertyObj->storage().get());
			insertTypes(propertyObj, (typeList != _typeLists.end()) ? typeList->second.get() : nullptr, isNewFile, false);
		}

		// Hand over bonds.
		if(!_bondProperties.empty()) {

			BondsObject* existingBonds = (existingParticles != nullptr) ? existingParticles->bonds() : nullptr;
			OORef<BondsObject> bonds = new BondsObject(poh.dataset());
			particles->setBonds(bonds);

			// Transfer bonds.
			for(auto& property : _bondProperties) {

				// Look for existing property object.
				OORef<PropertyObject> propertyObj;
				if(existingBonds) {
					for(PropertyObject* po : existingBonds->properties()) {
						if(po->type() == property->type() && po->name() == property->name()) {
							propertyObj = po;
							break;
						}
					}
				}

				if(propertyObj) {
					propertyObj->setStorage(std::move(property));
				}
				else {
					propertyObj = BondProperty::createFromStorage(poh.dataset(), std::move(property));
				}
				bonds->addProperty(propertyObj);

				// Transfer bond types.
				auto typeList = _typeLists.find(propertyObj->storage().get());
				insertTypes(propertyObj, (typeList != _typeLists.end()) ? typeList->second.get() : nullptr, isNewFile, true);
			}
		}
	}

	// Transfer voxel data.
	if(voxelGridShape().empty() == false) {

		VoxelGrid* existingVoxelGrid = existing.findObjectOfType<VoxelGrid>();
		OORef<VoxelGrid> voxelGrid = new VoxelGrid(poh.dataset());
		voxelGrid->setShape(voxelGridShape());
		voxelGrid->setDomain(cell);
		poh.outputObject(voxelGrid);
		
		for(auto& property : voxelProperties()) {

			// Look for existing field quantity object.
			OORef<PropertyObject> propertyObject;
			if(existingVoxelGrid) {
				for(PropertyObject* po : existingVoxelGrid->properties()) {
					if(po->name() == property->name()) {
						propertyObject = po;
						break;
					}
				}
			}

			if(propertyObject) {
				propertyObject->setStorage(std::move(property));
			}
			else {
				propertyObject = static_object_cast<VoxelProperty>(VoxelProperty::OOClass().createFromStorage(poh.dataset(), std::move(property)));
			}
			voxelGrid->addProperty(propertyObject);
		}
	}

	// Hand over timestep information and other metadata as global attributes.
	for(auto a = _attributes.cbegin(); a != _attributes.cend(); ++a) {
		poh.outputAttribute(a.key(), a.value());
	}

	// If the file parser has detected that the input file contains additional frame data following the
	// current frame, active the 'contains multiple frames' option for the importer. This will trigger
	// a scanning process for the entire file to discover all contained frames.
	if(_detectedAdditionalFrames && isNewFile) {
		if(ParticleImporter* importer = dynamic_object_cast<ParticleImporter>(fileSource->importer())) {
			importer->setMultiTimestepFile(true);
		}
	}
}

/******************************************************************************
* Inserts the particle or bond types into the given destination property object.
******************************************************************************/
void ParticleFrameData::insertTypes(PropertyObject* typeProperty, TypeList* typeList, bool isNewFile, bool isBondProperty)
{
	QSet<ElementType*> activeTypes;
	if(typeList) {
		for(const auto& item : typeList->types()) {
			OORef<ElementType> ptype;
			if(item.name.isEmpty()) 
				ptype = typeProperty->elementType(item.id);
			else {
				ptype = typeProperty->elementType(item.name);
				if(ptype) {
					ptype->setId(item.id);
				}
				else {
					ptype = typeProperty->elementType(item.id);
					if(ptype && !item.name.isEmpty())
						ptype->setName(item.name);
				}
			}
			if(!ptype) {
				if(!isBondProperty) {
					ptype = new ParticleType(typeProperty->dataset());
					ptype->setId(item.id);
					ptype->setName(item.name);
					if(item.radius == 0)
						static_object_cast<ParticleType>(ptype)->setRadius(ParticleType::getDefaultParticleRadius((ParticleProperty::Type)typeProperty->type(), ptype->nameOrId(), ptype->id()));
				}
				else {
					ptype = new BondType(typeProperty->dataset());
					ptype->setId(item.id);
					ptype->setName(item.name);
					if(item.radius == 0)
						static_object_cast<BondType>(ptype)->setRadius(BondType::getDefaultBondRadius((BondProperty::Type)typeProperty->type(), ptype->nameOrId(), ptype->id()));
				}

				if(item.color != Color(0,0,0))
					ptype->setColor(item.color);
				else if(!isBondProperty)
					ptype->setColor(ParticleType::getDefaultParticleColor((ParticleProperty::Type)typeProperty->type(), ptype->nameOrId(), ptype->id()));
				else
					ptype->setColor(BondType::getDefaultBondColor((BondProperty::Type)typeProperty->type(), ptype->nameOrId(), ptype->id()));

				typeProperty->addElementType(ptype);
			}
			activeTypes.insert(ptype);

			if(item.color != Color(0,0,0))
				ptype->setColor(item.color);

			if(item.radius != 0) {
				if(!isBondProperty)
					static_object_cast<ParticleType>(ptype)->setRadius(item.radius);
				else
					static_object_cast<BondType>(ptype)->setRadius(item.radius);
			}
		}
	}

	if(isNewFile) {
		// Remove unused types.
		for(int index = typeProperty->elementTypes().size() - 1; index >= 0; index--) {
			if(!activeTypes.contains(typeProperty->elementTypes()[index]))
				typeProperty->removeElementType(index);
		}
	}
}

/******************************************************************************
* Sorts the particles list with respect to particle IDs.
* Does nothing if particles do not have IDs.
******************************************************************************/
void ParticleFrameData::sortParticlesById()
{
	PropertyPtr ids = findStandardParticleProperty(ParticleProperty::IdentifierProperty);
	if(!ids) return;

	// Determine new permutation of particles where they are sorted by ascending ID.
	std::vector<size_t> permutation(ids->size());
	std::iota(permutation.begin(), permutation.end(), (size_t)0);
	std::sort(permutation.begin(), permutation.end(), [id = ids->constDataInt64()](size_t a, size_t b) { return id[a] < id[b]; });
	std::vector<size_t> invertedPermutation(ids->size());
	bool isAlreadySorted = true;
	for(size_t i = 0; i < permutation.size(); i++) {
		invertedPermutation[permutation[i]] = i;
		if(permutation[i] != i) isAlreadySorted = false;
	}
	if(isAlreadySorted) return;

	// Reorder all values in the particle property arrays.
	for(const PropertyPtr& prop : particleProperties()) {
		PropertyStorage copy(*prop);
		prop->mappedCopy(copy, invertedPermutation);
	}

	// Update bond topology data to match new particle ordering.
	if(PropertyPtr bondTopology = findStandardBondProperty(BondProperty::TopologyProperty)) {
		auto particleIndex = bondTopology->dataInt64();
		auto particleIndexEnd = particleIndex + bondTopology->size() * 2;
		for(; particleIndex != particleIndexEnd; ++particleIndex) {
			if(*particleIndex >= 0 && *particleIndex < invertedPermutation.size())
				*particleIndex = invertedPermutation[*particleIndex];
		}
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
