///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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
#include <core/utilities/io/CompressedTextReader.h>
#include "WavefrontOBJImporter.h"
#include "TriMeshFrameData.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(WavefrontOBJImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool WavefrontOBJImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

	// Read a couple of lines.
	for(size_t lineNumber = 0; lineNumber < 18; lineNumber++) {
		const char* line = stream.readLineTrimLeft(512);
		if(line[0] == '\0')  // Skip empty lines.
			continue;
		if(stream.lineStartsWith("#")) // Skip comment lines.
			continue;

		// Accept only lines starting with one of the following tokens.
		if(!stream.lineStartsWithToken("v") &&
			!stream.lineStartsWithToken("vn") &&
			!stream.lineStartsWithToken("vt") &&
			!stream.lineStartsWithToken("vp") &&
			!stream.lineStartsWithToken("l") &&
			!stream.lineStartsWithToken("f") &&
			!stream.lineStartsWithToken("s") &&
			!stream.lineStartsWithToken("mtllib") &&
			!stream.lineStartsWithToken("usemtl") &&
			!stream.lineStartsWithToken("o") &&
			!stream.lineStartsWithToken("g"))
			return false;
	}

	return true;
}

/******************************************************************************
* Parses the given input file and stores the data in the given container object.
******************************************************************************/
FileSourceImporter::FrameDataPtr WavefrontOBJImporter::FrameLoader::loadFile(QFile& file)
{
	// Open file for reading.
	CompressedTextReader stream(file, frame().sourceFile.path());
	setProgressText(tr("Reading OBJ file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset, frame().lineNumber);

	// Create output data.
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

		if(stream.lineStartsWithToken("v")) {
			// Parse vertex definition.
			Point3 xyz;
			if(sscanf(line, "v " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &xyz.x(), &xyz.y(), &xyz.z()) != 3)
				throw Exception(tr("Invalid vertex specification in line %1 of OBJ file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			mesh.addVertex(xyz);
		}
		else if(stream.lineStartsWithToken("f")) {
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
		else if(stream.lineStartsWithToken("vn")) {
			// Parse vertex normal.
			Vector3 xyz;
			if(sscanf(line, "vn " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &xyz.x(), &xyz.y(), &xyz.z()) != 3)
				throw Exception(tr("Invalid vertex normal in line %1 of OBJ file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			vertexNormals.push_back(xyz);
		}
		else if(stream.lineStartsWithToken("s")) {
			if(stream.lineStartsWith("s off"))
				smoothingGroup = 0;
			else {
				if(sscanf(line, "s %i", &smoothingGroup) != 1 || smoothingGroup < 0)
					throw Exception(tr("Invalid smoothing group specification in line %1 of OBJ file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			}
		}
		else if(stream.lineStartsWithToken("mtllib"))
			continue;
		else if(stream.lineStartsWithToken("usemtl"))
			continue;
		else if(stream.lineStartsWithToken("vt"))
			continue;
		else if(stream.lineStartsWithToken("vp"))
			continue;
		else if(stream.lineStartsWithToken("o"))
			continue;
		else if(stream.lineStartsWithToken("g"))
			continue;
		else
			throw Exception(tr("Invalid or unsupported OBJ file format. Encountered unknown token in line %1.").arg(stream.lineNumber()));
	}
	mesh.invalidateVertices();
	mesh.invalidateFaces();
	frameData->setStatus(tr("%1 vertices, %2 triangles").arg(mesh.vertexCount()).arg(mesh.faceCount()));
	return frameData;
}

}	// End of namespace
}	// End of namespace
