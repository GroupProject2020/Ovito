///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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
#include "TrajectoryObject.h"
#include "TrajectoryVis.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(TrajectoryObject);

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void TrajectoryObject::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	setPropertyClassDisplayName(tr("Trajectories"));
	setElementDescriptionName(QStringLiteral("vertex"));
	setPythonName(QStringLiteral("trajectories"));

	const QStringList emptyList;
	const QStringList xyzList = QStringList() << "X" << "Y" << "Z";
	registerStandardProperty(PositionProperty, tr("Position"), PropertyStorage::Float, xyzList);
	registerStandardProperty(SampleTimeProperty, tr("Time"), PropertyStorage::Int, emptyList);
	registerStandardProperty(ParticleIdentifierProperty, tr("Particle Identifier"), PropertyStorage::Int64, emptyList);
}

/******************************************************************************
* Creates a storage object for standard properties.
******************************************************************************/
PropertyPtr TrajectoryObject::OOMetaClass::createStandardStorage(size_t elementCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath) const
{
	int dataType;
	size_t componentCount;
	size_t stride;

	switch(type) {
	case PositionProperty:
		dataType = PropertyStorage::Float;
		componentCount = 3;
		stride = sizeof(Point3);
		break;
	case SampleTimeProperty:
		dataType = PropertyStorage::Int;
		componentCount = 1;
		stride = sizeof(int);
		break;
	case ParticleIdentifierProperty:
		dataType = PropertyStorage::Int64;
		componentCount = 1;
		stride = sizeof(qlonglong);
		break;
	default:
		OVITO_ASSERT_MSG(false, "TrajectoryObject::createStandardStorage()", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard property type: %1").arg(type));
	}

	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));
	
	return std::make_shared<PropertyStorage>(elementCount, dataType, componentCount, stride, 
								propertyName, initializeMemory, type, componentNames);
}

/******************************************************************************
* Default constructor.
******************************************************************************/
TrajectoryObject::TrajectoryObject(DataSet* dataset) : PropertyContainer(dataset)
{
	addVisElement(new TrajectoryVis(dataset));
}

}	// End of namespace
}	// End of namespace
