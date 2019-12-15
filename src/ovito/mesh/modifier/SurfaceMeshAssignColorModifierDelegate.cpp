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
#include <ovito/stdobj/properties/PropertyContainer.h>
#include "SurfaceMeshAssignColorModifierDelegate.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshVerticesAssignColorModifierDelegate);
IMPLEMENT_OVITO_CLASS(SurfaceMeshFacesAssignColorModifierDelegate);

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> SurfaceMeshVerticesAssignColorModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
	// Gather list of all surface mesh vertices in the input data collection.
	QVector<DataObjectReference> objects;
	for(const ConstDataObjectPath& path : input.getObjectsRecursive(SurfaceMeshVertices::OOClass())) {
		objects.push_back(path);
	}
	return objects;
}

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> SurfaceMeshFacesAssignColorModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
	// Gather list of all surface mesh vertices in the input data collection.
	QVector<DataObjectReference> objects;
	for(const ConstDataObjectPath& path : input.getObjectsRecursive(SurfaceMeshFaces::OOClass())) {
		objects.push_back(path);
	}
	return objects;
}

}	// End of namespace
}	// End of namespace
