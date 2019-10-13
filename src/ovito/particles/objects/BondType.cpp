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

#include <ovito/particles/Particles.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "BondType.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(BondType);
DEFINE_PROPERTY_FIELD(BondType, radius);
SET_PROPERTY_FIELD_LABEL(BondType, radius, "Radius");
SET_PROPERTY_FIELD_UNITS(BondType, radius, WorldParameterUnit);

/******************************************************************************
* Constructs a new BondType.
******************************************************************************/
BondType::BondType(DataSet* dataset) : ElementType(dataset), _radius(0)
{
}

/******************************************************************************
* Returns the default color for a bond type ID.
******************************************************************************/
Color BondType::getDefaultBondColorFromId(BondsObject::Type typeClass, int bondTypeId)
{
	// Assign initial standard color to new bond types.
	static const Color defaultTypeColors[] = {
		Color(1.0f,1.0f,0.0f),
		Color(0.7f,0.0f,1.0f),
		Color(0.2f,1.0f,1.0f),
		Color(1.0f,0.4f,1.0f),
		Color(0.4f,1.0f,0.4f),
		Color(1.0f,0.4f,0.4f),
		Color(0.4f,0.4f,1.0f),
		Color(1.0f,1.0f,0.7f),
		Color(0.97f,0.97f,0.97f)
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

	return getDefaultBondColorFromId(typeClass, bondTypeId);
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
