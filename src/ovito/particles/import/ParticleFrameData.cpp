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
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/BondsVis.h>
#include <ovito/particles/objects/BondType.h>
#include <ovito/particles/objects/ParticlesVis.h>
#include <ovito/particles/objects/ParticleType.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/grid/objects/VoxelGrid.h>
#include <ovito/grid/objects/VoxelGridVis.h>
#include <ovito/mesh/tri/TriMeshObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/simcell/SimulationCellVis.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/io/FileSource.h>
#include "ParticleFrameData.h"
#include "ParticleImporter.h"

namespace Ovito { namespace Particles {

/******************************************************************************
* Sorts the types w.r.t. their name. Reassigns the per-element type IDs too.
* This method is used by file parsers that create particle/bond types on the
* go while the read the data. In such a case, the assignment of IDs to types
* depends on the storage order of particles/bonds in the file, which is not desirable.
******************************************************************************/
void ParticleFrameData::TypeList::sortTypesByName(PropertyAccess<int>& typeProperty)
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
		for(int& t : typeProperty) {
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
	ConstPropertyAccess<Point3> posProperty = findStandardParticleProperty(ParticlesObject::PositionProperty);
	if(!posProperty) return;

	ConstPropertyAccess<ParticleIndexPair> bondTopologyProperty = findStandardBondProperty(BondsObject::TopologyProperty);
	if(!bondTopologyProperty) return;

	OVITO_ASSERT(!findStandardBondProperty(BondsObject::PeriodicImageProperty));
	PropertyAccess<Vector3I> bondPeriodicImageProperty = addBondProperty(BondsObject::OOClass().createStandardStorage(bondTopologyProperty.size(), BondsObject::PeriodicImageProperty, true));

	if(!simulationCell().pbcFlags()[0] && !simulationCell().pbcFlags()[1] && !simulationCell().pbcFlags()[2])
		return;

	for(size_t bondIndex = 0; bondIndex < bondTopologyProperty.size(); bondIndex++) {
		size_t index1 = bondTopologyProperty[bondIndex][0];
		size_t index2 = bondTopologyProperty[bondIndex][1];
		OVITO_ASSERT(index1 < posProperty.size() && index2 < posProperty.size());
		Vector3 delta = simulationCell().absoluteToReduced(posProperty[index2] - posProperty[index1]);
		for(size_t dim = 0; dim < 3; dim++) {
			if(simulationCell().pbcFlags()[dim])
				bondPeriodicImageProperty[bondIndex][dim] = -(int)std::floor(delta[dim] + FloatType(0.5));
		}
	}
}

/******************************************************************************
* Inserts the loaded data into the provided pipeline state structure.
* This function is called by the system from the main thread after the
* asynchronous loading task has finished.
******************************************************************************/
OORef<DataCollection> ParticleFrameData::handOver(const DataCollection* existing, bool isNewFile, FileSource* fileSource)
{
	OORef<DataCollection> output = new DataCollection(fileSource->dataset());

	// Hand over simulation cell.
	SimulationCellObject* cell = const_cast<SimulationCellObject*>(existing ? existing->getObject<SimulationCellObject>() : nullptr);
	if(!cell) {
		// Create a new SimulationCellObject.
		cell = output->createObject<SimulationCellObject>(fileSource, simulationCell());

		// Initialize the simulation cell and its vis element with default values.
		if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
			cell->loadUserDefaults();

		// Set up the vis element for the simulation cell.
		if(SimulationCellVis* cellVis = dynamic_object_cast<SimulationCellVis>(cell->visElement())) {

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
		output->addObject(cell);
	}

	if(!_particleProperties.empty()) {

		// Hand over particles.
		const ParticlesObject* existingParticles = existing ? existing->getObject<ParticlesObject>() : nullptr;
		ParticlesObject* particles = output->createObject<ParticlesObject>(fileSource);
		if(!existingParticles) {
			
			// Create the custom vis element requested by the file importer.
			if(_particleVisElementClass && (!particles->visElement() || _particleVisElementClass != &particles->visElement()->getOOMetaClass()))
				particles->setVisElement(static_object_cast<DataVis>(_particleVisElementClass->createInstance(particles->dataset())));
			else if(!_particleVisElementClass && particles->visElement())
				particles->removeVisElement(0);

			// Initialize the particles object and its vis element to default values.
			if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
				particles->loadUserDefaults();
		}
		else {
			// Adopt the existing particles vis element, or create the right vis element requested by the file importer.
			if(_particleVisElementClass && (!existingParticles->visElement() || _particleVisElementClass != &existingParticles->visElement()->getOOMetaClass())) {
				particles->setVisElement(static_object_cast<DataVis>(_particleVisElementClass->createInstance(particles->dataset())));
				if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
					particles->visElement()->loadUserDefaults();
			}
			else if(!_particleVisElementClass && particles->visElement())
				particles->removeVisElement(0);
			else
				particles->setVisElement(existingParticles->visElement());
		}

		// Auto-adjust particle display radius.
		if(isNewFile) {
			if(ParticlesVis* particleVis = particles->visElement<ParticlesVis>()) {
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

		// Transfer particle properties.
		for(auto& property : _particleProperties) {

			// Look for existing property object.
			OORef<PropertyObject> propertyObj;
			if(existingParticles) {
				propertyObj = (property->type() != 0) ? existingParticles->getProperty(property->type()) : existingParticles->getProperty(property->name());
			}

			if(propertyObj) {
				propertyObj->setStorage(std::move(property));
				particles->addProperty(propertyObj);
			}
			else {
				propertyObj = particles->createProperty(std::move(property));
			}
			// For backward compatibility with OVITO 2.9.0, attach the particles vis element
			// to the 'Position' particle property object as well.
			if(propertyObj->type() == ParticlesObject::PositionProperty)
				propertyObj->setVisElement(particles->visElement<ParticlesVis>());

			// Transfer particle types.
			auto typeList = _typeLists.find(propertyObj->storage().get());
			insertTypes(propertyObj, (typeList != _typeLists.end()) ? typeList->second.get() : nullptr, isNewFile, false);
		}

		// Hand over the bonds.
		if(!_bondProperties.empty()) {

			OORef<BondsObject> existingBonds = existingParticles ? existingParticles->bonds() : nullptr;
			BondsObject* bonds = new BondsObject(fileSource->dataset());
			particles->setBonds(bonds);
			bonds->setDataSource(fileSource);
			if(!existingBonds) {
				// Initialize the bonds object and its vis element to default values.
				if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
					bonds->loadUserDefaults();
			}
			else {
				// Adopt the existing vis element.
				bonds->setVisElements(existingBonds->visElements());
			}

			// Transfer bonds.
			for(auto& property : _bondProperties) {

				// Look for existing property object.
				OORef<PropertyObject> propertyObj;
				if(existingBonds) {
					propertyObj = (property->type() != 0) ? existingBonds->getProperty(property->type()) : existingBonds->getProperty(property->name());
				}

				if(propertyObj) {
					propertyObj->setStorage(std::move(property));
					bonds->addProperty(propertyObj);
				}
				else {
					propertyObj = bonds->createProperty(std::move(property));

					// For backward compatibility with OVITO 2.9.0, attach the bonds vis element
					// also to the 'Topology' bond property object.
					if(propertyObj->type() == BondsObject::TopologyProperty)
						propertyObj->setVisElement(bonds->visElement<BondsVis>());
				}

				// Transfer bond types.
				auto typeList = _typeLists.find(propertyObj->storage().get());
				insertTypes(propertyObj, (typeList != _typeLists.end()) ? typeList->second.get() : nullptr, isNewFile, true);
			}
		}

		particles->verifyIntegrity();
	}

	// Transfer voxel data.
	if(voxelGridShape() != VoxelGrid::GridDimensions{0,0,0}) {

		// Look for an existing VoxelGrid object in the old data collection.
		const VoxelGrid* existingVoxelGrid = nullptr;
		if(existing) {
			if(!voxelGridId().isEmpty()) {
				ConstDataObjectPath path = existing->getObject<VoxelGrid>(voxelGridId());
				if(!path.empty()) existingVoxelGrid = static_object_cast<VoxelGrid>(path.back());
			}
			else existingVoxelGrid = existing->getObject<VoxelGrid>();
		}

		// Create the new VoxelGrid object.
		VoxelGrid* voxelGrid = output->createObject<VoxelGrid>(voxelGridId().isEmpty() ? QStringLiteral("imported") : voxelGridId(), fileSource, voxelGridTitle());
		voxelGrid->setShape(voxelGridShape());
		voxelGrid->setDomain(cell);

		// Create a visualization element for the voxel grid.
		if(!existingVoxelGrid) {
			voxelGrid->setVisElement(new VoxelGridVis(voxelGrid->dataset()));
			// Initialize the vis element to default values.
			if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
				voxelGrid->visElement()->loadUserDefaults();
			voxelGrid->visElement()->setEnabled(false);
		}
		else {
			// Adopt the existing vis element.
			voxelGrid->setVisElements(existingVoxelGrid->visElements());
		}

		// Give the vis element an expressive title.
		if(VoxelGridVis* gridVis = voxelGrid->visElement<VoxelGridVis>())
			gridVis->setTitle(voxelGridTitle());

		for(auto& property : voxelProperties()) {

			// Look for existing field quantity object.
			OORef<PropertyObject> propertyObj;
			if(existingVoxelGrid) {
				propertyObj = (property->type() != 0) ? existingVoxelGrid->getProperty(property->type()) : existingVoxelGrid->getProperty(property->name());
			}

			if(propertyObj) {
				propertyObj->setStorage(std::move(property));
				voxelGrid->addProperty(propertyObj);
			}
			else {
				propertyObj = voxelGrid->createProperty(std::move(property));
			}
		}
	}

	// Hand over timestep information and other metadata as global attributes.
	for(auto a = _attributes.cbegin(); a != _attributes.cend(); ++a) {
		output->addAttribute(a.key(), a.value(), fileSource);
	}

	// If the file parser has detected that the input file contains additional frame data following the
	// current frame, active the 'contains multiple frames' option for the importer. This will trigger
	// a scanning process for the entire file to discover all contained frames.
	if(_detectedAdditionalFrames && isNewFile) {
		if(ParticleImporter* importer = dynamic_object_cast<ParticleImporter>(fileSource->importer())) {
			importer->setMultiTimestepFile(true);
		}
	}

	return output;
}

/******************************************************************************
* Inserts the particle or bond types into the given destination property object.
******************************************************************************/
void ParticleFrameData::insertTypes(PropertyObject* typeProperty, TypeList* typeList, bool isNewFile, bool isBondProperty)
{
	QSet<ElementType*> activeTypes;
	std::vector<std::pair<int,int>> typeRemapping;

	if(typeList) {
		for(auto& item : typeList->types()) {
			OORef<ElementType> ptype;
			if(item.name.isEmpty()) {
				ptype = typeProperty->elementType(item.id);
			}
			else {
				ptype = typeProperty->elementType(item.name);
				if(ptype) {
					if(item.id != ptype->numericId()) {
						typeRemapping.push_back({item.id, ptype->numericId()});
					}
				}
				else {
					ptype = typeProperty->elementType(item.id);
					if(ptype && ptype->name() != item.name) {
						ptype = nullptr;
						if(!isNewFile) {
							int mappedId = typeProperty->generateUniqueElementTypeId(item.id + typeList->types().size());
							typeRemapping.push_back({item.id, mappedId});
							item.id = mappedId;
						}
					}
				}
			}
			if(!ptype) {
				if(!isBondProperty) {
					ptype = static_object_cast<ElementType>(typeList->elementClass().createInstance(typeProperty->dataset()));
					if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
						ptype->loadUserDefaults();
					ptype->setNumericId(item.id);
					ptype->setName(item.name);
					if(item.radius == 0 && ParticleType::OOClass().isMember(ptype)) {
						static_object_cast<ParticleType>(ptype)->setRadius(ParticleType::getDefaultParticleRadius((ParticlesObject::Type)typeProperty->type(), ptype->nameOrNumericId(), ptype->numericId()));
					}
				}
				else {
					OVITO_ASSERT(typeList->elementClass().isDerivedFrom(BondType::OOClass()));
					ptype = new BondType(typeProperty->dataset());
					if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
						ptype->loadUserDefaults();
					ptype->setNumericId(item.id);
					ptype->setName(item.name);
					if(item.radius == 0)
						static_object_cast<BondType>(ptype)->setRadius(BondType::getDefaultBondRadius((BondsObject::Type)typeProperty->type(), ptype->nameOrNumericId(), ptype->numericId()));
				}

				if(item.color != Color(0,0,0))
					ptype->setColor(item.color);
				else if(!isBondProperty) {
					if(ParticleType::OOClass().isMember(ptype))
						ptype->setColor(ParticleType::getDefaultParticleColor((ParticlesObject::Type)typeProperty->type(), ptype->nameOrNumericId(), ptype->numericId()));
					else
						ptype->setColor(ElementType::getDefaultColor(PropertyStorage::GenericTypeProperty, ptype->nameOrNumericId(), ptype->numericId()));
				}
				else {
					ptype->setColor(BondType::getDefaultBondColor((BondsObject::Type)typeProperty->type(), ptype->nameOrNumericId(), ptype->numericId()));
				}

				typeProperty->addElementType(ptype);
			}
			activeTypes.insert(ptype);

			if(item.color != Color(0,0,0))
				ptype->setColor(item.color);

			if(item.radius != 0) {
				if(ParticleType* particleType = dynamic_object_cast<ParticleType>(ptype))
					particleType->setRadius(item.radius);
				else if(BondType* bondType = dynamic_object_cast<BondType>(ptype))
					bondType->setRadius(item.radius);
			}
			if(item.mass != 0) {
				if(ParticleType* particleType = dynamic_object_cast<ParticleType>(ptype))
					particleType->setMass(item.mass);
			}
			if(ParticleType* particleType = dynamic_object_cast<ParticleType>(ptype)) {
				if(item.shapeMesh) {
					TriMeshObject* shapeObject = particleType->shapeMesh();
					if(!shapeObject) {
						shapeObject = new TriMeshObject(typeProperty->dataset());
						particleType->setShapeMesh(shapeObject);
					}
					shapeObject->setMesh(item.shapeMesh);
				}
				else {
					// Note: Do not automatically reset shape, because we don't want to loose
					// a shape manually assigned by the user to this particle type.
					//static_object_cast<ParticleType>(ptype)->setShapeMesh(nullptr);
				}
			}
		}
	}

	if(isNewFile) {
		// Remove existing types that are no longer needed.
		for(int index = typeProperty->elementTypes().size() - 1; index >= 0; index--) {
			if(!activeTypes.contains(typeProperty->elementTypes()[index]))
				typeProperty->removeElementType(index);
		}
	}

	// Remap type IDs.
	if(!typeRemapping.empty()) {
		for(int& t : PropertyAccess<int>(typeProperty)) {
			for(const auto& mapping : typeRemapping) {
				if(t == mapping.first) {
					t = mapping.second;
					break;
				}
			}
		}
	}
}

/******************************************************************************
* Sorts the particles list with respect to particle IDs.
* Does nothing if particles do not have IDs.
******************************************************************************/
void ParticleFrameData::sortParticlesById()
{
	ConstPropertyAccess<qlonglong> ids = findStandardParticleProperty(ParticlesObject::IdentifierProperty);
	if(!ids) return;

	// Determine new permutation of particles where they are sorted by ascending ID.
	std::vector<size_t> permutation(ids.size());
	std::iota(permutation.begin(), permutation.end(), (size_t)0);
	std::sort(permutation.begin(), permutation.end(), [&](size_t a, size_t b) { return ids[a] < ids[b]; });
	std::vector<size_t> invertedPermutation(ids.size());
	bool isAlreadySorted = true;
	for(size_t i = 0; i < permutation.size(); i++) {
		invertedPermutation[permutation[i]] = i;
		if(permutation[i] != i) isAlreadySorted = false;
	}
	if(isAlreadySorted) return;

	// Reorder all values in the particle property arrays.
	for(const PropertyPtr& prop : particleProperties()) {
		PropertyStorage copy(*prop);
		prop->mappedCopyFrom(copy, invertedPermutation);
	}

	// Update bond topology data to match new particle ordering.
	if(PropertyAccess<ParticleIndexPair> bondTopology = findStandardBondProperty(BondsObject::TopologyProperty)) {
		for(ParticleIndexPair& bond : bondTopology) {
			if(bond[0] >= 0 && (size_t)bond[0] < invertedPermutation.size())
				bond[0] = invertedPermutation[bond[0]];
			if(bond[1] >= 0 && (size_t)bond[1] < invertedPermutation.size())
				bond[1] = invertedPermutation[bond[1]];
		}
	}
}

}	// End of namespace
}	// End of namespace
