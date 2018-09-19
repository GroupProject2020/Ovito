///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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
#include <plugins/mesh/surface/RenderableSurfaceMesh.h>
#include "VTKTriangleMeshExporter.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(VTKTriangleMeshExporter);

/******************************************************************************
 * This is called once for every output file to be written and before
 * exportData() is called.
 *****************************************************************************/
bool VTKTriangleMeshExporter::openOutputFile(const QString& filePath, int numberOfFrames, AsyncOperation& operation)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	OVITO_ASSERT(!_outputStream);

	_outputFile.setFileName(filePath);
	_outputStream.reset(new CompressedTextWriter(_outputFile, dataset()));

	return true;
}

/******************************************************************************
 * This is called once for every output file written after exportData()
 * has been called.
 *****************************************************************************/
void VTKTriangleMeshExporter::closeOutputFile(bool exportCompleted)
{
	_outputStream.reset();
	if(_outputFile.isOpen())
		_outputFile.close();

	if(!exportCompleted)
		_outputFile.remove();
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool VTKTriangleMeshExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation)
{
	// Evaluate pipeline.
	// Note: We are requesting the rendering state from the pipeline,
	// because we are interested in renderable triangle meshes.
	const PipelineFlowState& state = getPipelineDataToBeExported(time, operation, true);

	// Look up the RenderableSurfaceMesh to be exported in the pipeline state.
	DataObjectReference objectRef(&RenderableSurfaceMesh::OOClass(), dataObjectToExport().dataPath());
	const RenderableSurfaceMesh* meshObj = static_object_cast<RenderableSurfaceMesh>(state.getLeafObject(objectRef));
	if(!meshObj) {
		throwException(tr("The pipeline output does not contain the surface mesh to be exported (animation frame: %1; object key: %2). Available surface mesh keys: (%3)")
			.arg(frameNumber).arg(objectRef.dataPath()).arg(getAvailableDataObjectList(state, RenderableSurfaceMesh::OOClass())));
	}

	operation.setProgressText(tr("Writing file %1").arg(filePath));	

	const TriMesh& surfaceMesh = meshObj->surfaceMesh();
	const TriMesh& capPolygonsMesh = meshObj->capPolygonsMesh();
	textStream() << "# vtk DataFile Version 3.0\n";
	textStream() << "# Triangle surface mesh written by " << QCoreApplication::applicationName() << " " << QCoreApplication::applicationVersion() << "\n";
	textStream() << "ASCII\n";
	textStream() << "DATASET UNSTRUCTURED_GRID\n";
	textStream() << "POINTS " << (surfaceMesh.vertexCount() + capPolygonsMesh.vertexCount()) << " double\n";
	for(const Point3& p : surfaceMesh.vertices())
		textStream() << p.x() << " " << p.y() << " " << p.z() << "\n";
	for(const Point3& p : capPolygonsMesh.vertices())
		textStream() << p.x() << " " << p.y() << " " << p.z() << "\n";
	auto totalFaceCount = surfaceMesh.faceCount() + capPolygonsMesh.faceCount();
	textStream() << "\nCELLS " << totalFaceCount << " " << (totalFaceCount * 4) << "\n";
	for(const TriMeshFace& f : surfaceMesh.faces()) {
		textStream() << "3";
		for(size_t i = 0; i < 3; i++)
			textStream() << " " << f.vertex(i);
		textStream() << "\n";
	}
	for(const TriMeshFace& f : capPolygonsMesh.faces()) {
		textStream() << "3";
		for(size_t i = 0; i < 3; i++)
			textStream() << " " << (f.vertex(i) + surfaceMesh.vertexCount());
		textStream() << "\n";
	}
	textStream() << "\nCELL_TYPES " << totalFaceCount << "\n";
	for(size_t i = 0; i < totalFaceCount; i++)
		textStream() << "5\n";	// Triangle

	textStream() << "\nCELL_DATA " << totalFaceCount << "\n";
	textStream() << "SCALARS cap unsigned_char\n";
	textStream() << "LOOKUP_TABLE default\n";
	for(size_t i = 0; i < surfaceMesh.faceCount(); i++)
		textStream() << "0\n";
	for(size_t i = 0; i < capPolygonsMesh.faceCount(); i++)
		textStream() << "1\n";

	if(!meshObj->materialColors().empty()) {
		textStream() << "\nSCALARS material_index int\n";
		textStream() << "LOOKUP_TABLE default\n";
		for(size_t i = 0; i < surfaceMesh.faceCount(); i++)
			textStream() << surfaceMesh.face(i).materialIndex() << "\n";
		for(size_t i = 0; i < capPolygonsMesh.faceCount(); i++)
			textStream() << "0\n";

		textStream() << "\nCOLOR_SCALARS color 3\n";
		for(size_t i = 0; i < surfaceMesh.faceCount(); i++) {
			const ColorA& c = meshObj->materialColors()[surfaceMesh.face(i).materialIndex() % meshObj->materialColors().size()];
			textStream() << c.r() << " " << c.g() << " " << c.b() << "\n";
		}
		for(size_t i = 0; i < capPolygonsMesh.faceCount(); i++)
			textStream() << "1 1 1\n";			
	}

	textStream() << "\nPOINT_DATA " << (surfaceMesh.vertexCount() + capPolygonsMesh.vertexCount()) << "\n";
	textStream() << "SCALARS cap unsigned_char\n";
	textStream() << "LOOKUP_TABLE default\n";
	for(size_t i = 0; i < surfaceMesh.vertexCount(); i++)
		textStream() << "0\n";
	for(size_t i = 0; i < capPolygonsMesh.vertexCount(); i++)
		textStream() << "1\n";
		
	return !operation.isCanceled();
}

}	// End of namespace
}	// End of namespace
