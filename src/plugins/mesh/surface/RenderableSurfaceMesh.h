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

#pragma once


#include <plugins/mesh/Mesh.h>
#include <core/dataset/data/DataObject.h>
#include <core/dataset/data/VersionedDataObjectRef.h>
#include <core/utilities/mesh/TriMesh.h>

namespace Ovito { namespace Mesh {

/**
 * \brief A non-periodic triangle mesh that is generated from a periodic SurfaceMesh.
 */
class OVITO_MESH_EXPORT RenderableSurfaceMesh : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(RenderableSurfaceMesh)
	Q_CLASSINFO("DisplayName", "Renderable surface mesh");	
	
public:

	/// \brief Constructor.
	Q_INVOKABLE RenderableSurfaceMesh(DataSet* dataset, TriMesh surfaceMesh = TriMesh(), TriMesh capPolygonsMesh = TriMesh(), 
						DataObject* sourceObject = nullptr, unsigned int generatorDisplayObjectRevision = 0);

	/// \brief Returns whether this object, when returned as an editable sub-object by another object,
	///        should be displayed in the modification stack.
	///
	/// Return false because this object cannot be edited.
	virtual bool isSubObjectEditable() const override { return false; }

	/// Provides access to the surface part of the mesh.
	const TriMesh& surfaceMesh() const { return _surfaceMesh; }

	/// Provides access to the surface part of the mesh.
	TriMesh& surfaceMesh() { return _surfaceMesh; }

	/// Provides access to the cap polygon part of the mesh.
	const TriMesh& capPolygonsMesh() const { return _capPolygonsMesh; }

	/// Provides access to the cap polygon part of the mesh.
	TriMesh& capPolygonsMesh() { return _capPolygonsMesh; }

	/// Returns the material colors assigned to the surface mesh.
	const std::vector<ColorA>& materialColors() const { return _materialColors; }
		
	/// Provides access to the material colors assigned to the surface mesh.
	std::vector<ColorA>& materialColors() { return _materialColors; }

	/// Returns the source object from which this RenderableSurfaceMesh was created from.
	const VersionedDataObjectRef& sourceDataObject() const { return _sourceDataObject; }

	/// Stores a revision version number of the DisplayObject that created this RenderableSurfaceMesh. 
	unsigned int generatorDisplayObjectRevision() const { return _generatorDisplayObjectRevision; }
	
protected:

	/// Creates a copy of this object.
	virtual OORef<RefTarget> clone(bool deepCopy, CloneHelper& cloneHelper) override;

private:

	/// Stores a weak reference to + revision version number of the original DataObject 
	/// from which this RenderableSurfaceMesh was derived. 
	/// We use this detected changes to the source object and avoid unnecesary regeneration 
	/// of the renderable mesh if it didn't change.
	VersionedDataObjectRef _sourceDataObject;

	/// Stores a revision version number of the DisplayObject that created this RenderableSurfaceMesh. 
	/// We use this detected changes to the DisplayObject's parameters that require a re-generation of the
	/// renderable mesh.
	unsigned int _generatorDisplayObjectRevision;
	
	TriMesh _surfaceMesh;
	TriMesh _capPolygonsMesh;
	std::vector<ColorA> _materialColors;
};

}	// End of namespace
}	// End of namespace


