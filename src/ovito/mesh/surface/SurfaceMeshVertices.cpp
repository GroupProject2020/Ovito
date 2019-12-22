////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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

#include <ovito/mesh/Mesh.h>
#include "SurfaceMeshVertices.h"
#include "SurfaceMeshVis.h"

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
	case SelectionProperty:
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
	default:
		OVITO_ASSERT_MSG(false, "SurfaceMeshVertices::createStandardStorage", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard vertex property type: %1").arg(type));
	}
	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));

	PropertyPtr property = std::make_shared<PropertyStorage>(vertexCount, dataType, componentCount, stride,
								propertyName, false, type, componentNames);

	// Initialize memory if requested.
	if(initializeMemory && containerPath.size() >= 2) {
		// Certain standard properties need to be initialized with default values determined by the attached visual elements.
		if(type == ColorProperty) {
			if(const SurfaceMesh* surfaceMesh = dynamic_object_cast<SurfaceMesh>(containerPath[containerPath.size()-2])) {
				if(SurfaceMeshVis* vis = surfaceMesh->visElement<SurfaceMeshVis>()) {
					std::fill(property->dataColor(), property->dataColor() + property->size(), vis->surfaceColor());
					initializeMemory = false;
				}
			}
		}
	}

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

	registerStandardProperty(SelectionProperty, tr("Selection"), PropertyStorage::Int, emptyList);
	registerStandardProperty(ColorProperty, tr("Color"), PropertyStorage::Float, rgbList, tr("Vertex colors"));
	registerStandardProperty(PositionProperty, tr("Position"), PropertyStorage::Float, xyzList, tr("Vertex positions"));
}

}	// End of namespace
}	// End of namespace
