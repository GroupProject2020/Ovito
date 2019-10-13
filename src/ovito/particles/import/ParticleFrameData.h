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

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/grid/objects/VoxelGrid.h>
#include <ovito/core/dataset/io/FileSourceImporter.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/stdobj/simcell/SimulationCell.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import)

/**
 * Holds the data of a single frame loaded by a ParticleImporter.
 */
class OVITO_PARTICLES_EXPORT ParticleFrameData : public FileSourceImporter::FrameData
{
public:

	/// Used to describe particle and bond types.
	struct TypeDefinition {
		int id;
		QString name;
		std::string name8bit;
		Color color;
		FloatType radius;
		FloatType mass;
		std::shared_ptr<TriMesh> shapeMesh;
	};

	/// Used to store the lists of particle/bond types.
	class OVITO_PARTICLES_EXPORT TypeList
	{
	public:

		/// Defines a new particle/bond type with the given id.
		void addTypeId(int id) {
			for(const auto& type : _types) {
				if(type.id == id)
					return;
			}
			_types.push_back({ id, QString(), std::string(), Color(0,0,0), 0.0, 0.0 });
		}

		/// Defines a new type with the given id.
		void addTypeId(int id, const QString& name, const Color& color = Color(0,0,0), FloatType radius = 0, FloatType mass = 0) {
			for(const auto& type : _types) {
				if(type.id == id)
					return;
			}
			_types.push_back({ id, name, name.toStdString(), color, radius, mass });
		}

		/// Changes the name of an existing type.
		void setTypeName(int id, const QString& name) {
			for(auto& type : _types) {
				if(type.id == id) {
					type.name = name;
					type.name8bit = name.toStdString();
					break;
				}
			}
		}

		/// Changes the mass of an existing type.
		void setTypeMass(int id, FloatType mass) {
			for(auto& type : _types) {
				if(type.id == id) {
					type.mass = mass;
					break;
				}
			}
		}

		/// Changes the radius of an existing type.
		void setTypeRadius(int id, FloatType radius) {
			for(auto& type : _types) {
				if(type.id == id) {
					type.radius = radius;
					break;
				}
			}
		}

		/// Assigns a user-defined shape to an existing type.
		void setTypeShape(int id, std::shared_ptr<TriMesh> shape) {
			for(auto& type : _types) {
				if(type.id == id) {
					type.shapeMesh = std::move(shape);
					break;
				}
			}
		}

		/// Defines a new type with the given name.
		inline int addTypeName(const char* name, const char* name_end = nullptr) {
			size_t nameLen = (name_end ? (name_end - name) : qstrlen(name));
			for(const auto& type : _types) {
				if(type.name8bit.compare(0, type.name8bit.size(), name, nameLen) == 0)
					return type.id;
			}
			int id = _types.size() + 1;
			_types.push_back({ id, QString::fromLocal8Bit(name, nameLen), std::string(name, nameLen), Color(0,0,0), 0.0, 0.0 });
			return id;
		}

		/// Defines a new type with the given name.
		inline int addTypeName(const QString& name) {
			for(const auto& type : _types) {
				if(type.name == name)
					return type.id;
			}
			int id = _types.size() + 1;
			_types.push_back({ id, name, name.toStdString(), Color(0,0,0), 0.0, 0.0 });
			return id;
		}

		/// Defines a new type with the given name, color, and radius.
		int addTypeName(const char* name, const char* name_end, const Color& color, FloatType radius = 0, FloatType mass = 0) {
			size_t nameLen = (name_end ? (name_end - name) : qstrlen(name));
			for(const auto& type : _types) {
				if(type.name8bit.compare(0, type.name8bit.size(), name, nameLen) == 0)
					return type.id;
			}
			int id = _types.size() + 1;
			_types.push_back({ id, QString::fromLocal8Bit(name, nameLen), std::string(name, nameLen), color, radius, mass });
			return id;
		}

		/// Returns the list of particle or bond types.
		const std::vector<TypeDefinition>& types() const { return _types; }

		/// Returns the list of particle or bond types.
		std::vector<TypeDefinition>& types() { return _types; }

		/// Sorts the types w.r.t. their name. Reassigns the per-element type IDs.
		/// This method is used by file parsers that create particle/bond types on the go while the read the data.
		/// In such a case, the assignment of IDs to types depends on the storage order of particles/bonds in the file, which is not desirable.
		void sortTypesByName(const PropertyPtr& typeProperty);

		/// Sorts types according to numeric identifier.
		void sortTypesById();

	private:

		/// The list of defined types.
		std::vector<TypeDefinition> _types;
	};

public:

	/// Constructor.
	ParticleFrameData() {
		// Assume periodic boundary conditions by default.
		_simulationCell.setPbcFlags(true, true, true);
	}

	/// Inserts the loaded data into the provided pipeline state structure. This function is
	/// called by the system from the main thread after the asynchronous loading task has finished.
	virtual OORef<DataCollection> handOver(const DataCollection* existing, bool isNewFile, FileSource* fileSource) override;

	/// Returns the current simulation cell matrix.
	const SimulationCell& simulationCell() const { return _simulationCell; }

	/// Returns a reference to the simulation cell.
	SimulationCell& simulationCell() { return _simulationCell; }

	/// Returns the list of particle properties.
	const std::vector<PropertyPtr>& particleProperties() const { return _particleProperties; }

	/// Returns a standard particle property if already defined.
	PropertyPtr findStandardParticleProperty(ParticlesObject::Type which) const {
		OVITO_ASSERT(which != ParticlesObject::UserProperty);
		for(const auto& prop : _particleProperties)
			if(prop->type() == which)
				return prop;
		return {};
	}

	/// Finds a particle property by name.
	PropertyPtr findParticleProperty(const QString& name) const {
		for(const auto& prop : _particleProperties)
			if(prop->name() == name)
				return prop;
		return {};
	}

	/// Adds a new particle property.
	void addParticleProperty(PropertyPtr property) {
		_particleProperties.push_back(std::move(property));
	}

	/// Removes a particle property from the list.
	void removeParticleProperty(int index) {
		OVITO_ASSERT(index >= 0 && index < _particleProperties.size());
		_typeLists.erase(_particleProperties[index].get());
		_particleProperties.erase(_particleProperties.begin() + index);
	}

	/// Removes a particle property from the list.
	void removeParticleProperty(const PropertyPtr& property) {
		auto iter = std::find(_particleProperties.begin(), _particleProperties.end(), property);
		OVITO_ASSERT(iter != _particleProperties.end());
		_typeLists.erase(property.get());
		_particleProperties.erase(iter);
	}

	/// Returns the list of types defined for a particle or bond property.
	TypeList* propertyTypesList(const PropertyPtr& property) {
		auto typeList = _typeLists.find(property.get());
		if(typeList == _typeLists.end()) {
			typeList = _typeLists.emplace(property.get(), std::make_unique<TypeList>()).first;
		}
		return typeList->second.get();
	}

	/// Sets the list of types defined for a particle or bond property.
	void setPropertyTypesList(const PropertyPtr& property, std::unique_ptr<TypeList> list) {
		_typeLists.emplace(property.get(), std::move(list));
	}

	/// Returns the list of bond properties.
	const std::vector<PropertyPtr>& bondProperties() const { return _bondProperties; }

	/// Returns a standard bond property if already defined.
	PropertyPtr findStandardBondProperty(BondsObject::Type which) const {
		OVITO_ASSERT(which != BondsObject::UserProperty);
		for(const auto& prop : _bondProperties)
			if(prop->type() == which)
				return prop;
		return {};
	}

	/// Adds a new bond property.
	void addBondProperty(PropertyPtr property) {
		_bondProperties.push_back(std::move(property));
	}

	/// Removes a bond property from the list.
	void removeBondProperty(int index) {
		OVITO_ASSERT(index >= 0 && index < _bondProperties.size());
		_typeLists.erase(_bondProperties[index].get());
		_bondProperties.erase(_bondProperties.begin() + index);
	}

	/// Determines the PBC shift vectors for bonds using the minimum image convention.
	void generateBondPeriodicImageProperty();

	/// Returns the shape of the voxel grid.
	const VoxelGrid::GridDimensions& voxelGridShape() const { return _voxelGridShape; }

	/// Sets the shape of the voxel grid.
	void setVoxelGridShape(const VoxelGrid::GridDimensions& shape) { _voxelGridShape = shape; }

	/// Returns the human-readable name being assigned to the loaded voxel grid.
	const QString& voxelGridTitle() const { return _voxelGridTitle; }

	/// Sets the human-readable name that will be assigned to the voxel grid.
	void setVoxelGridTitle(const QString& title) { _voxelGridTitle = title; }

	/// Returns the unique data object ID being assigned to the loaded voxel grid.
	const QString& voxelGridId() const { return _voxelGridId; }

	/// Sets the unique data object ID that will be assigned to the voxel grid.
	void setVoxelGridId(const QString& id) { _voxelGridId = id; }

	/// Returns the list of voxel properties.
	const std::vector<PropertyPtr>& voxelProperties() const { return _voxelProperties; }

	/// Adds a new voxel grid property.
	void addVoxelProperty(PropertyPtr quantity) {
		_voxelProperties.push_back(std::move(quantity));
	}

	/// Removes a voxel grid property from the list.
	void removeVoxelProperty(int index) {
		OVITO_ASSERT(index >= 0 && index < _voxelProperties.size());
		_voxelProperties.erase(_voxelProperties.begin() + index);
	}

	/// Returns the metadata read from the file header.
	QVariantMap& attributes() { return _attributes; }

	/// Parsers call this method to indicate that the input file contains
	/// additional frames stored back to back with the currently loaded one.
	void signalAdditionalFrames() { _detectedAdditionalFrames = true; }

	/// Sorts the particles list with respect to particle IDs.
	/// Does nothing if particles do not have IDs.
	void sortParticlesById();

private:

	/// Inserts the stored particle or bond types into the given property object.
	void insertTypes(PropertyObject* propertyObj, TypeList* typeList, bool isNewFile, bool isBondProperty);

private:

	/// The simulation cell.
	SimulationCell _simulationCell;

	/// Particle properties.
	std::vector<PropertyPtr> _particleProperties;

	/// Bond properties.
	std::vector<PropertyPtr> _bondProperties;

	/// Voxel grid properties.
	std::vector<PropertyPtr> _voxelProperties;

	/// The dimensions of the voxel grid.
	VoxelGrid::GridDimensions _voxelGridShape{{0,0,0}};

	/// The human-readable name to assign to the loaded voxel grid.
	QString _voxelGridTitle;

	/// The ID to assign to the voxel grid data object.
	QString _voxelGridId;

	/// Stores the lists of types for typed properties (both particle and bond properties).
	std::map<const PropertyStorage*, std::unique_ptr<TypeList>> _typeLists;

	/// The metadata read from the file header.
	QVariantMap _attributes;

	/// Flag that is set by the parser to indicate that the input file contains more than one frame.
	bool _detectedAdditionalFrames = false;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
