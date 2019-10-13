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

#pragma once


#include <ovito/mesh/Mesh.h>
#include <ovito/core/dataset/data/DataObject.h>
#include <ovito/core/utilities/mesh/TriMesh.h>

namespace Ovito { namespace Mesh {

/**
 * \brief A data object type that consist of a triangle mesh.
 */
class OVITO_MESH_EXPORT TriMeshObject : public DataObject
{
	OVITO_CLASS(TriMeshObject)
	Q_OBJECT

public:

	/// Constructor that creates an object with an empty triangle mesh.
	Q_INVOKABLE TriMeshObject(DataSet* dataset);

	/// \brief Returns the title of this object.
	virtual QString objectTitle() const override { return tr("Triangle mesh"); }

	/// Returns a pointer to the internal data structure after making sure it is not shared with any other owners.
	/// The pointer can be used to modify the mesh. However, each time the mesh is modified,
	/// notifyTargetChanged() must be called to increment the object's revision number.
	const TriMeshPtr& modifiableMesh();

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

private:

	/// The data structure storing the mesh.
	DECLARE_RUNTIME_PROPERTY_FIELD(TriMeshPtr, mesh, setMesh);
};

}	// End of namespace
}	// End of namespace
