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
