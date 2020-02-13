////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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

#include <ovito/core/Core.h>
#include <ovito/core/utilities/io/CompressedTextWriter.h>
#include "TriMesh.h"

namespace Ovito {

/******************************************************************************
* Constructs an empty mesh.
******************************************************************************/
TriMesh::TriMesh() : _hasVertexColors(false), _hasFaceColors(false), _hasNormals(false)
{
}

/******************************************************************************
* Clears all vertices and faces.
******************************************************************************/
void TriMesh::clear()
{
	_vertices.clear();
	_faces.clear();
	_vertexColors.clear();
	_faceColors.clear();
	_boundingBox.setEmpty();
	_hasVertexColors = false;
	_hasFaceColors = false;
	_hasNormals = false;
}

/******************************************************************************
* Sets the number of vertices in this mesh.
******************************************************************************/
void TriMesh::setVertexCount(int n)
{
	_vertices.resize(n);
	if(_hasVertexColors)
		_vertexColors.resize(n);
}

/******************************************************************************
* Sets the number of faces in this mesh.
******************************************************************************/
void TriMesh::setFaceCount(int n)
{
	_faces.resize(n);
	if(_hasFaceColors)
		_faceColors.resize(n);
	if(_hasNormals)
		_normals.resize(n * 3);
}

/******************************************************************************
* Adds a new triangle face and returns a reference to it.
* The new face is NOT initialized by this method.
******************************************************************************/
TriMeshFace& TriMesh::addFace()
{
	setFaceCount(faceCount() + 1);
	return _faces.back();
}

/******************************************************************************
* Saves the mesh to the given stream.
******************************************************************************/
void TriMesh::saveToStream(SaveStream& stream)
{
	stream.beginChunk(0x03);

	// Save vertices.
	stream << _vertices;

	// Save vertex colors.
	stream << _hasVertexColors;
	stream << _vertexColors;

	// Save face colors.
	stream << _hasFaceColors;
	stream << _faceColors;

	// Save face normals.
	stream << _hasNormals;
	stream << _normals;

	// Save faces.
	stream << (int)faceCount();
	for(auto face = faces().constBegin(); face != faces().constEnd(); ++face) {
		stream << face->_flags;
		stream << face->_vertices[0];
		stream << face->_vertices[1];
		stream << face->_vertices[2];
		stream << face->_smoothingGroups;
		stream << face->_materialIndex;
	}

	stream.endChunk();
}

/******************************************************************************
* Loads the mesh from the given stream.
******************************************************************************/
void TriMesh::loadFromStream(LoadStream& stream)
{
	int formatVersion = stream.expectChunkRange(0x00, 0x03);

	// Reset mesh.
	clear();

	// Load vertices.
	stream >> _vertices;

	// Load vertex colors.
	stream >> _hasVertexColors;
	stream >> _vertexColors;
	OVITO_ASSERT(_vertexColors.size() == _vertices.size() || !_hasVertexColors);

	if(formatVersion >= 2) {
		// Load face colors.
		stream >> _hasFaceColors;
		stream >> _faceColors;
	}

	if(formatVersion >= 3) {
		// Load normals.
		stream >> _hasNormals;
		stream >> _normals;
	}

	// Load faces.
	int nFaces;
	stream >> nFaces;
	_faces.resize(nFaces);
	for(TriMeshFace& face : faces()) {
		stream >> face._flags;
		stream >> face._vertices[0];
		stream >> face._vertices[1];
		stream >> face._vertices[2];
		stream >> face._smoothingGroups;
		stream >> face._materialIndex;
	}

	stream.closeChunk();
}

/******************************************************************************
* Flip the orientation of all faces.
******************************************************************************/
void TriMesh::flipFaces()
{
	for(TriMeshFace& face : faces()) {
		face.setVertices(face.vertex(2), face.vertex(1), face.vertex(0));
		face.setEdgeVisibility(face.edgeVisible(1), face.edgeVisible(0), face.edgeVisible(2));
	}
	if(hasNormals()) {
		// Negate normal vectors and swap normals of first and third face vertex.
		for(auto n = _normals.begin(); n != _normals.end(); ) {
			Vector3& n1 = *n++;
			*n = -(*n);
			++n;
			Vector3 temp = -(*n);
			*n = -n1;
			n1 = temp;
			++n;
		}
	}
	invalidateFaces();
}

/******************************************************************************
* Performs a ray intersection calculation.
******************************************************************************/
bool TriMesh::intersectRay(const Ray3& ray, FloatType& t, Vector3& normal, int& faceIndex, bool backfaceCull) const
{
	FloatType bestT = FLOATTYPE_MAX;
	int index = 0;
	for(auto face = faces().constBegin(); face != faces().constEnd(); ++face) {

		Point3 v0 = vertex(face->vertex(0));
		Vector3 e1 = vertex(face->vertex(1)) - v0;
		Vector3 e2 = vertex(face->vertex(2)) - v0;

		Vector3 h = ray.dir.cross(e2);
		FloatType a = e1.dot(h);

		if(std::fabs(a) < FLOATTYPE_EPSILON)
			continue;

		FloatType f = 1 / a;
		Vector3 s = ray.base - v0;
		FloatType u = f * s.dot(h);

		if(u < FloatType(0) || u > FloatType(1))
			continue;

		Vector3 q = s.cross(e1);
		FloatType v = f * ray.dir.dot(q);

		if(v < FloatType(0) || u + v > FloatType(1))
			continue;

		FloatType tt = f * e2.dot(q);

		if(tt < FLOATTYPE_EPSILON)
			continue;

		if(tt >= bestT)
			continue;

		// Compute face normal.
		Vector3 faceNormal = e1.cross(e2);
		if(faceNormal.isZero(FLOATTYPE_EPSILON)) continue;

		// Do backface culling.
		if(backfaceCull && faceNormal.dot(ray.dir) >= 0)
			continue;

		bestT = tt;
		normal = faceNormal;
		faceIndex = (face - faces().constBegin());
	}

	if(bestT != FLOATTYPE_MAX) {
		t = bestT;
		return true;
	}

	return false;
}

/******************************************************************************
* Exports the triangle mesh to a VTK file.
******************************************************************************/
void TriMesh::saveToVTK(CompressedTextWriter& stream)
{
	stream << "# vtk DataFile Version 3.0\n";
	stream << "# Triangle mesh\n";
	stream << "ASCII\n";
	stream << "DATASET UNSTRUCTURED_GRID\n";
	stream << "POINTS " << vertexCount() << " double\n";
	for(const Point3& p : vertices())
		stream << p.x() << " " << p.y() << " " << p.z() << "\n";
	stream << "\nCELLS " << faceCount() << " " << (faceCount() * 4) << "\n";
	for(const TriMeshFace& f : faces()) {
		stream << "3";
		for(size_t i = 0; i < 3; i++)
			stream << " " << f.vertex(i);
		stream << "\n";
	}
	stream << "\nCELL_TYPES " << faceCount() << "\n";
	for(size_t i = 0; i < faceCount(); i++)
		stream << "5\n";	// Triangle
}

/******************************************************************************
* Exports the triangle mesh to a Wavefront .obj file.
******************************************************************************/
void TriMesh::saveToOBJ(CompressedTextWriter& stream)
{
	stream << "# Wavefront OBJ file written by OVITO\n";
	stream << "# List of geometric vertices:\n";
	for(const Point3& p : vertices())
		stream << "v " << p.x() << " " << p.y() << " " << p.z() << "\n";
	stream << "# List of faces:\n";
	for(const TriMeshFace& f : faces()) {
		stream << "f ";
		for(size_t i = 0; i < 3; i++)
			stream << " " << (f.vertex(i)+1);
		stream << "\n";
	}
}

/******************************************************************************
* Clips the mesh at the given plane.
******************************************************************************/
void TriMesh::clipAtPlane(const Plane3& plane)
{
	TriMesh clippedMesh;
	clippedMesh.setHasVertexColors(hasVertexColors());
	clippedMesh.setHasFaceColors(hasFaceColors());

	// Clip vertices.
	std::vector<int> existingVertexMapping(vertexCount(), -1);
	for(int vindex = 0; vindex < vertexCount(); vindex++) {
		if(plane.classifyPoint(vertex(vindex)) != 1) {
			existingVertexMapping[vindex] = clippedMesh.addVertex(vertex(vindex));
			if(hasVertexColors())
				clippedMesh.vertexColors().back() = vertexColor(vindex);
		}
	}

	// Clip edges.
	clippedMesh.setHasNormals(hasNormals());
	std::map<std::pair<int,int>, std::pair<int,FloatType>> newVertexMapping;
	for(const TriMeshFace& face : faces()) {
		for(int v = 0; v < 3; v++) {
			auto vindices = std::make_pair(face.vertex(v), face.vertex((v+1)%3));
			if(vindices.first > vindices.second) std::swap(vindices.first, vindices.second);
			const Point3& v1 = vertex(vindices.first);
			const Point3& v2 = vertex(vindices.second);
			// Check if edge intersects plane.
			FloatType z1 = plane.pointDistance(v1);
			FloatType z2 = plane.pointDistance(v2);
			if((z1 < FLOATTYPE_EPSILON && z2 > FLOATTYPE_EPSILON) || (z2 < FLOATTYPE_EPSILON && z1 > FLOATTYPE_EPSILON)) {
				if(newVertexMapping.find(vindices) == newVertexMapping.end()) {
					FloatType t = z1 / (z1 - z2);
					Point3 intersection = v1 + (v2 - v1) * t;
					newVertexMapping.emplace(vindices, std::make_pair(clippedMesh.addVertex(intersection), t));
					if(hasVertexColors()) {
						const ColorA& color1 = vertexColor(vindices.first);
						const ColorA& color2 = vertexColor(vindices.second);
						ColorA& newColor = clippedMesh.vertexColors().back();
						newColor.r() = color1.r() + (color2.r() - color1.r()) * t;
						newColor.g() = color1.g() + (color2.g() - color1.g()) * t;
						newColor.b() = color1.b() + (color2.b() - color1.b()) * t;
						newColor.a() = color1.a() + (color2.a() - color1.a()) * t;
					}
				}
			}
		}
	}

	// Clip faces.
	int faceIndex = 0;
	for(const TriMeshFace& face : faces()) {
		for(int v0 = 0; v0 < 3; v0++) {
			Point3 current_pos = vertex(face.vertex(v0));
			int current_classification = plane.classifyPoint(current_pos);
			if(current_classification == -1) {
				int newface[4];
				Vector3 newface_normals[4];
				bool newface_edge_visibility[4];
				int vout = 0;
				int next_classification;
				for(int v = v0; v < v0 + 3; v++, current_classification = next_classification) {
					int vwrapped = v % 3;
					next_classification = plane.classifyPoint(vertex(face.vertex((v+1)%3)));
					if((next_classification <= 0 && current_classification <= 0) || (next_classification == 1 && current_classification == 0)) {
						OVITO_ASSERT(existingVertexMapping[face.vertex(vwrapped)] >= 0);
						OVITO_ASSERT(vout <= 3);
						if(hasNormals())
							newface_normals[vout] = faceVertexNormal(faceIndex, vwrapped);
						newface_edge_visibility[vout] = face.edgeVisible(vwrapped);
						newface[vout++] = existingVertexMapping[face.vertex(vwrapped)];
					}
					else if((current_classification == +1 && next_classification == -1) || (current_classification == -1 && next_classification == +1)) {
						auto vindices = std::make_pair(face.vertex(vwrapped), face.vertex((v+1)%3));
						if(vindices.first > vindices.second) std::swap(vindices.first, vindices.second);
						auto ve = newVertexMapping.find(vindices);
						OVITO_ASSERT(ve != newVertexMapping.end());
						newface_edge_visibility[vout] = face.edgeVisible(vwrapped); 
						if(current_classification == -1) {
							OVITO_ASSERT(vout <= 3);
							if(hasNormals())
								newface_normals[vout] = faceVertexNormal(faceIndex, vwrapped);
							newface[vout++] = existingVertexMapping[face.vertex(vwrapped)];
							newface_edge_visibility[vout] = false;
						}
						OVITO_ASSERT(vout <= 3);
						if(hasNormals()) {
							FloatType t = ve->second.second;
							if(vindices.first != face.vertex(vwrapped)) t = FloatType(1)-t;
							newface_normals[vout] = faceVertexNormal(faceIndex, v%3) * t + faceVertexNormal(faceIndex, (v+1)%3) * (FloatType(1) - t);
							newface_normals[vout].normalizeSafely();
						}
						newface[vout++] = ve->second.first;
					}
				}
				if(vout >= 3) {
					OVITO_ASSERT(newface[0] >= 0 && newface[0] < clippedMesh.vertexCount());
					OVITO_ASSERT(newface[1] >= 0 && newface[1] < clippedMesh.vertexCount());
					OVITO_ASSERT(newface[2] >= 0 && newface[2] < clippedMesh.vertexCount());
					TriMeshFace& face1 = clippedMesh.addFace();
					face1.setVertices(newface[0], newface[1], newface[2]);
					face1.setSmoothingGroups(face.smoothingGroups());
					face1.setMaterialIndex(face.materialIndex());
					if(hasNormals()) {
						auto n = clippedMesh.normals().end() - 3;
						*n++ = newface_normals[0];
						*n++ = newface_normals[1];
						*n = newface_normals[2];
					}
					if(hasFaceColors()) {
						clippedMesh.faceColors().back() = faceColor(faceIndex);
					}
					if(vout == 4) {
						OVITO_ASSERT(newface[3] >= 0 && newface[3] < clippedMesh.vertexCount());
						OVITO_ASSERT(newface[3] != newface[0]);
						face1.setEdgeVisibility(newface_edge_visibility[0], newface_edge_visibility[1], false);
						TriMeshFace& face2 = clippedMesh.addFace();
						face2.setVertices(newface[0], newface[2], newface[3]);
						face2.setSmoothingGroups(face.smoothingGroups());
						face2.setMaterialIndex(face.materialIndex());
						face2.setEdgeVisibility(false, newface_edge_visibility[2], newface_edge_visibility[3]);
						if(hasNormals()) {
							auto n = clippedMesh.normals().end() - 3;
							*n++ = newface_normals[0];
							*n++ = newface_normals[2];
							*n = newface_normals[3];
						}
						if(hasFaceColors()) {
							clippedMesh.faceColors().back() = faceColor(faceIndex);
						}
					}
					else {
						face1.setEdgeVisibility(newface_edge_visibility[0], newface_edge_visibility[1], newface_edge_visibility[2]);
					}
				}
				break;
			}
		}
		faceIndex++;
	}

	this->swap(clippedMesh);
}

/******************************************************************************
* Determines the visibility of face edges depending on the angle between the normals of adjacent faces.
******************************************************************************/
void TriMesh::determineEdgeVisibility(FloatType thresholdAngle)
{
	FloatType dotThreshold = std::cos(thresholdAngle);

	// Build map of face edges and their adjacent faces.
	std::map<std::pair<int,int>,int> edgeMap;
	int faceIndex = 0;
	for(TriMeshFace& face : faces()) {
		for(size_t e = 0; e < 3; e++) {
			int v1 = face.vertex(e);
			int v2 = face.vertex((e+1)%3);
			if(v2 > v1)
				edgeMap.emplace(std::make_pair(v1,v2), faceIndex);
		}
		face.setEdgeVisibility(true, true, true);
		faceIndex++;
	}

	// Helper function that computes the normal of a triangle face.
	auto computeFaceNormal = [this](const TriMeshFace& face) {
		const Point3& p0 = vertex(face.vertex(0));
		Vector3 d1 = vertex(face.vertex(1)) - p0;
		Vector3 d2 = vertex(face.vertex(2)) - p0;
		return d2.cross(d1).safelyNormalized();
	};

	// Visit all face edges again.
	for(TriMeshFace& face : faces()) {
		for(size_t e = 0; e < 3; e++) {
			int v1 = face.vertex(e);
			int v2 = face.vertex((e+1)%3);
			if(v2 < v1) {
				// Look up the adjacent face for the current edge.
				auto iter = edgeMap.find(std::make_pair(v2,v1));
				if(iter != edgeMap.end()) {
					TriMeshFace& adjacentFace = this->face(iter->second);
					// Always retain edges between two faces with different colors or not belonging to the same smoothing group.
					if(adjacentFace.materialIndex() != face.materialIndex())
						continue;
					Vector3 normal1 = computeFaceNormal(face);
					// Look up the opposite edge.
					for(size_t e2 = 0; e2 < 3; e2++) {
						if(adjacentFace.vertex(e2) == v2 && adjacentFace.vertex((e2+1)%3) == v1) {
							Vector3 normal2 = computeFaceNormal(adjacentFace);
							if(normal1.dot(normal2) > dotThreshold) {
								face.setEdgeHidden(e);
								adjacentFace.setEdgeHidden(e2);
							}
							break;
						}
					}
				}
			}
		}
	}
}

/******************************************************************************
* Identifies duplicate vertices and merges them into a single vertex shared
* by multiple faces.
******************************************************************************/
void TriMesh::removeDuplicateVertices(FloatType epsilon)
{
	std::vector<int> remapping(vertexCount(), -1);

	int vcount = vertexCount();
	auto p1 = vertices().cbegin();
	for(int v1 = 0; v1 < vcount; v1++, ++p1) {
		if(remapping[v1] != -1) continue;
		auto p2 = p1 + 1;
		for(int v2 = v1 + 1; v2 < vcount; v2++, ++p2) {
			if(p1->equals(*p2, epsilon)) {
				remapping[v2] = v1;
			}
		}
	}

	int newIndex = 0;
	auto pold = vertices().begin();
	auto pnew = vertices().begin();
	for(int& rm : remapping) {
		if(rm == -1) {
			*pnew++ = *pold++;
			rm = newIndex++;
		}
		else {
			rm = remapping[rm];
			++pold;
		}
	}

	for(TriMeshFace& face : faces()) {
		for(size_t v = 0; v < 3; v++) {
			face.setVertex(v, remapping[face.vertex(v)]);
		}
	}

	setVertexCount(newIndex);
	invalidateVertices();
	invalidateFaces();
}

}	// End of namespace
