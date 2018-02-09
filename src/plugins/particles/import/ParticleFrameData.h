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
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <plugins/stdobj/simcell/SimulationCell.h>

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
			_types.push_back({ id, QString(), std::string(), Color(0,0,0), 0 });
		}

		/// Defines a new type with the given id.
		void addTypeId(int id, const QString& name, const Color& color = Color(0,0,0), FloatType radius = 0) {
			for(const auto& type : _types) {
				if(type.id == id)
					return;
			}
			_types.push_back({ id, name, name.toStdString(), color, radius });
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

		/// Defines a new type with the given name.
		inline int addTypeName(const char* name, const char* name_end = nullptr) {
			size_t nameLen = (name_end ? (name_end - name) : qstrlen(name));
			for(const auto& type : _types) {
				if(type.name8bit.compare(0, type.name8bit.size(), name, nameLen) == 0)
					return type.id;
			}
			int id = _types.size() + 1;
			_types.push_back({ id, QString::fromLocal8Bit(name, nameLen), std::string(name, nameLen), Color(0,0,0), 0.0f });
			return id;
		}

		/// Defines a new type with the given name.
		inline int addTypeName(const QString& name) {
			for(const auto& type : _types) {
				if(type.name == name)
					return type.id;
			}
			int id = _types.size() + 1;
			_types.push_back({ id, name, name.toStdString(), Color(0,0,0), 0.0f });
			return id;
		}
		
		/// Defines a new type with the given name, color, and radius.
		int addTypeName(const char* name, const char* name_end, const Color& color, FloatType radius = 0) {
			size_t nameLen = (name_end ? (name_end - name) : qstrlen(name));
			for(const auto& type : _types) {
				if(type.name8bit.compare(0, type.name8bit.size(), name, nameLen) == 0)
					return type.id;
			}
			int id = _types.size() + 1;
			_types.push_back({ id, QString::fromLocal8Bit(name, nameLen), std::string(name, nameLen), color, radius });
			return id;
		}

		/// Returns the list of particle or bond types.
		const std::vector<TypeDefinition>& types() const { return _types; }

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
	virtual PipelineFlowState handOver(DataSet* dataset, const PipelineFlowState& existing, bool isNewFile, FileSource* fileSource) override;

	/// Returns the current simulation cell matrix.
	const SimulationCell& simulationCell() const { return _simulationCell; }

	/// Returns a reference to the simulation cell.
	SimulationCell& simulationCell() { return _simulationCell; }

	/// Returns the list of particle properties.
	const std::vector<PropertyPtr>& particleProperties() const { return _particleProperties; }

	/// Returns a standard particle property if already defined.
	PropertyPtr findStandardParticleProperty(ParticleProperty::Type which) const {
		OVITO_ASSERT(which != ParticleProperty::UserProperty);
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
	PropertyPtr findStandardBondProperty(BondProperty::Type which) const {
		OVITO_ASSERT(which != BondProperty::UserProperty);
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

	/// Indicates that the file parser found additional frames
	/// in the input file stored back to back with the frame currently being loaded.
	void signalAdditionalFrames() { _detectedAdditionalFrames = true; }

private:

	/// Inserts the stored particle or bond types into the given property object.
	void insertTypes(PropertyObject* propertyObj, TypeList* typeList, bool isNewFile, bool isBondProperty);

private:

	/// The simulation cell.
	SimulationCell _simulationCell;

	/// Particle properties.
	std::vector<PropertyPtr> _particleProperties;

	/// The list of bonds between particles (if present).
	BondsPtr _bonds;

	/// Bond properties.
	std::vector<PropertyPtr> _bondProperties;

	/// Voxel grid properties.
	std::vector<PropertyPtr> _voxelProperties;

	/// The shape of the voxel grid.
	std::vector<size_t> _voxelGridShape;

	/// Stores the lists of types for typed properties (both particle and bond properties).
	std::map<const PropertyStorage*, std::unique_ptr<TypeList>> _typeLists;
	
	/// The metadata read from the file header.
	QVariantMap _attributes;

	/// Flag that indicates that the file parser has found additional frames
	/// in the input file stored back to back with the currently loaded frame.
	bool _detectedAdditionalFrames = false;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
