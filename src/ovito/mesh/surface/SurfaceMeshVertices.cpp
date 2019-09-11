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

#include <ovito/mesh/Mesh.h>
#include "SurfaceMeshVertices.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshVertices);

/******************************************************************************
* Creates a storage object for standard vertex properties.
******************************************************************************/
PropertyPtr SurfaceMeshVertices::OOMetaClass::createStandardStorage(size_t vertexCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath) const
{
	int dataType;
	size_t componentCount;
	size_t stride;

	switch(type) {
	case PositionProperty:
		dataType = PropertyStorage::Float;
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Point3));
		break;
	case ColorProperty:
		dataType = PropertyStorage::Float;
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Color));
		break;
	default:
		OVITO_ASSERT_MSG(false, "SurfaceMeshVertices::createStandardStorage", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard vertex property type: %1").arg(type));
	}
	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));

	PropertyPtr property = std::make_shared<PropertyStorage>(vertexCount, dataType, componentCount, stride,
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
void SurfaceMeshVertices::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	setPropertyClassDisplayName(tr("Mesh Vertices"));
	setElementDescriptionName(QStringLiteral("vertices"));
	setPythonName(QStringLiteral("vertices"));

	const QStringList emptyList;
	const QStringList xyzList = QStringList() << "X" << "Y" << "Z";
	const QStringList rgbList = QStringList() << "R" << "G" << "B";

	registerStandardProperty(ColorProperty, tr("Color"), PropertyStorage::Float, rgbList, tr("Vertex colors"));
	registerStandardProperty(PositionProperty, tr("Position"), PropertyStorage::Float, xyzList, tr("Vertex positions"));
}

}	// End of namespace
}	// End of namespace
