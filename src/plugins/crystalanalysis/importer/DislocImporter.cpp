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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationDisplay.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/objects/slip_surface/SlipSurface.h>
#include <plugins/crystalanalysis/objects/slip_surface/SlipSurfaceDisplay.h>
#include <plugins/crystalanalysis/data/NodalDislocationNetwork.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include <plugins/mesh/surface/SurfaceMeshDisplay.h>
#include <core/utilities/io/CompressedTextReader.h>
#include "DislocImporter.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool DislocImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

	// Read first line.
	stream.readLine(20);

	// Files start with the marker string "# disloc file format".
	if(stream.lineStartsWith("# disloc file format"))
		return true;

	return false;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr DislocImporter::FrameLoader::loadFile(QFile& file)
{
	// Open file for reading.
	CompressedTextReader stream(file, frame().sourceFile.path());
	setProgressText(tr("Reading disloc file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));
	qint64 progressUpdateInterval = std::max(stream.underlyingSize() / 1000, (qint64)1);
	setProgressMaximum(stream.underlyingSize() / progressUpdateInterval);

	// Read file header.
	stream.readLine();
	if(!stream.lineStartsWith("# disloc file format"))
		throw Exception(tr("File parsing error. This is not a proper disloc file."));

	// Create the destination container for loaded data.
	auto frameData = std::make_shared<DislocFrameData>();
	
	// Meta information.
	Vector3I processorGrid = Vector3I::Zero();
	int numProcessorPiecesLoaded = 0;
	int latticeStructure = 0;
	std::vector<Vector3> latticeVectors;
	std::vector<Vector3> transformedLatticeVectors;

	// Storage structures.
	std::map<std::array<unsigned int,4>, NodalDislocationNetwork::Node*> nodeMap;
	Cluster* defaultCluster = frameData->clusterGraph()->createCluster(1);
	NodalDislocationNetwork nodalNetwork(frameData->clusterGraph());

	auto createNodeFromCode = [&nodeMap,&nodalNetwork](std::array<unsigned int,4>&& code) -> NodalDislocationNetwork::Node* {
		std::sort(std::begin(code), std::end(code));
		auto iter = nodeMap.find(code);
		if(iter != nodeMap.end())
			return iter->second;
		else
			return nodeMap.emplace(code, nodalNetwork.createNode()).first->second;
	};

	while(!stream.eof()) {
		stream.readLineTrimLeft();
		if(!setProgressValueIntermittent(stream.underlyingByteOffset() / progressUpdateInterval))
			return {};

		if(stream.lineStartsWith("simulation cell:")) {
			AffineTransformation cell;
			std::array<bool,3> pbcFlags;
			for(size_t dim = 0; dim < 3; dim++) {
				char bcString[3];
				if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %2s", &cell(0,dim), &cell(1,dim), &cell(2,dim), bcString) != 4)
					throw Exception(tr("Failed to parse file. Invalid cell vector in line %1.").arg(stream.lineNumber()));
				pbcFlags[dim] = (strcmp(bcString, "pp") == 0);
			}
			if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &cell(0,3), &cell(1,3), &cell(2,3)) != 3)
				throw Exception(tr("Failed to parse file. Invalid cell origin in line %1.").arg(stream.lineNumber()));
			frameData->simulationCell().setMatrix(cell);
			frameData->simulationCell().setPbcFlags(pbcFlags[0], pbcFlags[1], pbcFlags[2]);
		}
		else if(stream.lineStartsWith("timestep number:")) {
			int timestep;
			if(sscanf(stream.readLine(), "%i", &timestep) != 1)
				throw Exception(tr("Failed to parse file. Invalid timestep number (line %1):\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			frameData->attributes().insert(QStringLiteral("Timestep"), QVariant::fromValue(timestep));
		}
		else if(stream.lineStartsWith("processor grid:")) {
			Vector3I pg;
			if(sscanf(stream.readLine(), "%i %i %i", &pg.x(), &pg.y(), &pg.z()) != 3)
				throw Exception(tr("Failed to parse file. Invalid processor grid specification (line %1):\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			if(pg != processorGrid && processorGrid != Vector3I::Zero())
				throw Exception(tr("Failed to parse file. Inconsistent processor grid specification in line %1:\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			processorGrid = pg;
			numProcessorPiecesLoaded++;
		}
		else if(stream.lineStartsWith("lattice structure:")) {
			stream.readLineTrimLeft();
			if(stream.lineStartsWith("bcc")) latticeStructure = StructureAnalysis::LATTICE_BCC;
			else if(stream.lineStartsWith("fcc")) latticeStructure = StructureAnalysis::LATTICE_FCC;
			else if(stream.lineStartsWith("fcc_perfect")) latticeStructure = StructureAnalysis::LATTICE_FCC;
			else throw Exception(tr("Failed to parse file. Unknown lattice structure type in line %1:\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
		}
		else if(stream.lineStartsWith("lattice vectors:")) {
			int nvectors;
			if(sscanf(stream.readLine(), "%i", &nvectors) != 1 || nvectors <= 1)
				throw Exception(tr("Failed to parse file. Invalid number of lattice vectors (line %1):\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			if(latticeVectors.empty()) {
				latticeVectors.resize(nvectors);
				transformedLatticeVectors.resize(nvectors);
			}
			else if(latticeVectors.size() != nvectors) {
				throw Exception(tr("Failed to parse file. Inconsistent number of lattice vectors (line %1):\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			}
			for(int i = 0; i < nvectors; i++) {
				Vector3& lv = latticeVectors[i];
				Vector3& sv = transformedLatticeVectors[i];
				if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " -> " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, 
						&lv.x(), &lv.y(), &lv.z(), &sv.x(), &sv.y(), &sv.z()) != 6)
					throw Exception(tr("Failed to parse file. Invalid lattice vector specification in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			}
		}
		else if(stream.lineStartsWith("line segments:")) {
			for(;;) {
				stream.readLine();
				if(stream.lineStartsWith("end of line segments:"))
					break;
				if(!setProgressValueIntermittent(stream.underlyingByteOffset() / progressUpdateInterval))
					return {};

				std::array<unsigned int,5> vertexCodes;
				int burgersVectorCode;

				if(sscanf(stream.line(), "%x %x %x %x %x %i", &vertexCodes[0], &vertexCodes[1], &vertexCodes[2], &vertexCodes[3], &vertexCodes[4], &burgersVectorCode) != 6)
					throw Exception(tr("Failed to parse file. Invalid line segment specification in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
				OVITO_ASSERT(vertexCodes[0] != vertexCodes[1]);
				OVITO_ASSERT(vertexCodes[1] != vertexCodes[2]);
				OVITO_ASSERT(vertexCodes[2] != vertexCodes[3]);
				Vector3 burgersVector;
				if(burgersVectorCode == -1) {
					if(sscanf(stream.line(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &burgersVector.x(), &burgersVector.y(), &burgersVector.z()) != 3)
						throw Exception(tr("Failed to parse file. Invalid Burgers vector specification in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
				}
				else {
					if(burgersVectorCode < 0 || burgersVectorCode > latticeVectors.size())
						throw Exception(tr("Failed to parse file. Invalid Burgers vector code in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
					burgersVector = latticeVectors[burgersVectorCode];
				}
				NodalDislocationNetwork::Node* node1 = createNodeFromCode({vertexCodes[0], vertexCodes[1], vertexCodes[2], vertexCodes[3]});
				NodalDislocationNetwork::Node* node2 = createNodeFromCode({vertexCodes[0], vertexCodes[1], vertexCodes[2], vertexCodes[4]});
				if(node1 < node2)
					nodalNetwork.createSegmentPair(node1, node2, ClusterVector(burgersVector, defaultCluster));
			}
		}	
		else if(stream.lineStartsWith("nodes:")) {
			int nnodes;
			if(sscanf(stream.readLine(), "%i", &nnodes) != 1)
				throw Exception(tr("Failed to parse file. Invalid number of nodes (line %1):\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			for(int i = 0; i < nnodes; i++) {
				if(!setProgressValueIntermittent(stream.underlyingByteOffset() / progressUpdateInterval))
					return {};
				
				std::array<unsigned int, 4> vertexCodes;
				Point3 position;
				if(sscanf(stream.readLine(), "%x %x %x %x " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, 
						&vertexCodes[0], &vertexCodes[1], &vertexCodes[2], &vertexCodes[3], &position.x(), &position.y(), &position.z()) != 7)
					throw Exception(tr("Failed to parse file. Invalid node specification in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
				OVITO_ASSERT(std::is_sorted(std::begin(vertexCodes), std::end(vertexCodes)));

				auto node = nodeMap.find(vertexCodes);
				if(node != nodeMap.end())
					node->second->pos = position;
			}
		}
	}

	// Consistency check.
	if(numProcessorPiecesLoaded != processorGrid.x() * processorGrid.y() * processorGrid.z())
		throw Exception(tr("Failed to parse file. Number of read data pieces %i is not consistent with processor grid size %i x %i x %i.")
			.arg(numProcessorPiecesLoaded).arg(processorGrid.x()).arg(processorGrid.y()).arg(processorGrid.z()));

	frameData->setStatus(tr("Number of nodes: %1\nNumber of segments: %2").arg(nodalNetwork.nodes().size()).arg(nodalNetwork.segments().size()));
	return frameData;
}

/******************************************************************************
* Inserts the data loaded by perform() into the provided pipeline state.
* This function is called by the system from the main thread after the
* asynchronous loading task has finished.
******************************************************************************/
PipelineFlowState DislocImporter::DislocFrameData::handOver(DataSet* dataset, const PipelineFlowState& existing, bool isNewFile)
{
	// Insert simulation cell.
	PipelineFlowState output = ParticleFrameData::handOver(dataset, existing, isNewFile);

	return output;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
