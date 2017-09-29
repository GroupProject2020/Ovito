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

#include <plugins/mesh/Mesh.h>
#include <core/utilities/io/CompressedTextReader.h>
#include "VTKFileImporter.h"
#include "TriMeshFrameData.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(VTKFileImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool VTKFileImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

	// Read first line.
	stream.readLine(24);

	// VTK files start with the string "# vtk DataFile Version".
	if(stream.lineStartsWith("# vtk DataFile Version"))
		return true;

	return false;
}

/******************************************************************************
* Parses the given input file and stores the data in the given container object.
******************************************************************************/
FileSourceImporter::FrameDataPtr VTKFileImporter::FrameLoader::loadFile(QFile& file)
{
	// Open file for reading.
	CompressedTextReader stream(file, frame().sourceFile.path());
	setProgressText(tr("Reading VTK file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset);

	// Read first line.
	stream.readLine(1024);

	// Check header code in first line.
	if(!stream.lineStartsWith("# vtk DataFile Version"))
		throw Exception(tr("Invalid first line in VTK file."));

	// Ignore comment line.
	stream.readLine();

	// Read encoding type.
	stream.readLine();
	if(!stream.lineStartsWith("ASCII"))
		throw Exception(tr("Can read only text-based VTK files (ASCII format)."));

	// Read data set type.
	stream.readNonEmptyLine();
	bool isPolyData;
	if(stream.lineStartsWith("DATASET UNSTRUCTURED_GRID"))
		isPolyData = false;
	else if(stream.lineStartsWith("DATASET POLYDATA"))
		isPolyData = true;
	else
		throw Exception(tr("Can read only read VTK files containing triangle polydata or unstructured grids with triangle cells."));

	// Read point count.
	expectKeyword(stream, "POINTS");
	int pointCount;
	if(sscanf(stream.line() + 6, "%i", &pointCount) != 1 || pointCount < 0)
		throw Exception(tr("Invalid number of points in VTK file (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));

	// Create output data.
	auto frameData = std::make_shared<TriMeshFrameData>();

	// Parse point coordinates.
	frameData->mesh().setVertexCount(pointCount);
	auto v = frameData->mesh().vertices().begin();
	size_t component = 0;
	for(int i = 0; i < pointCount; ) {
		if(stream.eof())
			throw Exception(tr("Unexpected end of VTK file in line %1.").arg(stream.lineNumber()));
		const char* s = stream.readLine();
		for(;;) {
			while(*s <= ' ' && *s != '\0') ++s;			// Skip whitespace in front of token
			if(*s == '\0' || i >= pointCount) break;
			(*v)[component++] = (FloatType)std::atof(s);
			if(component == 3) {
				component = 0;
				++v; ++i;
			}
			while(*s > ' ') ++s;						// Proceed to end of token
		}
	}
	frameData->mesh().invalidateVertices();

	int polygonCount;
	if(!isPolyData) {
		// Parse number of cells.
		expectKeyword(stream, "CELLS");
		if(sscanf(stream.line() + 5, "%i", &polygonCount) != 1 || polygonCount < 0)
			throw Exception(tr("Invalid number of cells in VTK file (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
	}
	else {
		// Parse number of polygons.
		expectKeyword(stream, "POLYGONS");
		if(sscanf(stream.line() + 8, "%i", &polygonCount) != 1 || polygonCount < 0)
			throw Exception(tr("Invalid number of polygons in VTK file (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
	}

	// Parse polygons.
	for(int i = 0; i < polygonCount; i++) {
		int vcount, s;
		const char* line = stream.readLine();
		if(sscanf(line, "%i%n", &vcount, &s) != 1 || vcount <= 2)
			throw Exception(tr("Invalid polygon/cell definition in VTK file (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
		unsigned int vindices[3];
		for(int j = 0; j < vcount; j++) {
			line += s;
			if(sscanf(line, "%u%n", &vindices[std::min(j,2)], &s) != 1)
				throw Exception(tr("Invalid polygon/cell definition in VTK file (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
			if(j >= 2) {
				if(vindices[0] >= pointCount || vindices[1] >= pointCount || vindices[2] >= pointCount)
					throw Exception(tr("Vertex indices out of range in polygon/cell (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
				TriMeshFace& f = frameData->mesh().addFace();
				f.setVertices(vindices[0], vindices[1], vindices[2]);
				vindices[1] = vindices[2];
			}
		}
	}
	frameData->mesh().invalidateFaces();

	if(!isPolyData) {
		// Parse cell types
		expectKeyword(stream, "CELL_TYPES");
		for(int i = 0; i < polygonCount; i++) {
			int t;
			if(sscanf(stream.readLine(), "%i", &t) != 1 || t != 5)
				throw Exception(tr("Invalid cell type in VTK file (line %1): %2. Only triangle cells are supported by OVITO.").arg(stream.lineNumber()).arg(stream.lineString()));
		}

		// Look for color information.
		for(; !stream.eof() && !stream.lineStartsWith("CELL_DATA"); )
			stream.readLine();
		for(; !stream.eof() && !stream.lineStartsWith("COLOR_SCALARS"); )
			stream.readLine();
		if(!stream.eof()) {
			int componentCount = 0;
			if(sscanf(stream.line(), "COLOR_SCALARS %*s %i", &componentCount) != 1 || (componentCount != 3 && componentCount != 4))
				throw Exception(tr("Invalid COLOR_SCALARS property in line %1 of VTK file. Component count must be 3 or 4.").arg(stream.lineNumber()));
			frameData->mesh().setHasFaceColors(true);
			auto& faceColors = frameData->mesh().faceColors();
			std::fill(faceColors.begin(), faceColors.end(), ColorA(1,1,1,1));
			component = 0;
			for(int i = 0; i < polygonCount;) {
				if(stream.eof())
					throw Exception(tr("Unexpected end of VTK file in line %1.").arg(stream.lineNumber()));
				const char* s = stream.readLine();
				for(; ;) {
					while(*s <= ' ' && *s != '\0') ++s;			// Skip whitespace in front of token
					if(!*s || i >= polygonCount) break;
					faceColors[i][component++] = (FloatType)std::atof(s);
					if(component == componentCount) {
						component = 0;
						++i;
					}
					while(*s > ' ') ++s;						// Proceed to end of token
				}
			}
			frameData->mesh().invalidateFaces();
		}
	}
	else {
		// ...to be implemented.
	}

	frameData->setStatus(tr("%1 vertices, %2 triangles").arg(pointCount).arg(frameData->mesh().faceCount()));
	return frameData;
}

/******************************************************************************
* Reads from the input stream and throws an exception if the given keyword 
* is not present.
******************************************************************************/
void VTKFileImporter::FrameLoader::expectKeyword(CompressedTextReader& stream, const char* keyword)
{
	stream.readNonEmptyLine();

	// Skip METADATA sections written by Paraview.
	if(stream.lineStartsWith("METADATA")) {
		while(!stream.eof()) {
			const char* line = stream.readLineTrimLeft();
			if(line[0] <= ' ') break;
		}
		stream.readNonEmptyLine();
	}

	if(!stream.lineStartsWith(keyword))
		throw Exception(tr("Invalid or unsupported VTK file format. Expected token '%1' in line %2, but found '%3'.").arg(keyword).arg(stream.lineNumber()).arg(stream.lineString().trimmed()));
}

}	// End of namespace
}	// End of namespace
