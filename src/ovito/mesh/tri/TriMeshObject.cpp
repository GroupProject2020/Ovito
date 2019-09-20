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
#include "TriMeshObject.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(TriMeshObject);
DEFINE_PROPERTY_FIELD(TriMeshObject, mesh);

/******************************************************************************
* Default constructor.
******************************************************************************/
TriMeshObject::TriMeshObject(DataSet* dataset) : DataObject(dataset)
{
}

/******************************************************************************
* Returns a pointer to the internal data structure after making sure it is
* not shared with any other owners.
******************************************************************************/
const TriMeshPtr& TriMeshObject::modifiableMesh()
{
	// Copy data if there is more than one reference to the storage.
	OVITO_ASSERT(mesh());
	OVITO_ASSERT(mesh().use_count() >= 1);
	if(mesh().use_count() > 1)
		_mesh.mutableValue() = std::make_shared<TriMesh>(*mesh());
	OVITO_ASSERT(mesh().use_count() == 1);
	return mesh();
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void TriMeshObject::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	DataObject::saveToStream(stream, excludeRecomputableData);
	if(mesh()) {
		stream.beginChunk(0x01);
		mesh()->saveToStream(stream);
		stream.endChunk();
	}
	else {
		stream.beginChunk(0x00);
		stream.endChunk();
	}
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void TriMeshObject::loadFromStream(ObjectLoadStream& stream)
{
	DataObject::loadFromStream(stream);
	if(stream.expectChunkRange(0x00, 0x01) != 0) {
		TriMeshPtr m = std::make_shared<TriMesh>();
		m->loadFromStream(stream);
		setMesh(std::move(m));
	}
	stream.closeChunk();
}

}	// End of namespace
}	// End of namespace
