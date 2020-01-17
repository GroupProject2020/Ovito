////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/stdobj/properties/ElementType.h>

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
			if(const BondType* bondType = dynamic_object_cast<BondType>(type))
				m.insert({ type->numericId(), bondType->radius() });
		return m;
	}

	//////////////////////////////////// Default settings ////////////////////////////////

	/// Returns the default color for the bond type with the given ID.
	static Color getDefaultBondColorForId(BondsObject::Type typeClass, int bondTypeId);

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


