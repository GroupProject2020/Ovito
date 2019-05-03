///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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

#include <plugins/mesh/Mesh.h>
#include "SurfaceMeshFaces.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshFaces);

/******************************************************************************
* Creates a storage object for standard face properties.
******************************************************************************/
PropertyPtr SurfaceMeshFaces::OOMetaClass::createStandardStorage(size_t faceCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath) const
{
	int dataType;
	size_t componentCount;
	size_t stride;

	switch(type) {
	case RegionProperty:
	case FaceTypeProperty:
		dataType = PropertyStorage::Int;
		componentCount = 1;
		stride = sizeof(int);
		break;
	case ColorProperty:
		dataType = PropertyStorage::Float;
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Color));
		break;
	case BurgersVectorProperty:
	case CrystallographicNormalProperty:
		dataType = PropertyStorage::Float;
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Vector3));
		break;
	default:
		OVITO_ASSERT_MSG(false, "SurfaceMeshFaces::createStandardStorage", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard face property type: %1").arg(type));
	}
	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));

	PropertyPtr property = std::make_shared<PropertyStorage>(faceCount, dataType, componentCount, stride,
								propertyName, false, type, componentNames);

	if(initializeMemory) {
		// Default-initialize property values with zeros.
		std::memset(property->data(), 0, property->size() * property->stride());
	}

	return property;
}

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void SurfaceMeshFaces::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	setPropertyClassDisplayName(tr("Mesh Faces"));
	setElementDescriptionName(QStringLiteral("faces"));
	setPythonName(QStringLiteral("faces"));

	const QStringList emptyList;
	const QStringList xyzList = QStringList() << "X" << "Y" << "Z";
	const QStringList rgbList = QStringList() << "R" << "G" << "B";

	registerStandardProperty(ColorProperty, tr("Color"), PropertyStorage::Float, rgbList, tr("Face colors"));
	registerStandardProperty(FaceTypeProperty, tr("Type"), PropertyStorage::Int, emptyList);
	registerStandardProperty(RegionProperty, tr("Region"), PropertyStorage::Int, emptyList);
	registerStandardProperty(BurgersVectorProperty, tr("Burgers vector"), PropertyStorage::Float, xyzList, tr("Burgers vectors"));
	registerStandardProperty(CrystallographicNormalProperty, tr("Crystallographic normal"), PropertyStorage::Float, xyzList);
}

}	// End of namespace
}	// End of namespace
