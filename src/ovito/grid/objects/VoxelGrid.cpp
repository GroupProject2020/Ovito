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

#include <ovito/grid/Grid.h>
#include "VoxelGrid.h"

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(VoxelGrid);
DEFINE_PROPERTY_FIELD(VoxelGrid, shape);
DEFINE_REFERENCE_FIELD(VoxelGrid, domain);
DEFINE_PROPERTY_FIELD(VoxelGrid, title);
SET_PROPERTY_FIELD_LABEL(VoxelGrid, shape, "Shape");
SET_PROPERTY_FIELD_LABEL(VoxelGrid, domain, "Domain");
SET_PROPERTY_FIELD_LABEL(VoxelGrid, title, "Title");

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void VoxelGrid::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	// Enable automatic conversion of a VoxelPropertyReference to a generic PropertyReference and vice versa.
	QMetaType::registerConverter<VoxelPropertyReference, PropertyReference>();
	QMetaType::registerConverter<PropertyReference, VoxelPropertyReference>();

	setPropertyClassDisplayName(tr("Voxel grid"));
	setElementDescriptionName(QStringLiteral("voxels"));
	setPythonName(QStringLiteral("voxels"));

	const QStringList emptyList;
	const QStringList rgbList = QStringList() << "R" << "G" << "B";

	registerStandardProperty(ColorProperty, tr("Color"), PropertyStorage::Float, rgbList, tr("Voxel colors"));
}

/******************************************************************************
* Creates a storage object for standard voxel properties.
******************************************************************************/
PropertyPtr VoxelGrid::OOMetaClass::createStandardStorage(size_t voxelCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath) const
{
	int dataType;
	size_t componentCount;
	size_t stride;

	switch(type) {
	case ColorProperty:
		dataType = PropertyStorage::Float;
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Color));
		break;
	default:
		OVITO_ASSERT_MSG(false, "VoxelGrid::createStandardStorage", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard voxel property type: %1").arg(type));
	}
	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));

	PropertyPtr property = std::make_shared<PropertyStorage>(voxelCount, dataType, componentCount, stride,
								propertyName, false, type, componentNames);

	if(initializeMemory) {
		// Default-initialize property values with zeros.
		std::memset(property->data(), 0, property->size() * property->stride());
	}

	return property;
}

/******************************************************************************
* Constructor.
******************************************************************************/
VoxelGrid::VoxelGrid(DataSet* dataset, const QString& title) : PropertyContainer(dataset),
	_title(title)
{
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void VoxelGrid::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	PropertyContainer::saveToStream(stream, excludeRecomputableData);

	stream.beginChunk(0x01);
	stream.writeSizeT(shape().size());
	for(size_t d : shape())
		stream.writeSizeT(d);
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void VoxelGrid::loadFromStream(ObjectLoadStream& stream)
{
	PropertyContainer::loadFromStream(stream);

	stream.expectChunk(0x01);

	size_t ndim;
	stream.readSizeT(ndim);
	if(ndim != _shape.get().size()) throwException(tr("Invalid voxel grid dimensionality."));

	for(size_t& d : _shape.mutableValue())
		stream.readSizeT(d);

	stream.closeChunk();
}

/******************************************************************************
* Makes sure that all property arrays in this container have a consistent length.
* If this is not the case, the method throws an exception.
******************************************************************************/
void VoxelGrid::verifyIntegrity() const
{
	PropertyContainer::verifyIntegrity();

	size_t expectedElementCount = shape()[0] * shape()[1] * shape()[2];
	if(elementCount() != expectedElementCount) {
		throwException(tr("Property arrays in voxel grid object have wrong length. Array length %1 does not match the number of grid elements %2.").arg(elementCount()).arg(expectedElementCount));
	}
}

}	// End of namespace
}	// End of namespace
