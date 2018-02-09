///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include <core/dataset/io/FileSourceImporter.h>
#include <core/utilities/mesh/TriMesh.h>

namespace Ovito { namespace Mesh {

/**
 * \brief Base class for file loader reading a triangle mesh from a file.
 */
class TriMeshFrameData : public FileSourceImporter::FrameData
{
public:

	/// Inserts the loaded loaded into the provided pipeline state structure. This function is
	/// called by the system from the main thread after the asynchronous loading task has finished.
	virtual PipelineFlowState handOver(DataSet* dataset, const PipelineFlowState& existing, bool isNewFile, FileSource* fileSource) override;

	/// Returns the triangle mesh data structure.
	const TriMesh& mesh() const { return _mesh; }

	/// Returns a reference to the triangle mesh data structure.
	TriMesh& mesh() { return _mesh; }

private:

	/// The triangle mesh.
	TriMesh _mesh;
};

}	// End of namespace
}	// End of namespace
