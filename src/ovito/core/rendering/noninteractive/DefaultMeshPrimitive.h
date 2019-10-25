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
#include <ovito/core/rendering/MeshPrimitive.h>
#include <ovito/core/utilities/mesh/TriMesh.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/**
 * \brief Buffer object that stores triangle mesh geometry to be rendered by a non-interactive renderer.
 */
class OVITO_CORE_EXPORT DefaultMeshPrimitive : public MeshPrimitive
{
public:

	/// Constructor.
	using MeshPrimitive::MeshPrimitive;

	/// Sets the mesh to be stored in this buffer object.
	virtual void setMesh(const TriMesh& mesh, const ColorA& meshColor, bool emphasizeEdges) override {
		// Create a shallow copy of the mesh and store it in this buffer object.
		_mesh = mesh;
		_meshColor = meshColor;
		_emphasizeEdges = emphasizeEdges;
	}

	/// \brief Returns the number of triangle faces stored in the buffer.
	virtual int faceCount() override { return _mesh.faceCount(); }

	/// \brief Returns true if the geometry buffer is filled and can be rendered with the given renderer.
	virtual bool isValid(SceneRenderer* renderer) override;

	/// \brief Renders the geometry.
	virtual void render(SceneRenderer* renderer) override;

	/// Returns a internal triangle mesh.
	const TriMesh& mesh() const { return _mesh; }

	/// Returns the rendering color to be used if the mesh doesn't have per-vertex colors.
	const ColorA& meshColor() const { return _meshColor; }

	/// Returns whether the polygonal edges should be rendered using a wireframe model.
	bool emphasizeEdges() const { return _emphasizeEdges; }

	/// Activates rendering of multiple instances of the mesh.
	virtual void setInstancedRendering(std::vector<AffineTransformation> perInstanceTMs, std::vector<ColorA> perInstanceColors) override {
		OVITO_ASSERT(perInstanceTMs.size() == perInstanceColors.size() || perInstanceColors.empty());
		_perInstanceTMs = std::move(perInstanceTMs);
		_perInstanceColors = std::move(perInstanceColors);
		_useInstancedRendering = true;
	}

	/// Indicates whether rendering of multiple instances of the same mesh is activated.
	bool useInstancedRendering() const { return _useInstancedRendering; }

	/// Returns the list of transformation matrices when rendering multiple instances of the mesh is enabled.
	const std::vector<AffineTransformation>& perInstanceTMs() const { return _perInstanceTMs; }

	/// Returns the list of colors when rendering multiple instances of the mesh is enabled.
	const std::vector<ColorA>& perInstanceColors() const { return _perInstanceColors; }

private:

	/// The mesh storing the geometry.
	TriMesh _mesh;

	/// The rendering color to be used if the mesh doesn't have per-vertex colors.
	ColorA _meshColor;

	/// Controls the rendering of edge wireframe.
	bool _emphasizeEdges = false;

	/// The list of transformation matrices when rendering multiple instances of the mesh.
	std::vector<AffineTransformation> _perInstanceTMs;

	/// The list of colors when rendering multiple instances of the mesh.
	std::vector<ColorA> _perInstanceColors;

	/// Activates the rendering of multiple instances of the same mesh.
	bool _useInstancedRendering = false;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
