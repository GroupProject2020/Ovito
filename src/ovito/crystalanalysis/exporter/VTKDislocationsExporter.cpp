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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/RenderableDislocationLines.h>
#include <ovito/crystalanalysis/objects/DislocationNetworkObject.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include "VTKDislocationsExporter.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(VTKDislocationsExporter);

/******************************************************************************
 * This is called once for every output file to be written and before
 * exportFrame() is called.
 *****************************************************************************/
bool VTKDislocationsExporter::openOutputFile(const QString& filePath, int numberOfFrames, AsyncOperation& operation)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	OVITO_ASSERT(!_outputStream);

	_outputFile.setFileName(filePath);
	_outputStream.reset(new CompressedTextWriter(_outputFile, dataset()));

	return true;
}

/******************************************************************************
 * This is called once for every output file written after exportFrame()
 * has been called.
 *****************************************************************************/
void VTKDislocationsExporter::closeOutputFile(bool exportCompleted)
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
bool VTKDislocationsExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation)
{
	// Evaluate data pipeline.
	// Note: We are requesting the renderable flow state from the pipeline,
	// because we are interested in clipped (post-processed) dislocation lines.
	const PipelineFlowState& state = getPipelineDataToBeExported(time, operation, true);
	if(operation.isCanceled())
		return false;

	// Look up the RenderableDislocationLines object in the pipeline state.
	const RenderableDislocationLines* renderableLines = state.getObject<RenderableDislocationLines>();
	if(!renderableLines)
		throwException(tr("The object to be exported does not contain any exportable dislocation line data."));

	// Get the original dislocation lines.
	const DislocationNetworkObject* dislocationsObj = dynamic_object_cast<DislocationNetworkObject>(renderableLines->sourceDataObject().get());
	if(!dislocationsObj)
		throwException(tr("The object to be exported does not contain any exportable dislocation line data."));

	operation.setProgressText(tr("Writing file %1").arg(filePath));

	// Count disloction polylines and output vertices.
	std::vector<size_t> polyVertexCounts;
	for(size_t i = 0; i < renderableLines->lineSegments().size(); i++) {
		if(renderableLines->lineSegments()[i].dislocationIndex >= dislocationsObj->segments().size())
			throwException(tr("Inconsistent data: Dislocation index out of range."));
		if(i != 0) {
			const auto& s1 = renderableLines->lineSegments()[i-1];
			const auto& s2 = renderableLines->lineSegments()[i];
			if(s1.verts[1] != s2.verts[0])
				polyVertexCounts.push_back(2);
			else
				polyVertexCounts.back()++;
		}
		else polyVertexCounts.push_back(2);
	}
	size_t vertexCount = std::accumulate(polyVertexCounts.begin(), polyVertexCounts.end(), (size_t)0);

	textStream() << "# vtk DataFile Version 3.0\n";
	textStream() << "# Dislocation lines written by " << QCoreApplication::applicationName() << " " << QCoreApplication::applicationVersion() << "\n";
	textStream() << "ASCII\n";
	textStream() << "DATASET UNSTRUCTURED_GRID\n";
	textStream() << "POINTS " << vertexCount << " double\n";
	for(size_t i = 0; i < renderableLines->lineSegments().size(); i++) {
		const auto& s2 = renderableLines->lineSegments()[i];
		if(i != 0) {
			const auto& s1 = renderableLines->lineSegments()[i-1];
			if(s1.verts[1] != s2.verts[0])
				textStream() << s2.verts[0].x() << " " << s2.verts[0].y() << " " << s2.verts[0].z() << "\n";
		}
		else {
			textStream() << s2.verts[0].x() << " " << s2.verts[0].y() << " " << s2.verts[0].z() << "\n";
		}
		textStream() << s2.verts[1].x() << " " << s2.verts[1].y() << " " << s2.verts[1].z() << "\n";
	}

	textStream() << "\nCELLS " << polyVertexCounts.size() << " " << (polyVertexCounts.size() + vertexCount) << "\n";
	size_t index = 0;
	for(auto c : polyVertexCounts) {
		textStream() << c;
		for(size_t i = 0; i < c; i++) {
			textStream() << " " << (index++);
		}
		textStream() << "\n";
	}

	textStream() << "\nCELL_TYPES " << polyVertexCounts.size() << "\n";
	for(size_t i = 0; i < polyVertexCounts.size(); i++)
		textStream() << "4\n";	// VTK Polyline

	textStream() << "\nCELL_DATA " << polyVertexCounts.size() << "\n";
	textStream() << "SCALARS dislocation_index int\n";
	textStream() << "LOOKUP_TABLE default\n";
	auto segment = renderableLines->lineSegments().begin();
	for(auto c : polyVertexCounts) {
		textStream() << segment->dislocationIndex << "\n";
		segment += c - 1;
	}
	OVITO_ASSERT(segment == renderableLines->lineSegments().end());

	textStream() << "\nVECTORS burgers_vector_local double\n";
	segment = renderableLines->lineSegments().begin();
	for(auto c : polyVertexCounts) {
		const DislocationSegment* dislocation = dislocationsObj->segments()[segment->dislocationIndex];
		textStream() << dislocation->burgersVector.localVec().x() << " " << dislocation->burgersVector.localVec().y() << " " << dislocation->burgersVector.localVec().z() << "\n";
		segment += c - 1;
	}
	OVITO_ASSERT(segment == renderableLines->lineSegments().end());

	textStream() << "\nVECTORS burgers_vector_world double\n";
	segment = renderableLines->lineSegments().begin();
	for(auto c : polyVertexCounts) {
		const DislocationSegment* dislocation = dislocationsObj->segments()[segment->dislocationIndex];
		Vector3 transformedVector = dislocation->burgersVector.toSpatialVector();
		textStream() << transformedVector.x() << " " << transformedVector.y() << " " << transformedVector.z() << "\n";
		segment += c - 1;
	}
	OVITO_ASSERT(segment == renderableLines->lineSegments().end());

	return !operation.isCanceled();
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
