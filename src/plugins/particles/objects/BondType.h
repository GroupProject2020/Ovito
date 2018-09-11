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
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/stdobj/properties/ElementType.h>

namespace Ovito { namespace Particles {

/**
 * \brief Stores the properties of a bond type, e.g. name, color, and radius.
 */
class OVITO_PARTICLES_EXPORT BondType : public ElementType
{
	Q_OBJECT
	OVITO_CLASS(BondType)

public:

	/// \brief Constructs a new bond type.
	Q_INVOKABLE BondType(DataSet* dataset);

	//////////////////////////////////// Utility methods ////////////////////////////////
	
	/// Builds a map from type identifiers to bond radii.
	static std::map<int,FloatType> typeRadiusMap(const PropertyObject* typeProperty) {
		std::map<int,FloatType> m;
		for(const ElementType* type : typeProperty->elementTypes())
			m.insert({ type->numericId(), static_object_cast<BondType>(type)->radius() });
		return m;
	}	
	
	//////////////////////////////////// Default settings ////////////////////////////////
	
	/// Returns the default color for the bond type with the given ID.
	static Color getDefaultBondColorFromId(BondsObject::Type typeClass, int bondTypeId);

	/// Returns the default color for a named bond type.
	static Color getDefaultBondColor(BondsObject::Type typeClass, const QString& bondTypeName, int bondTypeId, bool userDefaults = true);

	/// Returns the default radius for a named bond type.
	static FloatType getDefaultBondRadius(BondsObject::Type typeClass, const QString& bondTypeName, int bondTypeId, bool userDefaults = true);
	
private:

	/// Stores the radius of the bond type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, radius, setRadius);
};

}	// End of namespace
}	// End of namespace


