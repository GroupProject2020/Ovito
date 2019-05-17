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
#include <plugins/particles/objects/ParticlesObject.h>
#include <plugins/stdobj/properties/ElementType.h>
#include <plugins/mesh/tri/TriMeshObject.h>

namespace Ovito { namespace Particles {

/**
 * \brief Stores the properties of a particle type, e.g. name, color, and radius.
 */
class OVITO_PARTICLES_EXPORT ParticleType : public ElementType
{
	Q_OBJECT
	OVITO_CLASS(ParticleType)

public:

	enum PredefinedParticleType {
		H,He,Li,C,N,O,Na,Mg,Al,Si,K,Ca,Ti,Cr,Fe,Co,Ni,Cu,Zn,Ga,Ge,Kr,Sr,Y,Zr,Nb,Pd,Pt,W,Au,Pb,Bi,

		NUMBER_OF_PREDEFINED_PARTICLE_TYPES
	};

	enum PredefinedStructureType {
		OTHER = 0,					//< Unidentified structure
		FCC,						//< Face-centered cubic
		HCP,						//< Hexagonal close-packed
		BCC,						//< Body-centered cubic
		ICO,						//< Icosahedral structure
		CUBIC_DIAMOND,				//< Cubic diamond structure
		CUBIC_DIAMOND_FIRST_NEIGH,	//< First neighbor of a cubic diamond atom
		CUBIC_DIAMOND_SECOND_NEIGH,	//< Second neighbor of a cubic diamond atom
		HEX_DIAMOND,				//< Hexagonal diamond structure
		HEX_DIAMOND_FIRST_NEIGH,	//< First neighbor of a hexagonal diamond atom
		HEX_DIAMOND_SECOND_NEIGH,	//< Second neighbor of a hexagonal diamond atom
		SC,							//< Simple cubic structure
		GRAPHENE,						//< Graphene structure

		NUMBER_OF_PREDEFINED_STRUCTURE_TYPES
	};

public:

	/// \brief Constructs a new particle type.
	Q_INVOKABLE ParticleType(DataSet* dataset);

	//////////////////////////////////// Utility methods ////////////////////////////////

	/// Builds a map from type identifiers to particle radii.
	static std::map<int,FloatType> typeRadiusMap(const PropertyObject* typeProperty) {
		std::map<int,FloatType> m;
		for(const ElementType* type : typeProperty->elementTypes())
			m.insert({ type->numericId(), static_object_cast<ParticleType>(type)->radius() });
		return m;
	}

	//////////////////////////////////// Default settings ////////////////////////////////

	/// Returns the name string of a predefined particle type.
	static const QString& getPredefinedParticleTypeName(PredefinedParticleType predefType) {
		OVITO_ASSERT(predefType < NUMBER_OF_PREDEFINED_PARTICLE_TYPES);
		return std::get<0>(_predefinedParticleTypes[predefType]);
	}

	/// Returns the name string of a predefined structure type.
	static const QString& getPredefinedStructureTypeName(PredefinedStructureType predefType) {
		OVITO_ASSERT(predefType < NUMBER_OF_PREDEFINED_STRUCTURE_TYPES);
		return std::get<0>(_predefinedStructureTypes[predefType]);
	}

	/// Returns the default color for the particle type with the given ID.
	static Color getDefaultParticleColorFromId(ParticlesObject::Type typeClass, int particleTypeId);

	/// Returns the default color for a named particle type.
	static Color getDefaultParticleColor(ParticlesObject::Type typeClass, const QString& particleTypeName, int particleTypeId, bool userDefaults = true);

	/// Changes the default color for a named particle type.
	static void setDefaultParticleColor(ParticlesObject::Type typeClass, const QString& particleTypeName, const Color& color);

	/// Returns the default radius for a named particle type.
	static FloatType getDefaultParticleRadius(ParticlesObject::Type typeClass, const QString& particleTypeName, int particleTypeId, bool userDefaults = true);

	/// Changes the default radius for a named particle type.
	static void setDefaultParticleRadius(ParticlesObject::Type typeClass, const QString& particleTypeName, FloatType radius);

private:

	/// The default display radius to be used for particles of this type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, radius, setRadius);

	/// An optional user-defined shape used for rendering particles of this type.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(TriMeshObject, shapeMesh, setShapeMesh);

private:

	/// Data structure that holds the name, color, and radius of a particle type.
	typedef std::tuple<QString,Color,FloatType> PredefinedTypeInfo;

	/// Contains default names, colors, and radii for some predefined particle types.
	static std::array<PredefinedTypeInfo, NUMBER_OF_PREDEFINED_PARTICLE_TYPES> _predefinedParticleTypes;

	/// Contains default names, colors, and radii for the predefined structure types.
	static std::array<PredefinedTypeInfo, NUMBER_OF_PREDEFINED_STRUCTURE_TYPES> _predefinedStructureTypes;
};

}	// End of namespace
}	// End of namespace
