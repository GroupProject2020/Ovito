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

#pragma once


#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/BondsStorage.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/grid/objects/VoxelProperty.h>
#include <core/dataset/io/FileSourceImporter.h>
#include <core/dataset/data/properties/PropertyStorage.h>
#include <core/dataset/data/simcell/SimulationCell.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import)

/**
 * Holds the data of a single frame loaded by a ParticleImporter.
 */
class OVITO_PARTICLES_EXPORT ParticleFrameData : public FileSourceImporter::FrameData
{
public:

	struct ParticleTypeDefinition {
		int id;
		QString name;
		std::string name8bit;
		Color color;
		FloatType radius;
	};

	class OVITO_PARTICLES_EXPORT ParticleTypeList {
	public:

		/// Defines a new particle type with the given id.
		void addParticleTypeId(int id) {
			for(const auto& type : _particleTypes) {
				if(type.id == id)
					return;
			}
			_particleTypes.push_back({ id, QString(), std::string(), Color(0,0,0), 0 });
		}

		/// Defines a new particle type with the given id.
		void addParticleTypeId(int id, const QString& name, const Color& color = Color(0,0,0), FloatType radius = 0) {
			for(const auto& type : _particleTypes) {
				if(type.id == id)
					return;
			}
			_particleTypes.push_back({ id, name, name.toStdString(), color, radius });
		}

		/// Changes the name of an existing particle type.
		void setParticleTypeName(int id, const QString& name) {
			for(auto& type : _particleTypes) {
				if(type.id == id) {
					type.name = name;
					type.name8bit = name.toStdString();
					break;
				}
			}
		}

		/// Defines a new particle type with the given name.
		inline int addParticleTypeName(const char* name, const char* name_end = nullptr) {
			size_t nameLen = (name_end ? (name_end - name) : qstrlen(name));
			for(const auto& type : _particleTypes) {
				if(type.name8bit.compare(0, type.name8bit.size(), name, nameLen) == 0)
					return type.id;
			}
			int id = _particleTypes.size() + 1;
			_particleTypes.push_back({ id, QString::fromLocal8Bit(name, nameLen), std::string(name, nameLen), Color(0,0,0), 0.0f });
			return id;
		}

		/// Defines a new particle type with the given name.
		inline int addParticleTypeName(const QString& name) {
			for(const auto& type : _particleTypes) {
				if(type.name == name)
					return type.id;
			}
			int id = _particleTypes.size() + 1;
			_particleTypes.push_back({ id, name, name.toStdString(), Color(0,0,0), 0.0f });
			return id;
		}
		
		/// Defines a new particle type with the given name, color, and radius.
		int addParticleTypeName(const char* name, const char* name_end, const Color& color, FloatType radius = 0) {
			size_t nameLen = (name_end ? (name_end - name) : qstrlen(name));
			for(const auto& type : _particleTypes) {
				if(type.name8bit.compare(0, type.name8bit.size(), name, nameLen) == 0)
					return type.id;
			}
			int id = _particleTypes.size() + 1;
			_particleTypes.push_back({ id, QString::fromLocal8Bit(name, nameLen), std::string(name, nameLen), color, radius });
			return id;
		}

		/// Returns the list of particle types.
		const std::vector<ParticleTypeDefinition>& particleTypes() const { return _particleTypes; }

		/// Sorts the particle types w.r.t. their name. Reassigns the per-particle type IDs.
		/// This method is used by file parsers that create particle types on the go while the read the particle data.
		/// In such a case, the assignment of IDs to types depends on the storage order of particles in the file, which is not desirable.
		void sortParticleTypesByName(PropertyStorage* typeProperty);

		/// Sorts particle types with ascending identifier.
		void sortParticleTypesById();

	private:

		/// The list of defined particle types.
		std::vector<ParticleTypeDefinition> _particleTypes;
	};

	struct BondTypeDefinition {
		int id;
		QString name;
		std::string name8bit;
		Color color;
		FloatType radius;
	};

	class OVITO_PARTICLES_EXPORT BondTypeList {
	public:

		/// Defines a new bond type with the given id.
		void addBondTypeId(int id) {
			for(const auto& type : _bondTypes) {
				if(type.id == id)
					return;
			}
			_bondTypes.push_back({ id, QString(), std::string(), Color(0,0,0), 0 });
		}

		/// Defines a new bond type with the given id.
		void addBondTypeId(int id, const QString& name, const Color& color = Color(0,0,0), FloatType radius = 0) {
			for(const auto& type : _bondTypes) {
				if(type.id == id)
					return;
			}
			_bondTypes.push_back({ id, name, name.toStdString(), color, radius });
		}

		/// Changes the name of an existing bond type.
		void setBondTypeName(int id, const QString& name) {
			for(auto& type : _bondTypes) {
				if(type.id == id) {
					type.name = name;
					type.name8bit = name.toStdString();
					break;
				}
			}
		}

		/// Defines a new bond type with the given name.
		inline int addBondTypeName(const char* name, const char* name_end = nullptr) {
			size_t nameLen = (name_end ? (name_end - name) : qstrlen(name));
			for(const auto& type : _bondTypes) {
				if(type.name8bit.compare(0, type.name8bit.size(), name, nameLen) == 0)
					return type.id;
			}
			int id = _bondTypes.size() + 1;
			_bondTypes.push_back({ id, QString::fromLocal8Bit(name, nameLen), std::string(name, nameLen), Color(0,0,0), 0.0f });
			return id;
		}

		/// Defines a new bond type with the given name.
		inline int addBondTypeName(const QString& name) {
			for(const auto& type : _bondTypes) {
				if(type.name == name)
					return type.id;
			}
			int id = _bondTypes.size() + 1;
			_bondTypes.push_back({ id, name, name.toStdString(), Color(0,0,0), 0.0f });
			return id;
		}
		
		/// Defines a new bond type with the given name, color, and radius.
		int addBondTypeName(const char* name, const char* name_end, const Color& color, FloatType radius = 0) {
			size_t nameLen = (name_end ? (name_end - name) : qstrlen(name));
			for(const auto& type : _bondTypes) {
				if(type.name8bit.compare(0, type.name8bit.size(), name, nameLen) == 0)
					return type.id;
			}
			int id = _bondTypes.size() + 1;
			_bondTypes.push_back({ id, QString::fromLocal8Bit(name, nameLen), std::string(name, nameLen), color, radius });
			return id;
		}

		/// Returns the list of bond types.
		const std::vector<BondTypeDefinition>& bondTypes() const { return _bondTypes; }

	private:

		/// The list of bond types.
		std::vector<BondTypeDefinition> _bondTypes;
	};

public:

	/// Constructor.
	ParticleFrameData() {
		// Assume periodic boundary conditions by default.
		_simulationCell.setPbcFlags(true, true, true);
	}

	/// Inserts the loaded data into the provided pipeline state structure. This function is
	/// called by the system from the main thread after the asynchronous loading task has finished.
	virtual PipelineFlowState handOver(DataSet* dataset, const PipelineFlowState& existing, bool isNewFile) override;

	/// Returns the current simulation cell matrix.
	const SimulationCell& simulationCell() const { return _simulationCell; }

	/// Returns a reference to the simulation cell.
	SimulationCell& simulationCell() { return _simulationCell; }

	/// Returns the list of particle properties.
	const std::vector<PropertyPtr>& particleProperties() const { return _particleProperties; }

	/// Returns a standard particle property if defined.
	PropertyStorage* particleProperty(ParticleProperty::Type which) const {
		for(const auto& prop : _particleProperties)
			if(prop->type() == which)
				return prop.get();
		return nullptr;
	}

	/// Adds a new particle property.
	void addParticleProperty(PropertyPtr property, ParticleTypeList* typeList = nullptr) {
		if(typeList) _particleTypeLists[property.get()] = std::unique_ptr<ParticleTypeList>(typeList);
		_particleProperties.push_back(std::move(property));
	}

	/// Removes a particle property from the list.
	void removeParticleProperty(int index) {
		OVITO_ASSERT(index >= 0 && index < _particleProperties.size());
		_particleTypeLists.erase(_particleProperties[index].get());
		_particleProperties.erase(_particleProperties.begin() + index);
	}

	/// Returns the list of types defined for a particle type property.
	ParticleTypeList* getTypeListOfParticleProperty(const PropertyStorage* property) const {
		auto typeList = _particleTypeLists.find(property);
		if(typeList != _particleTypeLists.end()) return typeList->second.get();
		return nullptr;
	}

	/// Returns the list of bond properties.
	const std::vector<PropertyPtr>& bondProperties() const { return _bondProperties; }

	/// Returns a standard bond property if defined.
	PropertyStorage* bondProperty(BondProperty::Type which) const {
		for(const auto& prop : _bondProperties)
			if(prop->type() == which)
				return prop.get();
		return nullptr;
	}

	/// Adds a new bond property.
	void addBondProperty(PropertyPtr property, BondTypeList* typeList = nullptr) {
		if(typeList) _bondTypeLists[property.get()] = std::unique_ptr<BondTypeList>(typeList);
		_bondProperties.push_back(std::move(property));
	}

	/// Removes a bond property from the list.
	void removeBondProperty(int index) {
		OVITO_ASSERT(index >= 0 && index < _bondProperties.size());
		_bondTypeLists.erase(_bondProperties[index].get());
		_bondProperties.erase(_bondProperties.begin() + index);
	}

	/// Returns the list of types defined for a bond type property.
	BondTypeList* getTypeListOfBondProperty(const PropertyStorage* property) const {
		auto typeList = _bondTypeLists.find(property);
		if(typeList != _bondTypeLists.end()) return typeList->second.get();
		return nullptr;
	}

	/// Returns the shape of the voxel grid.
	const std::vector<size_t>& voxelGridShape() const { return _voxelGridShape; }

	/// Sets the shape of the voxel grid.
	void setVoxelGridShape(std::vector<size_t> shape) { _voxelGridShape = std::move(shape); }
	
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

	/// Sets the bonds between particles.
	void setBonds(BondsPtr bonds) { _bonds = std::move(bonds); }

	/// Returns the bonds between particles (if present).
	const BondsPtr& bonds() const { return _bonds; }

private:

	/// Inserts the stored particle types into the given destination object.
	void insertParticleTypes(ParticleProperty* propertyObj, ParticleTypeList* typeList, bool isNewFile);

	/// Inserts the stored bond types into the given destination object.
	void insertBondTypes(BondProperty* propertyObj, BondTypeList* typeList);

private:

	/// The simulation cell.
	SimulationCell _simulationCell;

	/// Particle properties.
	std::vector<PropertyPtr> _particleProperties;

	/// Stores the lists of particle types for type properties.
	std::map<const PropertyStorage*, std::unique_ptr<ParticleTypeList>> _particleTypeLists;

	/// The list of bonds between particles (if present).
	BondsPtr _bonds;

	/// Bond properties.
	std::vector<PropertyPtr> _bondProperties;

	/// Stores the lists of bond types for type properties.
	std::map<const PropertyStorage*, std::unique_ptr<BondTypeList>> _bondTypeLists;

	/// Voxel grid properties.
	std::vector<PropertyPtr> _voxelProperties;

	/// The shape of the voxel grid.
	std::vector<size_t> _voxelGridShape;

	/// The metadata read from the file header.
	QVariantMap _attributes;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


