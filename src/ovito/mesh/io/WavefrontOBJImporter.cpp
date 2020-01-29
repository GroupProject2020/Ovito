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

#include <ovito/mesh/Mesh.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include <ovito/mesh/tri/TriMeshObject.h>
#include "WavefrontOBJImporter.h"
#include "TriMeshFrameData.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(WavefrontOBJImporter);

/******************************************************************************
* Returns whether this importer class supports importing data of the given type.
******************************************************************************/
bool WavefrontOBJImporter::OOMetaClass::supportsDataType(const DataObject::OOMetaClass& dataObjectType) const
{
	return TriMeshObject::OOClass().isDerivedFrom(dataObjectType);
}

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool WavefrontOBJImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	CompressedTextReader stream(file);

	// Read a couple of lines.
	int nverts = 0;
	for(size_t lineNumber = 0; lineNumber < 18 && !stream.eof() && nverts < 3; lineNumber++) {
		const char* line = stream.readLineTrimLeft(512);
		if(line[0] == '\0')  // Skip empty lines.
			continue;
		if(stream.lineStartsWith("#", true)) // Skip comment lines.
			continue;

		// Accept only lines starting with one of the following tokens.
		if(!stream.lineStartsWithToken("v", true) &&
			!stream.lineStartsWithToken("vn", true) &&
			!stream.lineStartsWithToken("vt", true) &&
			!stream.lineStartsWithToken("vp", true) &&
			!stream.lineStartsWithToken("l", true) &&
			!stream.lineStartsWithToken("f", true) &&
			!stream.lineStartsWithToken("s", true) &&
			!stream.lineStartsWithToken("mtllib", true) &&
			!stream.lineStartsWithToken("usemtl", true) &&
			!stream.lineStartsWithToken("o", true) &&
			!stream.lineStartsWithToken("g", true))
			return false;

		// Keep reading until at least three vertices have been encountered.
		// Any valid OBJ file should contain three or more vertices.
		if(stream.lineStartsWithToken("v", true))
			nverts++;
	}

	return nverts >= 3;
}

/******************************************************************************
* Parses the given input file and stores the data in the given container object.
******************************************************************************/
FileSourceImporter::FrameDataPtr WavefrontOBJImporter::FrameLoader::loadFile()
{
	// Open file for reading.
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Reading OBJ file %1").arg(fileHandle().toString()));
	setProgressMaximum(stream.underlyingSize());

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset, frame().lineNumber);

	// Create output data structure.
	auto frameData = std::make_shared<TriMeshFrameData>();
	TriMesh& mesh = frameData->mesh();

	// List of parsed vertex normals.
	std::vector<Vector3> vertexNormals;
	// The current smoothing group number.
	int smoothingGroup = 0;

	// Parse file line by line.
	while(!stream.eof()) {
		const char* line = stream.readLineTrimLeft();

		// Skip empty lines and comment lines.
		if(line[0] == '\0') continue;
		if(line[0] == '#') continue;

		if(stream.lineStartsWithToken("v", true)) {
			// Parse vertex definition.
			Point3 xyz;
			if(sscanf(line, "v " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &xyz.x(), &xyz.y(), &xyz.z()) != 3)
				throw Exception(tr("Invalid vertex specification in line %1 of OBJ file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			mesh.addVertex(xyz);
		}
		else if(stream.lineStartsWithToken("f", true)) {
			// Parse polygon definition.
			int nVertices = 0;
			int vindices[3];
			int vnindices[3] = {-1,-1,-1};
			const char* s = line + 1;
			for(;;) {
				while(*s <= ' ' && *s != '\0') ++s;			// Skip whitespace in front of token
				if(*s == '\0') break;
				int vi = std::atoi(s);
				if(vi >= 1) {
					if(vi > mesh.vertexCount())
						throw Exception(tr("Invalid polygon specification in line %1 of OBJ file: Vertex index %2 is out of range.").arg(stream.lineNumber()).arg(vi));
					vi--; // Make it zero-based.
				}
				else if(vi <= -1) {
					if(mesh.vertexCount() + vi < 0)
						throw Exception(tr("Invalid polygon specification in line %1 of OBJ file: Vertex index %2 is out of range.").arg(stream.lineNumber()).arg(vi));
					vi = mesh.vertexCount() + vi;
				}
				else throw Exception(tr("Invalid polygon specification in line %1 of OBJ file: Vertex index %2 is invalid.").arg(stream.lineNumber()).arg(vi));
				vindices[std::min(nVertices,2)] = vi;
				while(*s > ' ' && *s != '/') ++s;			// Proceed to end of vertex coordinate index
				if(*s == '/') {
					++s;
					while(*s > ' ' && *s != '/') ++s;		// Proceed to end of texture coordinate index
					if(*s == '/') {
						++s;
						int vni = std::atoi(s);
						if(vni >= 1) {
							if(vni > vertexNormals.size())
								throw Exception(tr("Invalid polygon specification in line %1 of OBJ file: Vertex normal index %2 is out of range.").arg(stream.lineNumber()).arg(vni));
							vni--; // Make it zero-based.
						}
						else if(vni <= -1) {
							if(vertexNormals.size() + vni < 0)
								throw Exception(tr("Invalid polygon specification in line %1 of OBJ file: Vertex normal index %2 is out of range.").arg(stream.lineNumber()).arg(vni));
							vni = (int)vertexNormals.size() + vni;
						}
						else throw Exception(tr("Invalid polygon specification in line %1 of OBJ file: Vertex normal index %2 is invalid.").arg(stream.lineNumber()).arg(vni));
						vnindices[std::min(nVertices,2)] = vni;
					}
					while(*s > ' ') ++s;			// Proceed to end of vertex normal index
				}
				nVertices++;

				// Emit a new face to triangulate the polygon.
				if(nVertices >= 3) {
					TriMeshFace& f = mesh.addFace();
					f.setVertices(vindices[0], vindices[1], vindices[2]);
					if(smoothingGroup != 0 && smoothingGroup < OVITO_MAX_NUM_SMOOTHING_GROUPS)
						f.setSmoothingGroups(1 << (smoothingGroup-1));
					if(nVertices == 3)
						f.setEdgeVisibility(true, true, false);
					else
						f.setEdgeVisibility(false, true, false);
//					if(vnindices[0] != -1 && vnindices[1] != -1 && vnindices[2] != -1) {
//						mesh.setHasNormals(true);
//						mesh.setFaceVertexNormal(mesh.faceCount() - 1, 0, vertexNormals[vnindices[0]]);
//						mesh.setFaceVertexNormal(mesh.faceCount() - 1, 1, vertexNormals[vnindices[1]]);
//						mesh.setFaceVertexNormal(mesh.faceCount() - 1, 2, vertexNormals[vnindices[2]]);
//					}
					vindices[1] = vindices[2];
					vnindices[1] = vnindices[2];
				}
			}
			if(nVertices >= 3)
				mesh.faces().back().setEdgeVisible(2);
		}
		else if(stream.lineStartsWithToken("vn", true)) {
			// Parse vertex normal.
			Vector3 xyz;
			if(sscanf(line, "vn " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &xyz.x(), &xyz.y(), &xyz.z()) != 3)
				throw Exception(tr("Invalid vertex normal in line %1 of OBJ file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			vertexNormals.push_back(xyz);
		}
		else if(stream.lineStartsWithToken("s", true)) {
			if(stream.lineStartsWith("s off"))
				smoothingGroup = 0;
			else {
				if(sscanf(line, "s %i", &smoothingGroup) != 1 || smoothingGroup < 0)
					throw Exception(tr("Invalid smoothing group specification in line %1 of OBJ file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			}
		}
		else if(stream.lineStartsWithToken("mtllib", true))
			continue;
		else if(stream.lineStartsWithToken("usemtl", true))
			continue;
		else if(stream.lineStartsWithToken("vt", true))
			continue;
		else if(stream.lineStartsWithToken("vp", true))
			continue;
		else if(stream.lineStartsWithToken("o", true))
			continue;
		else if(stream.lineStartsWithToken("g", true))
			continue;
		else
			throw Exception(tr("Invalid or unsupported OBJ file format. Encountered unknown token in line %1.").arg(stream.lineNumber()));

		if(!setProgressValueIntermittent(stream.underlyingByteOffset()))
			return {};
	}
	mesh.invalidateVertices();
	mesh.invalidateFaces();
	frameData->setStatus(tr("%1 vertices, %2 triangles").arg(mesh.vertexCount()).arg(mesh.faceCount()));
	return frameData;
}

}	// End of namespace
}	// End of namespace
