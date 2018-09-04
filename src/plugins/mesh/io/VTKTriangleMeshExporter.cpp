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
#include <plugins/mesh/surface/RenderableSurfaceMesh.h>
#include <core/dataset/scene/PipelineSceneNode.h>
#include <core/dataset/scene/SelectionSet.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/utilities/concurrent/TaskManager.h>
#include <core/utilities/concurrent/Promise.h>
#include "VTKTriangleMeshExporter.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(VTKTriangleMeshExporter);

/******************************************************************************
* Selects the nodes from the scene to be exported by this exporter if 
* no specific set of nodes was provided.
******************************************************************************/
void VTKTriangleMeshExporter::selectStandardOutputData()
{
	QVector<SceneNode*> nodes = dataset()->selection()->nodes();
	if(nodes.empty())
		throwException(tr("Please select an object to be exported first."));
	setOutputData(nodes);
}

/******************************************************************************
 * This is called once for every output file to be written and before
 * exportData() is called.
 *****************************************************************************/
bool VTKTriangleMeshExporter::openOutputFile(const QString& filePath, int numberOfFrames)
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
bool VTKTriangleMeshExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, TaskManager& taskManager)
{
	if(!FileExporter::exportFrame(frameNumber, time, filePath, taskManager))
		return false;
 
	// Export the first scene node from the selection set.
	if(outputData().empty())
		throwException(tr("The selection set to be exported is empty."));
 
	Promise<> exportTask = Promise<>::createSynchronous(&taskManager, true, true);
	exportTask.setProgressText(tr("Writing file %1").arg(filePath));
	
	PipelineSceneNode* objectNode = dynamic_object_cast<PipelineSceneNode>(outputData().front());
	if(!objectNode)
		throwException(tr("The scene node to be exported is not an object node."));

	// Evaluate pipeline of object node.
	// Note: We are requesting the rendering flow state from the node,
	// because we are interested in renderable triangle meshes.	
	auto evalFuture = objectNode->evaluateRenderingPipeline(time);
	if(!taskManager.waitForTask(evalFuture))
		return false;

	// Look up the RenderableSurfaceMesh in the pipeline state.
	const PipelineFlowState& state = evalFuture.result();
	const RenderableSurfaceMesh* meshObj = state.getObject<RenderableSurfaceMesh>();
	if(!meshObj)
		throwException(tr("The object to be exported does not contain any exportable surface mesh data."));

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
		
	return !exportTask.isCanceled();
}

}	// End of namespace
}	// End of namespace
