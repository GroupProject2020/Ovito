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
#include <ovito/stdobj/properties/PropertyAccess.h>
#include "SurfaceMeshRegions.h"
#include "SurfaceMeshVis.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshRegions);

/******************************************************************************
* Creates a storage object for standard region properties.
******************************************************************************/
PropertyPtr SurfaceMeshRegions::OOMetaClass::createStandardStorage(size_t regionCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath) const
{
	int dataType;
	size_t componentCount;
	size_t stride;

	switch(type) {
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
	case PhaseProperty:
		dataType = PropertyStorage::Int;
		componentCount = 1;
		stride = sizeof(int);
		break;
	case VolumeProperty:
	case SurfaceAreaProperty:
		dataType = PropertyStorage::Float;
		componentCount = 1;
		stride = sizeof(FloatType);
		break;
	case LatticeCorrespondenceProperty:
		dataType = PropertyStorage::Float;
		componentCount = 9;
		stride = sizeof(Matrix3);
		break;
	default:
		OVITO_ASSERT_MSG(false, "SurfaceMeshRegions::createStandardStorage", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard region property type: %1").arg(type));
	}
	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));

	PropertyPtr property = std::make_shared<PropertyStorage>(regionCount, dataType, componentCount, stride,
								propertyName, false, type, componentNames);

	// Initialize memory if requested.
	if(initializeMemory && containerPath.size() >= 2) {
		// Certain standard properties need to be initialized with default values determined by the attached visual elements.
		if(type == ColorProperty) {
			if(const SurfaceMesh* surfaceMesh = dynamic_object_cast<SurfaceMesh>(containerPath[containerPath.size()-2])) {
				if(SurfaceMeshVis* vis = surfaceMesh->visElement<SurfaceMeshVis>()) {
					PropertyAccess<Color>(property).fill(vis->surfaceColor());
					initializeMemory = false;
				}
			}
		}
	}

	if(initializeMemory) {
		// Default-initialize property values with zeros.
		property->fillZero();
	}

	return property;
}

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void SurfaceMeshRegions::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	setPropertyClassDisplayName(tr("Mesh Regions"));
	setElementDescriptionName(QStringLiteral("regions"));
	setPythonName(QStringLiteral("regions"));

	const QStringList emptyList;
	const QStringList rgbList = QStringList() << "R" << "G" << "B";
	const QStringList tensorList = QStringList() << "XX" << "YX" << "ZX" << "XY" << "YY" << "ZY" << "XZ" << "YZ" << "ZZ";

	registerStandardProperty(SelectionProperty, tr("Selection"), PropertyStorage::Int, emptyList);
	registerStandardProperty(ColorProperty, tr("Color"), PropertyStorage::Float, rgbList, tr("Region colors"));
	registerStandardProperty(PhaseProperty, tr("Phase"), PropertyStorage::Int, emptyList, tr("Phases"));
	registerStandardProperty(VolumeProperty, tr("Volume"), PropertyStorage::Float, emptyList);
	registerStandardProperty(SurfaceAreaProperty, tr("Surface Area"), PropertyStorage::Float, emptyList);
	registerStandardProperty(LatticeCorrespondenceProperty, tr("Lattice Correspondence"), PropertyStorage::Float, tensorList);
}

/******************************************************************************
* Generates a human-readable string representation of the data object reference.
******************************************************************************/
QString SurfaceMeshRegions::OOMetaClass::formatDataObjectPath(const ConstDataObjectPath& path) const
{
	QString str;
	for(const DataObject* obj : path) {
		if(!str.isEmpty())
			str += QStringLiteral(u" \u2192 ");  // Unicode arrow
		str += obj->objectTitle();
	}
	return str;
}

}	// End of namespace
}	// End of namespace
