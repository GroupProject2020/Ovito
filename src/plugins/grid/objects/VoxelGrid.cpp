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

#include <plugins/grid/Grid.h>
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

}	// End of namespace
}	// End of namespace
