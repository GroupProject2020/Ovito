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

#include <plugins/mesh/Mesh.h>
#include "RenderableSurfaceMesh.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(RenderableSurfaceMesh);

/******************************************************************************
* Constructs an empty surface mesh object.
******************************************************************************/
RenderableSurfaceMesh::RenderableSurfaceMesh(DataSet* dataset, TriMesh surfaceMesh, TriMesh capPolygonsMesh, 
			DataObject* sourceObject, unsigned int generatorDisplayObjectRevision) : DataObject(dataset),
	_surfaceMesh(std::move(surfaceMesh)),
	_capPolygonsMesh(std::move(capPolygonsMesh)),
	_sourceDataObject(sourceObject),
	_generatorDisplayObjectRevision(generatorDisplayObjectRevision)
{
}

/******************************************************************************
* Creates a copy of this object.
******************************************************************************/
OORef<RefTarget> RenderableSurfaceMesh::clone(bool deepCopy, CloneHelper& cloneHelper)
{
	// Let the base class create an instance of this class.
	OORef<RenderableSurfaceMesh> clone = static_object_cast<RenderableSurfaceMesh>(DataObject::clone(deepCopy, cloneHelper));

	// Copy internal data.
	clone->_surfaceMesh = this->_surfaceMesh;
	clone->_capPolygonsMesh = this->_capPolygonsMesh;
	clone->_materialColors = this->_materialColors;
	clone->_sourceDataObject = this->_sourceDataObject;
	clone->_generatorDisplayObjectRevision = this->_generatorDisplayObjectRevision;
	
	return clone;
}

}	// End of namespace
}	// End of namespace
