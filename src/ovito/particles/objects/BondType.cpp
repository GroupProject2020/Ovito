////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/core/utilities/units/UnitsManager.h>
#include "BondType.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(BondType);
DEFINE_PROPERTY_FIELD(BondType, radius);
SET_PROPERTY_FIELD_LABEL(BondType, radius, "Radius");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(BondType, radius, WorldParameterUnit, 0);

/******************************************************************************
* Constructs a new BondType.
******************************************************************************/
BondType::BondType(DataSet* dataset) : ElementType(dataset), _radius(0)
{
}

/******************************************************************************
* Returns the default color for a bond type ID.
******************************************************************************/
Color BondType::getDefaultBondColorForId(BondsObject::Type typeClass, int bondTypeId)
{
	// Initial standard colors assigned to new bond types:
	static const Color defaultTypeColors[] = {
		Color(1.0,  1.0,  0.0),
		Color(0.7,  0.0,  1.0),
		Color(0.2,  1.0,  1.0),
		Color(1.0,  0.4,  1.0),
		Color(0.4,  1.0,  0.4),
		Color(1.0,  0.4,  0.4),
		Color(0.4,  0.4,  1.0),
		Color(1.0,  1.0,  0.7),
		Color(0.97, 0.97, 0.97)
	};
	return defaultTypeColors[std::abs(bondTypeId) % (sizeof(defaultTypeColors) / sizeof(defaultTypeColors[0]))];
}

/******************************************************************************
* Returns the default color for a bond type name.
******************************************************************************/
Color BondType::getDefaultBondColor(BondsObject::Type typeClass, const QString& bondTypeName, int bondTypeId, bool userDefaults)
{
	if(userDefaults) {
		QSettings settings;
		settings.beginGroup("bonds/defaults/color");
		settings.beginGroup(QString::number((int)typeClass));
		QVariant v = settings.value(bondTypeName);
		if(v.isValid() && v.type() == QVariant::Color)
			return v.value<Color>();
	}

	return getDefaultBondColorForId(typeClass, bondTypeId);
}

/******************************************************************************
* Returns the default radius for a bond type name.
******************************************************************************/
FloatType BondType::getDefaultBondRadius(BondsObject::Type typeClass, const QString& bondTypeName, int bondTypeId, bool userDefaults)
{
	if(userDefaults) {
		QSettings settings;
		settings.beginGroup("bonds/defaults/radius");
		settings.beginGroup(QString::number((int)typeClass));
		QVariant v = settings.value(bondTypeName);
		if(v.isValid() && v.canConvert<FloatType>())
			return v.value<FloatType>();
	}

	return 0;
}

}	// End of namespace
}	// End of namespace
