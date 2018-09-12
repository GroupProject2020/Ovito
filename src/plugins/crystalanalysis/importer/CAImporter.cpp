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
#include <plugins/crystalanalysis/objects/dislocations/DislocationVis.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/objects/partition_mesh/PartitionMesh.h>
#include <plugins/crystalanalysis/objects/partition_mesh/PartitionMeshVis.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include <plugins/mesh/surface/SurfaceMeshVis.h>
#include <core/app/Application.h>
#include <core/dataset/io/FileSource.h>
#include <core/utilities/io/CompressedTextReader.h>
#include "CAImporter.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(CAImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool CAImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

	// Read first line.
	stream.readLine(20);

	// Files start with the string "CA_FILE_VERSION ".
	if(stream.lineStartsWith("CA_FILE_VERSION "))
		return true;

	return false;
}

/******************************************************************************
* Scans the given input file to find all contained simulation frames.
******************************************************************************/
void CAImporter::FrameFinder::discoverFramesInFile(QFile& file, const QUrl& sourceUrl, QVector<FileSourceImporter::Frame>& frames)
{
	CompressedTextReader stream(file, sourceUrl.path());
	setProgressText(tr("Scanning CA file %1").arg(stream.filename()));
	setProgressMaximum(stream.underlyingSize());

	QFileInfo fileInfo(stream.device().fileName());
	QString filename = fileInfo.fileName();
	QDateTime lastModified = fileInfo.lastModified();
	int frameNumber = 0;
	qint64 byteOffset;

	while(!stream.eof() && !isCanceled()) {

		if(frameNumber == 0) {
			byteOffset = stream.byteOffset();
			stream.readLine();
		}
		int startLineNumber = stream.lineNumber();

		if(stream.line()[0] == '\0') break;
		if(!stream.lineStartsWith("CA_FILE_VERSION "))
			throw Exception(tr("Failed to parse file. This is not a proper file written by the Crystal Analysis Tool or OVITO."));

		// Create a new record for the frame.
		Frame frame;
		frame.sourceFile = sourceUrl;
		frame.byteOffset = byteOffset;
		frame.lineNumber = startLineNumber;
		frame.lastModificationTime = lastModified;
		frame.label = QString("%1 (Frame %2)").arg(filename).arg(frameNumber++);
		frames.push_back(frame);

		// Seek to end of frame record.
		while(!stream.eof()) {
			byteOffset = stream.byteOffset();
			stream.readLineTrimLeft();
			if(stream.lineStartsWith("CA_FILE_VERSION ")) break;
			if((stream.lineNumber() % 4096) == 0)
				setProgressValue(stream.underlyingByteOffset());
		}
	}
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr CAImporter::FrameLoader::loadFile(QFile& file)
{
	// Open file for reading.
	CompressedTextReader stream(file, frame().sourceFile.path());
	setProgressText(tr("Reading CA file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Read file header.
	stream.readLine();
	if(!stream.lineStartsWith("CA_FILE_VERSION "))
		throw Exception(tr("Failed to parse file. This is not a proper CA file written by OVITO or the Crystal Analysis Tool."));
	int fileFormatVersion = 0;
	if(sscanf(stream.line(), "CA_FILE_VERSION %i", &fileFormatVersion) != 1)
		throw Exception(tr("Failed to parse file. This is not a proper CA file written by OVITO or the Crystal Analysis Tool."));
	if(fileFormatVersion != 4 && fileFormatVersion != 5 && fileFormatVersion != 6 && fileFormatVersion != 7)
		throw Exception(tr("Failed to parse file. This CA file format version is not supported: %1").arg(fileFormatVersion));
	stream.readLine();
	if(!stream.lineStartsWith("CA_LIB_VERSION"))
		throw Exception(tr("Failed to parse file. This is not a proper CA file written by OVITO or the Crystal Analysis Tool."));

	// Create the destination container for loaded data.
	auto frameData = std::make_shared<CrystalAnalysisFrameData>();
	
	QString caFilename;
	QString atomsFilename;
	AffineTransformation cell = AffineTransformation::Zero();
	int pbcFlags[3] = {1,1,1};
	int numClusters = 0;
	int numClusterTransitions = 0;
	int numDislocationSegments = 0;

	while(!stream.eof()) {
		stream.readLineTrimLeft();

		// Read file path information.
		if(stream.lineStartsWith("OUTPUT_PATH ")) {
			caFilename = stream.lineString().mid(12).trimmed();
		}
		else if(stream.lineStartsWith("ATOMS_PATH ")) {
			atomsFilename = stream.lineString().mid(11).trimmed();
		}
		else if(stream.lineStartsWith("STRUCTURE_PATTERNS ") || stream.lineStartsWith("STRUCTURE_TYPES ")) {
			// Read pattern catalog.
			int numPatterns;
			if(sscanf(stream.line(), "%*s %i", &numPatterns) != 1 || numPatterns <= 0)
				throw Exception(tr("Failed to parse file. Invalid number of structure types in line %1.").arg(stream.lineNumber()));
			std::vector<int> patternId2Index;
			for(int index = 0; index < numPatterns; index++) {
				CrystalAnalysisFrameData::PatternInfo pattern;
				if(fileFormatVersion <= 4) {
					if(sscanf(stream.readLine(), "PATTERN ID %i", &pattern.id) != 1)
						throw Exception(tr("Failed to parse file. Invalid pattern ID in line %1.").arg(stream.lineNumber()));
				}
				else {
					if(sscanf(stream.readLine(), "STRUCTURE_TYPE %i", &pattern.id) != 1)
						throw Exception(tr("Failed to parse file. Invalid structure type ID in line %1.").arg(stream.lineNumber()));
				}
				if((int)patternId2Index.size() <= pattern.id)
					patternId2Index.resize(pattern.id+1);
				patternId2Index[pattern.id] = index;
				while(!stream.eof()) {
					stream.readLineTrimLeft();
					if(stream.lineStartsWith("NAME ")) {
						pattern.shortName = stream.lineString().mid(5).trimmed();
					}
					else if(stream.lineStartsWith("FULL_NAME ")) {
						pattern.longName = stream.lineString().mid(9).trimmed();
					}
					else if(stream.lineStartsWith("TYPE ")) {
						QString patternTypeString = stream.lineString().mid(5).trimmed();
						if(patternTypeString == QStringLiteral("LATTICE")) pattern.type = StructurePattern::Lattice;
						else if(patternTypeString == QStringLiteral("INTERFACE")) pattern.type = StructurePattern::Interface;
						else if(patternTypeString == QStringLiteral("POINTDEFECT")) pattern.type = StructurePattern::PointDefect;
						else throw Exception(tr("Failed to parse file. Invalid pattern type in line %1: %2").arg(stream.lineNumber()).arg(patternTypeString));
					}
					else if(stream.lineStartsWith("COLOR ")) {
						if(sscanf(stream.line(), "COLOR " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &pattern.color.r(), &pattern.color.g(), &pattern.color.b()) != 3)
							throw Exception(tr("Failed to parse file. Invalid pattern color in line %1.").arg(stream.lineNumber()));
					}
					else if(stream.lineStartsWith("BURGERS_VECTOR_FAMILIES ")) {
						int numFamilies;
						if(sscanf(stream.line(), "BURGERS_VECTOR_FAMILIES %i", &numFamilies) != 1 || numFamilies < 0)
							throw Exception(tr("Failed to parse file. Invalid number of Burgers vectors families in line %1.").arg(stream.lineNumber()));
						for(int familyIndex = 0; familyIndex < numFamilies; familyIndex++) {
							CrystalAnalysisFrameData::BurgersVectorFamilyInfo family;
							if(sscanf(stream.readLine(), "BURGERS_VECTOR_FAMILY ID %i", &family.id) != 1)
								throw Exception(tr("Failed to parse file. Invalid Burgers vector family ID in line %1.").arg(stream.lineNumber()));
							stream.readLine();
							family.name = stream.lineString().trimmed();
							if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &family.burgersVector.x(), &family.burgersVector.y(), &family.burgersVector.z()) != 3)
								throw Exception(tr("Failed to parse file. Invalid Burgers vector in line %1.").arg(stream.lineNumber()));
							if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &family.color.r(), &family.color.g(), &family.color.b()) != 3)
								throw Exception(tr("Failed to parse file. Invalid color in line %1.").arg(stream.lineNumber()));
							pattern.burgersVectorFamilies.push_back(std::move(family));
						}
					}
					else if(stream.lineStartsWith("END_PATTERN") || stream.lineStartsWith("END_STRUCTURE_TYPE"))
						break;
				}
				if(pattern.longName.isEmpty())
					pattern.longName = pattern.shortName;
				frameData->addPattern(std::move(pattern));
			}
		}
		else if(stream.lineStartsWith("SIMULATION_CELL_ORIGIN ")) {
			// Read simulation cell geometry.
			if(sscanf(stream.line(), "SIMULATION_CELL_ORIGIN " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &cell(0,3), &cell(1,3), &cell(2,3)) != 3)
				throw Exception(tr("Failed to parse file. Invalid cell origin in line %1.").arg(stream.lineNumber()));
		}
		else if(stream.lineStartsWith("SIMULATION_CELL ")) {
			if(sscanf(stream.line(), "SIMULATION_CELL " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING
					" " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING
					" " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
					&cell(0,0), &cell(0,1), &cell(0,2), &cell(1,0), &cell(1,1), &cell(1,2), &cell(2,0), &cell(2,1), &cell(2,2)) != 9)
				throw Exception(tr("Failed to parse file. Invalid cell vectors in line %1.").arg(stream.lineNumber()));
		}
		else if(stream.lineStartsWith("SIMULATION_CELL_MATRIX")) {
			for(size_t row = 0; row < 3; row++) {
				if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
						&cell(row,0), &cell(row,1), &cell(row,2)) != 3)
					throw Exception(tr("Failed to parse file. Invalid cell matrix in line %1.").arg(stream.lineNumber()));
			}
		}
		else if(stream.lineStartsWith("PBC_FLAGS ")) {
			if(sscanf(stream.line(), "PBC_FLAGS %i %i %i", &pbcFlags[0], &pbcFlags[1], &pbcFlags[2]) != 3)
				throw Exception(tr("Failed to parse file. Invalid PBC flags in line %1.").arg(stream.lineNumber()));
		}
		else if(stream.lineStartsWith("CLUSTERS ")) {
			// Read cluster list.
			if(sscanf(stream.line(), "CLUSTERS %i", &numClusters) != 1)
				throw Exception(tr("Failed to parse file. Invalid number of clusters in line %1.").arg(stream.lineNumber()));
			setProgressText(tr("Reading clusters"));
			setProgressMaximum(numClusters);
			for(int index = 0; index < numClusters; index++) {
				if(!setProgressValueIntermittent(index))
					return {};
				if(fileFormatVersion <= 4) {
					int patternId = 0, clusterId = 0, clusterProc = 0;
					stream.readLine();
					if(sscanf(stream.readLine(), "%i %i", &clusterId, &clusterProc) != 2)
						throw Exception(tr("Failed to parse file. Invalid cluster ID in line %1.").arg(stream.lineNumber()));
					if(sscanf(stream.readLine(), "%i", &patternId) != 1)
						throw Exception(tr("Failed to parse file. Invalid cluster pattern index in line %1.").arg(stream.lineNumber()));
					Cluster* cluster = frameData->createCluster(patternId);
					OVITO_ASSERT(cluster->structure != 0);
					if(sscanf(stream.readLine(), "%i", &cluster->atomCount) != 1)
						throw Exception(tr("Failed to parse file. Invalid cluster atom count in line %1.").arg(stream.lineNumber()));
					if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &cluster->centerOfMass.x(), &cluster->centerOfMass.y(), &cluster->centerOfMass.z()) != 3)
						throw Exception(tr("Failed to parse file. Invalid cluster center of mass in line %1.").arg(stream.lineNumber()));
					if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING
							" " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING
							" " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
							&cluster->orientation(0,0), &cluster->orientation(0,1), &cluster->orientation(0,2),
							&cluster->orientation(1,0), &cluster->orientation(1,1), &cluster->orientation(1,2),
							&cluster->orientation(2,0), &cluster->orientation(2,1), &cluster->orientation(2,2)) != 9)
						throw Exception(tr("Failed to parse file. Invalid cluster orientation matrix in line %1.").arg(stream.lineNumber()));
				}
				else {
					int patternId = 0, clusterId = 0, atomCount = 0;
					Point3 centerOfMass = Point3::Origin();
					Matrix3 orientation = Matrix3::Identity();
					Color color(1,1,1);
					while(!stream.eof()) {
						stream.readLineTrimLeft();
						if(stream.lineStartsWith("CLUSTER ")) {
							if(sscanf(stream.line(), "CLUSTER %i", &clusterId) != 1)
								throw Exception(tr("Failed to parse file. Invalid cluster ID in line %1.").arg(stream.lineNumber()));
						}
						else if(stream.lineStartsWith("CLUSTER_STRUCTURE ")) {
							if(sscanf(stream.line(), "CLUSTER_STRUCTURE %i", &patternId) != 1)
								throw Exception(tr("Failed to parse file. Invalid cluster structure type in line %1.").arg(stream.lineNumber()));
						}
						else if(stream.lineStartsWith("CLUSTER_SIZE ")) {
							if(sscanf(stream.line(), "CLUSTER_SIZE %i", &atomCount) != 1)
								throw Exception(tr("Failed to parse file. Invalid cluster size in line %1.").arg(stream.lineNumber()));
						}
						else if(stream.lineStartsWith("CLUSTER_CENTER_OF_MASS ")) {
							if(sscanf(stream.line(), "CLUSTER_CENTER_OF_MASS " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &centerOfMass.x(), &centerOfMass.y(), &centerOfMass.z()) != 3)
								throw Exception(tr("Failed to parse file. Invalid cluster center in line %1.").arg(stream.lineNumber()));
						}
						else if(stream.lineStartsWith("CLUSTER_COLOR ")) {
							if(sscanf(stream.line(), "CLUSTER_COLOR " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &color.r(), &color.g(), &color.b()) != 3)
								throw Exception(tr("Failed to parse file. Invalid cluster color in line %1.").arg(stream.lineNumber()));
						}
						else if(stream.lineStartsWith("CLUSTER_ORIENTATION")) {
							for(size_t row = 0; row < 3; row++) {
								if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
										&orientation(row,0), &orientation(row,1), &orientation(row,2)) != 3)
									throw Exception(tr("Failed to parse file. Invalid cluster orientation matrix in line %1.").arg(stream.lineNumber()));
							}
						}
						else if(stream.lineStartsWith("END_CLUSTER"))
							break;
					}
					Cluster* cluster = frameData->createCluster(patternId);
					if(cluster->id != clusterId)
						throw Exception(tr("Failed to parse file. Invalid cluster id: %1.").arg(clusterId));
					cluster->atomCount = atomCount;
					cluster->centerOfMass = centerOfMass;
					cluster->orientation = orientation;
					cluster->color = color;
				}
			}
		}
		else if(stream.lineStartsWith("CLUSTER_TRANSITIONS ")) {
			// Read cluster transition list.
			if(sscanf(stream.line(), "CLUSTER_TRANSITIONS %i", &numClusterTransitions) != 1)
				throw Exception(tr("Failed to parse file. Invalid number of cluster transitions in line %1.").arg(stream.lineNumber()));
			setProgressText(tr("Reading cluster transitions"));
			setProgressMaximum(numClusterTransitions);
			for(int index = 0; index < numClusterTransitions; index++) {
				if(!setProgressValueIntermittent(index))
					return {};
				int clusterIndex1, clusterIndex2;
				if(sscanf(stream.readLine(), "TRANSITION %i %i", &clusterIndex1, &clusterIndex2) != 2 || clusterIndex1 >= numClusters || clusterIndex2 >= numClusters) {
					throw Exception(tr("Failed to parse file. Invalid cluster transition in line %1.").arg(stream.lineNumber()));
				}
				Matrix3 tm;
				if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING
						" " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING
						" " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
						&tm(0,0), &tm(0,1), &tm(0,2),
						&tm(1,0), &tm(1,1), &tm(1,2),
						&tm(2,0), &tm(2,1), &tm(2,2)) != 9)
					throw Exception(tr("Failed to parse file. Invalid cluster transition matrix in line %1.").arg(stream.lineNumber()));
				frameData->clusterGraph()->createClusterTransition(frameData->clusterGraph()->clusters()[clusterIndex1+1], frameData->clusterGraph()->clusters()[clusterIndex2+1], tm);
			}
		}
		else if(stream.lineStartsWith("DISLOCATIONS ")) {
			// Read dislocations list.
			if(sscanf(stream.line(), "DISLOCATIONS %i", &numDislocationSegments) != 1)
				throw Exception(tr("Failed to parse file. Invalid number of dislocation segments in line %1.").arg(stream.lineNumber()));
			setProgressText(tr("Reading dislocations"));
			setProgressMaximum(numDislocationSegments);
			for(int index = 0; index < numDislocationSegments; index++) {
				if(!setProgressValueIntermittent(index))
					return {};
				int segmentId;
				if(sscanf(stream.readLine(), "%i", &segmentId) != 1)
					throw Exception(tr("Failed to parse file. Invalid segment ID in line %1.").arg(stream.lineNumber()));

				Vector3 burgersVector;
				if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &burgersVector.x(), &burgersVector.y(), &burgersVector.z()) != 3)
					throw Exception(tr("Failed to parse file. Invalid Burgers vector in line %1.").arg(stream.lineNumber()));

				Cluster* cluster = nullptr;
				if(fileFormatVersion <= 4) {
					int clusterIndex;
					if(sscanf(stream.readLine(), "%i", &clusterIndex) != 1 || clusterIndex < 0 || clusterIndex >= numClusters)
						throw Exception(tr("Failed to parse file. Invalid cluster index in line %1.").arg(stream.lineNumber()));
					cluster = frameData->clusterGraph()->clusters()[clusterIndex+1];
				}
				else {
					int clusterId;
					if(sscanf(stream.readLine(), "%i", &clusterId) != 1 || clusterId <= 0)
						throw Exception(tr("Failed to parse file. Invalid cluster ID in line %1.").arg(stream.lineNumber()));
					cluster = frameData->clusterGraph()->findCluster(clusterId);
				}
				if(!cluster)
					throw Exception(tr("Failed to parse file. Invalid cluster reference in line %1.").arg(stream.lineNumber()));

				DislocationSegment* segment = frameData->dislocations()->createSegment(ClusterVector(burgersVector, cluster));

				// Read polyline.
				int numPoints;
				if(sscanf(stream.readLine(), "%i", &numPoints) != 1 || numPoints <= 1)
					throw Exception(tr("Failed to parse file. Invalid segment number of points in line %1.").arg(stream.lineNumber()));
				segment->line.resize(numPoints);
				for(Point3& p : segment->line) {
					if(fileFormatVersion <= 4) {
						if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &p.x(), &p.y(), &p.z()) != 3)
							throw Exception(tr("Failed to parse file. Invalid point in line %1.").arg(stream.lineNumber()));
					}
					else {
						int coreSize = 0;
						if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %i", &p.x(), &p.y(), &p.z(), &coreSize) < 3)
							throw Exception(tr("Failed to parse file. Invalid point in line %1.").arg(stream.lineNumber()));
						if(coreSize > 0)
							segment->coreSize.push_back(coreSize);
					}
				}

				if(fileFormatVersion <= 4) {
					// Read dislocation core size.
					segment->coreSize.resize(numPoints);
					for(int& coreSize : segment->coreSize) {
						if(sscanf(stream.readLine(), "%i", &coreSize) != 1)
							throw Exception(tr("Failed to parse file. Invalid core size in line %1.").arg(stream.lineNumber()));
					}
				}
			}
		}
		else if(stream.lineStartsWith("DISLOCATION_JUNCTIONS")) {
			// Read dislocation junctions.
			auto dislocations = frameData->dislocations();
			for(int index = 0; index < numDislocationSegments; index++) {
				DislocationSegment* segment = dislocations->segments()[index];
				for(int nodeIndex = 0; nodeIndex < 2; nodeIndex++) {
					int isForward, otherSegmentId;
					if(sscanf(stream.readLine(), "%i %i", &isForward, &otherSegmentId) != 2 || otherSegmentId < 0 || otherSegmentId >= numDislocationSegments)
						throw Exception(tr("Failed to parse file. Invalid dislocation junction record in line %1.").arg(stream.lineNumber()));
					segment->nodes[nodeIndex]->junctionRing = dislocations->segments()[otherSegmentId]->nodes[isForward ? 0 : 1];
				}
			}
		}
		else if(stream.lineStartsWith("DEFECT_MESH_VERTICES ")) {
			// Read defect mesh vertices.
			int numDefectMeshVertices;
			if(sscanf(stream.line(), "DEFECT_MESH_VERTICES %i", &numDefectMeshVertices) != 1)
				throw Exception(tr("Failed to parse file. Invalid number of defect mesh vertices in line %1.").arg(stream.lineNumber()));
			setProgressText(tr("Reading defect surface"));
			setProgressMaximum(numDefectMeshVertices);
			auto defectSurface = frameData->defectSurface();
			defectSurface->reserveVertices(numDefectMeshVertices);
			for(int index = 0; index < numDefectMeshVertices; index++) {
				if(!setProgressValueIntermittent(index)) return {};
				Point3 p;
				if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &p.x(), &p.y(), &p.z()) != 3)
					throw Exception(tr("Failed to parse file. Invalid point in line %1.").arg(stream.lineNumber()));
				defectSurface->createVertex(p);
			}
		}
		else if(stream.lineStartsWith("DEFECT_MESH_FACETS ")) {
			// Read defect mesh facets.
			int numDefectMeshFacets;
			if(sscanf(stream.line(), "DEFECT_MESH_FACETS %i", &numDefectMeshFacets) != 1)
				throw Exception(tr("Failed to parse file. Invalid number of defect mesh facets in line %1.").arg(stream.lineNumber()));
			setProgressMaximum(numDefectMeshFacets * 2);
			auto defectSurface = frameData->defectSurface();
			defectSurface->reserveFaces(numDefectMeshFacets);
			for(int index = 0; index < numDefectMeshFacets; index++) {
				if(!setProgressValueIntermittent(index))
					return {};
				int v[3];
				if(sscanf(stream.readLine(), "%i %i %i", &v[0], &v[1], &v[2]) != 3)
					throw Exception(tr("Failed to parse file. Invalid triangle facet in line %1.").arg(stream.lineNumber()));
				defectSurface->createFace({ defectSurface->vertex(v[0]), defectSurface->vertex(v[1]), defectSurface->vertex(v[2]) });
			}

			// Read facet adjacency information.
			for(int index = 0; index < numDefectMeshFacets; index++) {
				if(!setProgressValueIntermittent(index + numDefectMeshFacets))
					return {};
				int v[3];
				if(sscanf(stream.readLine(), "%i %i %i", &v[0], &v[1], &v[2]) != 3)
					throw Exception(tr("Failed to parse file. Invalid triangle adjacency info in line %1.").arg(stream.lineNumber()));
				HalfEdgeMesh<>::Edge* edge = defectSurface->face(index)->edges();
				for(int i = 0; i < 3; i++, edge = edge->nextFaceEdge()) {
					OVITO_CHECK_POINTER(edge);
					if(edge->oppositeEdge() != nullptr) continue;
					HalfEdgeMesh<>::Face* oppositeFace = defectSurface->face(v[i]);
					HalfEdgeMesh<>::Edge* oppositeEdge = oppositeFace->findEdge(edge->vertex2(), edge->vertex1());
					OVITO_ASSERT(oppositeEdge != nullptr);
					edge->linkToOppositeEdge(oppositeEdge);
				}
			}
		}
		else if(stream.lineStartsWith("PARTITION_MESH_VERTICES ")) {
			// Read partition mesh vertices.
			int numVertices;
			if(sscanf(stream.line(), "PARTITION_MESH_VERTICES %i", &numVertices) != 1)
				throw Exception(tr("Failed to parse file. Invalid number of mesh vertices in line %1.").arg(stream.lineNumber()));
			setProgressText(tr("Reading partition mesh"));
			setProgressMaximum(numVertices);
			auto partitionMesh = frameData->partitionMesh();
			partitionMesh->reserveVertices(numVertices);
			for(int index = 0; index < numVertices; index++) {
				if(!setProgressValueIntermittent(index)) return {};
				Point3 p;
				if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &p.x(), &p.y(), &p.z()) != 3)
					throw Exception(tr("Failed to parse file. Invalid point in line %1.").arg(stream.lineNumber()));
				partitionMesh->createVertex(p);
			}
		}
		else if(stream.lineStartsWith("PARTITION_MESH_FACETS ")) {
			// Read partition mesh facets.
			int numFacets;
			if(sscanf(stream.line(), "PARTITION_MESH_FACETS %i", &numFacets) != 1)
				throw Exception(tr("Failed to parse file. Invalid number of mesh facets in line %1.").arg(stream.lineNumber()));
			setProgressMaximum(numFacets * 2);
			auto partitionMesh = frameData->partitionMesh();
			partitionMesh->reserveFaces(numFacets);
			for(int index = 0; index < numFacets; index++) {
				if(!setProgressValueIntermittent(index))
					return {};
				int v[3];
				int region;
				if(sscanf(stream.readLine(), "%i %i %i %i", &region, &v[0], &v[1], &v[2]) != 4)
					throw Exception(tr("Failed to parse file. Invalid triangle facet in line %1.").arg(stream.lineNumber()));
				PartitionMeshData::Face* face = partitionMesh->createFace({ partitionMesh->vertex(v[0]), partitionMesh->vertex(v[1]), partitionMesh->vertex(v[2]) });
				face->region = region;
			}

			// Read facet adjacency information.
			for(int index = 0; index < numFacets; index++) {
				if(!setProgressValueIntermittent(index + numFacets))
					return {};
				int v[3];
				int mfe[3][2];
				int oppositeFaceIndex;
				if(sscanf(stream.readLine(), "%i %i %i %i %i %i %i %i %i %i", &oppositeFaceIndex,
						&v[0], &mfe[0][0], &mfe[0][1],
						&v[1], &mfe[1][0], &mfe[1][1],
						&v[2], &mfe[2][0], &mfe[2][1]) != 10)
					throw Exception(tr("Failed to parse file. Invalid triangle adjacency info in line %1.").arg(stream.lineNumber()));
				PartitionMeshData::Face* oppositeFace = partitionMesh->face(oppositeFaceIndex);
				partitionMesh->face(index)->oppositeFace = oppositeFace;
				PartitionMeshData::Edge* edge = partitionMesh->face(index)->edges();
				for(int i = 0; i < 3; i++, edge = edge->nextFaceEdge()) {
					OVITO_CHECK_POINTER(edge);
					PartitionMeshData::Edge* manifoldEdge = oppositeFace->findEdge(partitionMesh->vertex(mfe[i][0]), partitionMesh->vertex(mfe[i][1]));
					OVITO_ASSERT(manifoldEdge != nullptr);
					edge->nextManifoldEdge = manifoldEdge;
					if(edge->oppositeEdge() != nullptr) continue;
					PartitionMeshData::Face* adjacentFace = partitionMesh->face(v[i]);
					PartitionMeshData::Edge* oppositeEdge = adjacentFace->findEdge(edge->vertex2(), edge->vertex1());
					OVITO_ASSERT(oppositeEdge != nullptr);
					edge->linkToOppositeEdge(oppositeEdge);
				}
			}
		}
		else if(stream.lineStartsWith("SLIP_SURFACE_VERTICES ")) {
			// Read slip surface vertices.
			int numVertices;
			if(sscanf(stream.line(), "SLIP_SURFACE_VERTICES %i", &numVertices) != 1)
				throw Exception(tr("Failed to parse file. Invalid number of mesh vertices in line %1.").arg(stream.lineNumber()));
			setProgressText(tr("Reading slip surfaces"));
			setProgressMaximum(numVertices);
			for(int index = 0; index < numVertices; index++) {
				if(!setProgressValueIntermittent(index)) return {};
				stream.readLine();
			}
		}
		else if(stream.lineStartsWith("SLIP_SURFACE_FACETS ")) {
			// Read slip surface facets.
			int numFacets;
			if(sscanf(stream.line(), "SLIP_SURFACE_FACETS %i", &numFacets) != 1)
				throw Exception(tr("Failed to parse file. Invalid number of mesh facets in line %1.").arg(stream.lineNumber()));
			setProgressMaximum(numFacets);
			for(int index = 0; index < numFacets; index++) {
				if(!setProgressValueIntermittent(index)) return {};
				stream.readLine();
			}
		}
		else if(stream.lineStartsWith("STACKING_FAULT_VERTICES ")) {
			// Read slip surface vertices.
			int numVertices;
			if(sscanf(stream.line(), "STACKING_FAULT_VERTICES %i", &numVertices) != 1)
				throw Exception(tr("Failed to parse file. Invalid number of mesh vertices in line %1.").arg(stream.lineNumber()));
			setProgressText(tr("Reading stacking faults"));
			setProgressMaximum(numVertices);
			for(int index = 0; index < numVertices; index++) {
				if(!setProgressValueIntermittent(index)) return {};
				stream.readLine();
			}
		}
		else if(stream.lineStartsWith("STACKING_FAULT_FACETS ")) {
			// Read slip surface facets.
			int numFacets;
			if(sscanf(stream.line(), "STACKING_FAULT_FACETS %i", &numFacets) != 1)
				throw Exception(tr("Failed to parse file. Invalid number of mesh facets in line %1.").arg(stream.lineNumber()));
			setProgressMaximum(numFacets);
			for(int index = 0; index < numFacets; index++) {
				if(!setProgressValueIntermittent(index)) return {};
				stream.readLine();
			}
		}
		else if(stream.lineStartsWith("METADATA ")) {
			// Ignore. This is for future use.
		}
		else if(stream.lineStartsWith("CA_FILE_VERSION ")) {
			// Beginning of next frame.
			frameData->signalAdditionalFrames();
			break;
		}
		else if(stream.line()[0] != '\0') {
			throw Exception(tr("Failed to parse file. Invalid keyword in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
		}
	}

	frameData->simulationCell().setMatrix(cell);
	frameData->simulationCell().setPbcFlags(pbcFlags[0], pbcFlags[1], pbcFlags[2]);

	frameData->setStatus(tr("Number of dislocations: %1").arg(numDislocationSegments));	
	return frameData;
}

/******************************************************************************
* Inserts the data loaded by perform() into the provided pipeline state.
* This function is called by the system from the main thread after the
* asynchronous loading task has finished.
******************************************************************************/
OORef<DataCollection> CAImporter::CrystalAnalysisFrameData::handOver(const DataCollection* existing, bool isNewFile, FileSource* fileSource)
{
	// Insert simulation cell.
	OORef<DataCollection> output = ParticleFrameData::handOver(existing, isNewFile, fileSource);

	// Insert defect surface.
	if(_defectSurface) {
		SurfaceMesh* defectSurfaceObj = const_cast<SurfaceMesh*>(existing ? existing->getObject<SurfaceMesh>() : nullptr);
		if(!defectSurfaceObj) {
			defectSurfaceObj = output->createObject<SurfaceMesh>(fileSource);
			OORef<SurfaceMeshVis> vis = new SurfaceMeshVis(fileSource->dataset());
			if(!Application::instance()->scriptMode())
				vis->loadUserDefaults();
			defectSurfaceObj->setVisElement(vis);
		}
		else {
			output->addObject(defectSurfaceObj);
		}
		defectSurfaceObj->setDomain(output->getObject<SimulationCellObject>());
		defectSurfaceObj->setStorage(defectSurface());
	}

	// Insert partition mesh.
	if(_partitionMesh) {
		PartitionMesh* partitionMeshObj = const_cast<PartitionMesh*>(existing ? existing->getObject<PartitionMesh>() : nullptr);
		if(!partitionMeshObj) {
			partitionMeshObj = output->createObject<PartitionMesh>(fileSource);
			OORef<PartitionMeshVis> vis = new PartitionMeshVis(fileSource->dataset());
			if(!Application::instance()->scriptMode())
				vis->loadUserDefaults();
			partitionMeshObj->setVisElement(vis);
		}
		else {
			output->addObject(partitionMeshObj);
		}
		partitionMeshObj->setDomain(output->getObject<SimulationCellObject>());
		partitionMeshObj->setStorage(partitionMesh());
	}

	// Insert pattern catalog.
	PatternCatalog* patternCatalog = const_cast<PatternCatalog*>(existing ? existing->getObject<PatternCatalog>() : nullptr);
	if(!patternCatalog) {
		patternCatalog = output->createObject<PatternCatalog>(fileSource);
	}
	else {
		output->addObject(patternCatalog);
	}

	// Update pattern catalog.
	for(int i = 0; i < _patterns.size(); i++) {
		OORef<StructurePattern> pattern;
		if(patternCatalog->patterns().size() > i+1) {
			pattern = patternCatalog->patterns()[i+1];
		}
		else {
			pattern.reset(new StructurePattern(patternCatalog->dataset()));
			patternCatalog->addPattern(pattern);
		}
		if(pattern->shortName() != _patterns[i].shortName)
			pattern->setColor(_patterns[i].color);
		pattern->setShortName(_patterns[i].shortName);
		pattern->setLongName(_patterns[i].longName);
		pattern->setStructureType(_patterns[i].type);
		pattern->setNumericId(_patterns[i].id);
		pattern->setSymmetryType(_patterns[i].symmetryType);

		// Update Burgers vector families.
		for(int j = 0; j < _patterns[i].burgersVectorFamilies.size(); j++) {
			OORef<BurgersVectorFamily> family;
			if(pattern->burgersVectorFamilies().size() > j+1) {
				family = pattern->burgersVectorFamilies()[j+1];
			}
			else {
				family.reset(new BurgersVectorFamily(pattern->dataset()));
				pattern->addBurgersVectorFamily(family);
			}
			if(family->name() != _patterns[i].burgersVectorFamilies[j].name)
				family->setColor(_patterns[i].burgersVectorFamilies[j].color);
			family->setName(_patterns[i].burgersVectorFamilies[j].name);
			family->setBurgersVector(_patterns[i].burgersVectorFamilies[j].burgersVector);
		}
		// Remove excess families.
		for(int j = pattern->burgersVectorFamilies().size() - 1; j > _patterns[i].burgersVectorFamilies.size(); j--)
			pattern->removeBurgersVectorFamily(j);
	}
	// Remove excess patterns from the catalog.
	for(int i = patternCatalog->patterns().size() - 1; i > _patterns.size(); i--)
		patternCatalog->removePattern(i);

	// Insert cluster graph.
	if(_clusterGraph) {
		ClusterGraphObject* clusterGraphObj = const_cast<ClusterGraphObject*>(existing ? existing->getObject<ClusterGraphObject>() : nullptr);
		if(!clusterGraphObj) {
			clusterGraphObj = output->createObject<ClusterGraphObject>(fileSource);
		}
		else {
			output->addObject(clusterGraphObj);
		}
		clusterGraphObj->setStorage(clusterGraph());
	}

	// Insert dislocations.
	if(_dislocations) {
		DislocationNetworkObject* dislocationNetwork = const_cast<DislocationNetworkObject*>(existing ? existing->getObject<DislocationNetworkObject>() : nullptr);
		if(!dislocationNetwork) {
			dislocationNetwork = output->createObject<DislocationNetworkObject>(fileSource);
			OORef<DislocationVis> vis = new DislocationVis(fileSource->dataset());
			if(!Application::instance()->scriptMode())
				vis->loadUserDefaults();
			dislocationNetwork->setVisElement(vis);
		}
		else {
			output->addObject(dislocationNetwork);
		}
		dislocationNetwork->setDomain(output->getObject<SimulationCellObject>());
		dislocationNetwork->setStorage(dislocations());
	}

	return output;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
