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
#include "RenderableSurfaceMesh.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(RenderableSurfaceMesh);
DEFINE_PROPERTY_FIELD(RenderableSurfaceMesh, surfaceMesh);
DEFINE_PROPERTY_FIELD(RenderableSurfaceMesh, capPolygonsMesh);
DEFINE_PROPERTY_FIELD(RenderableSurfaceMesh, materialColors);
DEFINE_PROPERTY_FIELD(RenderableSurfaceMesh, originalFaceMap);
DEFINE_PROPERTY_FIELD(RenderableSurfaceMesh, backfaceCulling);

/******************************************************************************
* Initialization constructor.
******************************************************************************/
RenderableSurfaceMesh::RenderableSurfaceMesh(TransformingDataVis* creator, const DataObject* sourceData, TriMesh surfaceMesh, TriMesh capPolygonsMesh, bool backfaceCulling) :
	TransformedDataObject(creator, sourceData),
	_surfaceMesh(std::move(surfaceMesh)),
	_capPolygonsMesh(std::move(capPolygonsMesh)),
	_backfaceCulling(backfaceCulling)
{
	// Adopt the ID string from the original data object.
	if(sourceData)
		setIdentifier(sourceData->identifier());
}

}	// End of namespace
}	// End of namespace
