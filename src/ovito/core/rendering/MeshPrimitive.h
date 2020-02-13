////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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


#include <ovito/core/Core.h>
#include "PrimitiveBase.h"

namespace Ovito {

/**
 * \brief Abstract base class for rendering triangle meshes.
 */
class OVITO_CORE_EXPORT MeshPrimitive : public PrimitiveBase
{
public:

	/// Sets the mesh to be stored in this buffer object.
	virtual void setMesh(const TriMesh& mesh, const ColorA& meshColor, bool emphasizeEdges = false) = 0;

	/// \brief Returns the number of triangle faces stored in the buffer.
	virtual int faceCount() = 0;

	/// \brief Enables or disables the culling of triangles not facing the viewer.
	void setCullFaces(bool enable) { _cullFaces = enable; }

	/// \brief Returns whether the culling of triangles not facing the viewer is enabled.
	bool cullFaces() const { return _cullFaces; }

	/// Returns the array of materials referenced by the materialIndex() field of the mesh faces.
	const std::vector<ColorA>& materialColors() const { return _materialColors; }

	/// Sets array of materials referenced by the materialIndex() field of the mesh faces.
	void setMaterialColors(std::vector<ColorA> colors) {
		_materialColors = std::move(colors);
	}

	/// Activates rendering of multiple instances of the mesh.
	virtual void setInstancedRendering(std::vector<AffineTransformation> perInstanceTMs, std::vector<ColorA> perInstanceColors) = 0;

private:

	/// Controls the culling of triangles not facing the viewer.
	bool _cullFaces = false;

	/// The array of materials referenced by the materialIndex() field of the mesh faces.
	std::vector<ColorA> _materialColors;
};

}	// End of namespace
