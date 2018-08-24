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
DEFINE_REFERENCE_FIELD(VoxelGrid, properties);
SET_PROPERTY_FIELD_LABEL(VoxelGrid, shape, "Shape");
SET_PROPERTY_FIELD_LABEL(VoxelGrid, properties, "Properties");

/******************************************************************************
* Constructor.
******************************************************************************/
VoxelGrid::VoxelGrid(DataSet* dataset) : PeriodicDomainDataObject(dataset)
{
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void VoxelGrid::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	PeriodicDomainDataObject::saveToStream(stream, excludeRecomputableData);

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
	PeriodicDomainDataObject::loadFromStream(stream);

	stream.expectChunk(0x01);
	size_t ndim;
	stream.readSizeT(ndim);
	std::vector<size_t> s(ndim);
	for(size_t& d : s)
		stream.readSizeT(d);
	setShape(std::move(s));
	stream.closeChunk();
}

}	// End of namespace
}	// End of namespace
