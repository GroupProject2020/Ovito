////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/BondType.h>
#include <ovito/mesh/surface/SurfaceMeshData.h>
#include <ovito/mesh/util/CapPolygonTessellator.h>
#include <ovito/core/utilities/mesh/TriMesh.h>
#include "GSDImporter.h"
#include "GSDFile.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(GSDImporter);
DEFINE_PROPERTY_FIELD(GSDImporter, roundingResolution);
SET_PROPERTY_FIELD_LABEL(GSDImporter, roundingResolution, "Shape rounding resolution");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(GSDImporter, roundingResolution, IntegerParameterUnit, 1, 6);

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void GSDImporter::propertyChanged(const PropertyFieldDescriptor& field)
{
	ParticleImporter::propertyChanged(field);

	if(field == PROPERTY_FIELD(roundingResolution)) {
		// Clear shape cache and reload GSD file when the rounding resolution is changed.
		_cacheSynchronization.lockForWrite();
		_particleShapeCache.clear();
		_cacheSynchronization.unlock();
		requestReload();
	}
}

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool GSDImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	QString filename = QDir::toNativeSeparators(file.localFilePath());
	if(!filename.isEmpty()) {
		gsd_handle handle;
		if(::gsd_open(&handle, filename.toLocal8Bit().constData(), GSD_OPEN_READONLY) == gsd_error::GSD_SUCCESS) {
			::gsd_close(&handle);
			return true;
		}
	}

	return false;
}

/******************************************************************************
* Stores the particle shape geometry generated from a JSON string in the internal cache.
******************************************************************************/
void GSDImporter::storeParticleShapeInCache(const QByteArray& jsonString, const TriMeshPtr& mesh)
{
	QWriteLocker locker(&_cacheSynchronization);
	_particleShapeCache.insert(jsonString, mesh);
}

/******************************************************************************
* Looks up a particle shape geometry in the internal cache that was previously
* generated from a JSON string.
******************************************************************************/
TriMeshPtr GSDImporter::lookupParticleShapeInCache(const QByteArray& jsonString) const
{
	QReadLocker locker(&_cacheSynchronization);
	auto iter = _particleShapeCache.find(jsonString);
	return (iter != _particleShapeCache.end()) ? iter.value() : TriMeshPtr();
}

/******************************************************************************
* Scans the input file for simulation timesteps.
******************************************************************************/
void GSDImporter::FrameFinder::discoverFramesInFile(QVector<FileSourceImporter::Frame>& frames)
{
	setProgressText(tr("Scanning file %1").arg(fileHandle().toString()));

	// First close text stream, we don't need it here.
	QString filename = QDir::toNativeSeparators(fileHandle().localFilePath());
	if(filename.isEmpty())
		throw Exception(tr("The GSD file reader supports reading only from physical files. Cannot read data from an in-memory buffer."));

	// Open GSD file for reading.
	GSDFile gsd(filename.toLocal8Bit().constData());
	uint64_t nFrames = gsd.numerOfFrames();

	Frame frame(fileHandle());
	for(uint64_t i = 0; i < nFrames; i++) {
		frame.byteOffset = i;
		frame.label = tr("Frame %1").arg(i);
		frames.push_back(frame);
	}
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr GSDImporter::FrameLoader::loadFile()
{
	setProgressText(tr("Reading GSD file %1").arg(fileHandle().toString()));

	// Open GSD file for reading.
	QString filename = QDir::toNativeSeparators(fileHandle().localFilePath());
	if(filename.isEmpty())
		throw Exception(tr("The GSD file reader supports reading only from physical files. Cannot read data from an in-memory buffer."));
	GSDFile gsd(filename.toLocal8Bit().constData());

	// Check schema name.
	if(qstrcmp(gsd.schemaName(), "hoomd") != 0)
		throw Exception(tr("Failed to open GSD file for reading. File schema must be 'hoomd', but found '%1'.").arg(gsd.schemaName()));

	// Create the destination container for loaded data.
	auto frameData = std::make_shared<ParticleFrameData>();

	// Parse number of frames in file.
	uint64_t nFrames = gsd.numerOfFrames();

	// The animation frame to read from the GSD file.
	uint64_t frameNumber = frame().byteOffset;

	// Parse simulation step.
	uint64_t simulationStep = gsd.readOptionalScalar<uint64_t>("configuration/step", frameNumber, 0);
	frameData->attributes().insert(QStringLiteral("Timestep"), QVariant::fromValue(simulationStep));

	// Parse number of dimensions.
	uint8_t ndimensions = gsd.readOptionalScalar<uint8_t>("configuration/dimensions", frameNumber, 3);

	// Parse simulation box.
	std::array<float,6> boxValues = {1,1,1,0,0,0};
	gsd.readOptional1DArray("configuration/box", frameNumber, boxValues);
	AffineTransformation simCell = AffineTransformation::Identity();
	simCell(0,0) = boxValues[0];
	simCell(1,1) = boxValues[1];
	simCell(2,2) = boxValues[2];
	simCell(0,1) = boxValues[3] * boxValues[1];
	simCell(0,2) = boxValues[4] * boxValues[2];
	simCell(1,2) = boxValues[5] * boxValues[2];
	simCell.translation() = simCell.linear() * Vector3(FloatType(-0.5));
	frameData->simulationCell().setMatrix(simCell);
	frameData->simulationCell().setPbcFlags(true, true, true);
	frameData->simulationCell().set2D(ndimensions == 2);

	// Parse number of particles.
	uint32_t numParticles = gsd.readOptionalScalar<uint32_t>("particles/N", frameNumber, 0);

	// Parse list of particle type names.
	QByteArrayList particleTypeNames = gsd.readStringTable("particles/types", frameNumber);
	if(particleTypeNames.empty())
		particleTypeNames.push_back(QByteArrayLiteral("A"));

	// Read particle positions.
	PropertyAccess<Point3> posProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(numParticles, ParticlesObject::PositionProperty, false));
	gsd.readFloatArray("particles/position", frameNumber, posProperty.begin(), numParticles, posProperty.componentCount());
	if(isCanceled()) return {};

	// Create particle types.
	PropertyAccess<int> typeProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(numParticles, ParticlesObject::TypeProperty, false));
	ParticleFrameData::TypeList* typeList = frameData->createPropertyTypesList(typeProperty);
	for(int i = 0; i < particleTypeNames.size(); i++)
		typeList->addTypeId(i, QString::fromUtf8(particleTypeNames[i]));

	// Read particle types.
	if(gsd.hasChunk("particles/typeid", frameNumber))
		gsd.readIntArray("particles/typeid", frameNumber, typeProperty.begin(), numParticles);
	else
		typeProperty.fill(0);
	if(isCanceled()) return {};

	// Parse particle shape information.
	QByteArrayList particleTypeShapes = gsd.readStringTable("particles/type_shapes", frameNumber);
	if(particleTypeShapes.size() == typeList->types().size()) {
		for(int i = 0; i < particleTypeShapes.size(); i++) {
			if(isCanceled()) return {};
			parseParticleShape(i, typeList, numParticles, frameData.get(), particleTypeShapes[i]);
		}
	}

	readOptionalProperty(gsd, "particles/mass", frameNumber, numParticles, ParticlesObject::MassProperty, false, frameData);
	readOptionalProperty(gsd, "particles/charge", frameNumber, numParticles, ParticlesObject::ChargeProperty, false, frameData);
	readOptionalProperty(gsd, "particles/velocity", frameNumber, numParticles, ParticlesObject::VelocityProperty, false, frameData);
	readOptionalProperty(gsd, "particles/image", frameNumber, numParticles, ParticlesObject::PeriodicImageProperty, false, frameData);
	PropertyAccess<FloatType> radiusProperty = readOptionalProperty(gsd, "particles/diameter", frameNumber, numParticles, ParticlesObject::RadiusProperty, false, frameData);
	if(radiusProperty) {
		// Convert particle diameters to radii.
		for(FloatType& r : radiusProperty)
			r /= 2;
	}
	PropertyAccess<Quaternion> orientationProperty = readOptionalProperty(gsd, "particles/orientation", frameNumber, numParticles, ParticlesObject::OrientationProperty, false, frameData);
	if(orientationProperty) {
		// Convert quaternion representation from GSD format to OVITO's internal format.
		// Left-shift all quaternion components by one: (W,X,Y,Z) -> (X,Y,Z,W).
		for(Quaternion& q : orientationProperty)
			std::rotate(q.begin(), q.begin() + 1, q.end());
	}
	if(isCanceled()) return {};

	// Read any user-defined particle properties.
	const char* chunkName = gsd.findMatchingChunkName("log/particles/", nullptr);
	while(chunkName) {
		if(isCanceled()) return {};
		readOptionalProperty(gsd, chunkName, frameNumber, numParticles, ParticlesObject::UserProperty, false, frameData);
		chunkName = gsd.findMatchingChunkName("log/particles/", chunkName);
	}

	// Read any user-defined log chunks and add them to the global attributes dictionary.
	chunkName = gsd.findMatchingChunkName("log/", nullptr);
	while(chunkName) {
		QString key(chunkName);
		if(key.count(QChar('/')) == 1) {
			key.remove(0, 4);
			frameData->attributes().insert(key, gsd.readVariant(chunkName, frameNumber));
		}
		chunkName = gsd.findMatchingChunkName("log/", chunkName);
	}

	// Parse number of bonds.
	uint32_t numBonds = gsd.readOptionalScalar<uint32_t>("bonds/N", frameNumber, 0);
	if(numBonds != 0) {
		// Read bond list.
		std::vector<int> bondList(numBonds * 2);
		gsd.readIntArray("bonds/group", frameNumber, bondList.data(), numBonds, 2);
		if(isCanceled()) return {};

		// Convert to OVITO format.
		PropertyAccess<ParticleIndexPair> bondTopologyProperty = frameData->addBondProperty(BondsObject::OOClass().createStandardStorage(numBonds, BondsObject::TopologyProperty, false));
		auto bondTopoPtr = bondList.cbegin();
		for(ParticleIndexPair& bond : bondTopologyProperty) {
			if(*bondTopoPtr >= numParticles)
				throw Exception(tr("Nonexistent atom tag in bond list in GSD file."));
			bond[0] = *bondTopoPtr++;
			if(*bondTopoPtr >= numParticles)
				throw Exception(tr("Nonexistent atom tag in bond list in GSD file."));
			bond[1] = *bondTopoPtr++;
		}
		frameData->generateBondPeriodicImageProperty();
		if(isCanceled()) return {};

		// Read bond types.
		if(gsd.hasChunk("bonds/types", frameNumber)) {

			// Parse list of bond type names.
			QByteArrayList bondTypeNames = gsd.readStringTable("bonds/types", frameNumber);
			if(bondTypeNames.empty())
				bondTypeNames.push_back(QByteArrayLiteral("A"));

			// Create bond types.
			PropertyAccess<int> bondTypeProperty = frameData->addBondProperty(BondsObject::OOClass().createStandardStorage(numBonds, BondsObject::TypeProperty, false));
			ParticleFrameData::TypeList* bondTypeList = frameData->createPropertyTypesList(bondTypeProperty, BondType::OOClass());
			for(int i = 0; i < bondTypeNames.size(); i++)
				bondTypeList->addTypeId(i, QString::fromUtf8(bondTypeNames[i]));

			// Read bond types.
			if(gsd.hasChunk("bonds/typeid", frameNumber)) {
				gsd.readIntArray("bonds/typeid", frameNumber, bondTypeProperty.begin(), numBonds);
			}
			else {
				bondTypeProperty.fill(0);
			}
			if(isCanceled()) return {};
		}

		// Read any user-defined bond properties.
		const char* chunkName = gsd.findMatchingChunkName("log/bonds/", nullptr);
		while(chunkName) {
			if(isCanceled()) return {};
			readOptionalProperty(gsd, chunkName, frameNumber, numBonds, BondsObject::UserProperty, true, frameData);
			chunkName = gsd.findMatchingChunkName("log/bonds/", chunkName);
		}
	}

	QString statusString = tr("Number of particles: %1").arg(numParticles);
	if(numBonds != 0)
		statusString += tr("\nNumber of bonds: %1").arg(numBonds);
	frameData->setStatus(statusString);
	return frameData;
}

/******************************************************************************
* Reads the values of a particle or bond property from the GSD file.
******************************************************************************/
PropertyStorage* GSDImporter::FrameLoader::readOptionalProperty(GSDFile& gsd, const char* chunkName, uint64_t frameNumber, uint32_t numElements, int propertyType, bool isBondProperty, const std::shared_ptr<ParticleFrameData>& frameData)
{
	if(gsd.hasChunk(chunkName, frameNumber)) {
		PropertyPtr prop;
		if(propertyType != PropertyStorage::GenericUserProperty) {
			if(!isBondProperty)
				prop = ParticlesObject::OOClass().createStandardStorage(numElements, propertyType, false);
			else
				prop = BondsObject::OOClass().createStandardStorage(numElements, propertyType, false);
		}
		else {
			QString propertyName(chunkName);
			int slashPos = propertyName.lastIndexOf(QChar('/'));
			if(slashPos != -1) propertyName.remove(0, slashPos + 1);
			std::pair<int, size_t> dataTypeAndComponents = gsd.getChunkDataTypeAndComponentCount(chunkName);
			prop = std::make_shared<PropertyStorage>(numElements, dataTypeAndComponents.first, dataTypeAndComponents.second, 0, propertyName, false);
		}
		if(!isBondProperty)
			frameData->addParticleProperty(prop);
		else
			frameData->addBondProperty(prop);
		if(prop->dataType() == PropertyStorage::Float)
			gsd.readFloatArray(chunkName, frameNumber, PropertyAccess<FloatType,true>(prop).begin(), numElements, prop->componentCount());
		else if(prop->dataType() == PropertyStorage::Int)
			gsd.readIntArray(chunkName, frameNumber, PropertyAccess<int,true>(prop).begin(), numElements, prop->componentCount());
		else if(prop->dataType() == PropertyStorage::Int64)
			gsd.readIntArray(chunkName, frameNumber, PropertyAccess<qlonglong,true>(prop).begin(), numElements, prop->componentCount());
		else
			throw Exception(tr("Property '%1' cannot be read from GSD file, because its data type is not supported by OVITO.").arg(prop->name()));
		return prop.get();
	}
	else return nullptr;
}

/******************************************************************************
* Parse a JSON string containing a particle shape definition.
******************************************************************************/
void GSDImporter::FrameLoader::parseParticleShape(int typeId, ParticleFrameData::TypeList* typeList, size_t numParticles, ParticleFrameData* frameData, const QByteArray& shapeSpecString)
{
	// Check if an existing geometry is already stored in the cache for the JSON string.
	TriMeshPtr cacheShapeMesh = _importer->lookupParticleShapeInCache(shapeSpecString);
	if(cacheShapeMesh) {
		// Assign shape to particle type.
		typeList->setTypeShape(typeId, std::move(cacheShapeMesh));
		return; // No need to parse the JSON string again.
	}

	// Parse the JSON string.
	QJsonParseError parsingError;
	QJsonDocument shapeSpec = QJsonDocument::fromJson(shapeSpecString, &parsingError);
	if(shapeSpec.isNull())
		throw Exception(tr("Invalid particle shape specification string in GSD file: %1").arg(parsingError.errorString()));

	// Empty JSON documents are ignored (assuming spherical particle shape with default radius).
	if(!shapeSpec.isObject() || shapeSpec.object().isEmpty())
		return;

	// Parse the "type" field.
	QString shapeType = shapeSpec.object().value("type").toString();
	if(shapeType.isNull() || shapeType.isEmpty())
		throw Exception(tr("Missing 'type' field in particle shape specification in GSD file."));

	if(shapeType == "Sphere") {
		parseSphereShape(typeId, typeList, shapeSpec.object());
	}
	else if(shapeType == "Ellipsoid") {
		parseEllipsoidShape(typeId, typeList, numParticles, frameData, shapeSpec.object());
	}
	else if(shapeType == "Polygon") {
		parsePolygonShape(typeId, typeList, shapeSpec.object(), shapeSpecString);
	}
	else if(shapeType == "ConvexPolyhedron") {
		parseConvexPolyhedronShape(typeId, typeList, shapeSpec.object(), shapeSpecString);
	}
	else if(shapeType == "Mesh") {
		parseMeshShape(typeId, typeList, shapeSpec.object(), shapeSpecString);
	}
	else if(shapeType == "SphereUnion") {
		parseSphereUnionShape(typeId, typeList, shapeSpec.object(), shapeSpecString);
	}
	else {
		qWarning() << "GSD file reader: The following particle shape type is not supported by this version of OVITO:" << shapeType;
	}
}

/******************************************************************************
* Parsing routine for 'Sphere' particle shape definitions.
******************************************************************************/
void GSDImporter::FrameLoader::parseSphereShape(int typeId, ParticleFrameData::TypeList* typeList, QJsonObject definition)
{
	double diameter = definition.value("diameter").toDouble();
	if(diameter <= 0)
		throw Exception(tr("Missing or invalid 'diameter' field in 'Sphere' particle shape definition in GSD file."));
	typeList->setTypeRadius(typeId, diameter / 2);
}

/******************************************************************************
* Parsing routine for 'Ellipsoid' particle shape definitions.
******************************************************************************/
void GSDImporter::FrameLoader::parseEllipsoidShape(int typeId, ParticleFrameData::TypeList* typeList, size_t numParticles, ParticleFrameData* frameData, QJsonObject definition)
{
	Vector3 abc;
	abc.x() = definition.value("a").toDouble();
	abc.y() = definition.value("b").toDouble();
	abc.z() = definition.value("c").toDouble();
	if(abc.x() <= 0)
		throw Exception(tr("Missing or invalid 'a' field in 'Ellipsoid' particle shape definition in GSD file."));
	if(abc.y() <= 0)
		throw Exception(tr("Missing or invalid 'b' field in 'Ellipsoid' particle shape definition in GSD file."));
	if(abc.z() <= 0)
		throw Exception(tr("Missing or invalid 'c' field in 'Ellipsoid' particle shape definition in GSD file."));

	// Create the 'Aspherical Shape' particle property if it doesn't exist yet.
	PropertyAccess<Vector3> ashapeProperty = frameData->findStandardParticleProperty(ParticlesObject::AsphericalShapeProperty);
	if(!ashapeProperty)
		ashapeProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(numParticles, ParticlesObject::AsphericalShapeProperty, true));

	// Assign the [a,b,c] values to those particles which are of the given type.
	ConstPropertyAccess<int> typeProperty = frameData->findStandardParticleProperty(ParticlesObject::TypeProperty);
	for(size_t i = 0; i < numParticles; i++) {
		if(typeProperty[i] == typeId)
			ashapeProperty[i] = abc;
	}
}

/******************************************************************************
* Parsing routine for 'Polygon' particle shape definitions.
******************************************************************************/
void GSDImporter::FrameLoader::parsePolygonShape(int typeId, ParticleFrameData::TypeList* typeList, QJsonObject definition, const QByteArray& shapeSpecString)
{
	// Parse the list of vertices.
	const QJsonValue vertexArrayVal = definition.value("vertices");
	if(!vertexArrayVal.isArray())
		throw Exception(tr("Missing or invalid 'vertex' array in 'Polygon' particle shape definition in GSD file."));
	std::vector<Point2> vertices;
	for(QJsonValue val : vertexArrayVal.toArray()) {
		const QJsonArray coordinateArray = val.toArray();
		if(coordinateArray.size() != 2)
			throw Exception(tr("Invalid vertex value in 'vertex' array of 'Polygon' particle shape definition in GSD file."));
		Point2 vertex = Point2::Origin();
		for(int c = 0; c < 2; c++)
			vertex[c] = coordinateArray[c].toDouble();
		vertices.push_back(vertex);
	}
	if(vertices.size() < 3)
		throw Exception(tr("Invalid 'Polygon' particle shape definition in GSD file: Number of vertices must be at least 3."));

	// Parse rounding radius.
	FloatType roundingRadius = definition.value("rounding_radius").toDouble();
	if(roundingRadius > 0) {
		// Constructed the rounded polygon.
		std::vector<Point2> roundedVertices;
		int res = (1 << (_roundingResolution - 1));
		roundedVertices.reserve(vertices.size()*(res+1));
		auto v1 = vertices.cend() - 1;
		auto v2 = vertices.cbegin();
		auto v3 = vertices.cbegin() + 1;
		Vector2 u1 = (*v1) - (*v2);
		u1.normalizeSafely();
		do {
			Vector2 u2 = (*v2) - (*v3);
			u2.normalizeSafely();
			FloatType theta1 = std::atan2(u1.x(), -u1.y());
			FloatType theta2 = std::atan2(u2.x(), -u2.y());
			FloatType delta_theta = std::fmod(theta2 - theta1, FLOATTYPE_PI*2);
			if(delta_theta < 0) delta_theta += FLOATTYPE_PI*2;
			delta_theta /= res;
			for(int i = 0; i < res+1; i++, theta1 += delta_theta) {
				Vector2 delta(cos(theta1) * roundingRadius, sin(theta1) * roundingRadius);
				roundedVertices.push_back((*v2) + delta);
			}
			v1 = v2;
			v2 = v3;
			++v3;
			if(v3 == vertices.cend()) v3 = vertices.cbegin();
			u1 = u2;
		}
		while(v2 != vertices.cbegin());
		vertices.swap(roundedVertices);
	}

	// Create triangulation of (convex or concave) polygon.
	std::shared_ptr<TriMesh> triMesh = std::make_shared<TriMesh>();
	CapPolygonTessellator tessellator(*triMesh, 2, false, true);
	tessellator.beginPolygon();
	tessellator.beginContour();
	for(const Point2& p : vertices)
		tessellator.vertex(p);
	tessellator.endContour();
	tessellator.endPolygon();
	triMesh->flipFaces();
	triMesh->determineEdgeVisibility();

	// Store shape geometry in internal cache to avoid parsing the JSON string again for other animation frames.
	_importer->storeParticleShapeInCache(shapeSpecString, triMesh);

	// Assign shape to particle type.
	typeList->setTypeShape(typeId, std::move(triMesh));
}

/******************************************************************************
* Recursive helper function that tessellates a corner face.
******************************************************************************/
static void tessellateCornerFacet(SurfaceMeshData::face_index seedFace, int recursiveDepth, FloatType roundingRadius, SurfaceMeshData& mesh, std::vector<Vector3>& vertexNormals, const Point3& center)
{
	if(recursiveDepth <= 1) return;

	// List of edges that should be split during the next iteration.
	std::set<SurfaceMeshData::edge_index> edgeList;

	// List of faces that should be subdivided during the next iteration.
	std::vector<SurfaceMeshData::face_index> faceList, faceList2;

	// Initialize lists.
	faceList.push_back(seedFace);
	SurfaceMeshData::edge_index e = mesh.firstFaceEdge(seedFace);
	do {
		edgeList.insert(e);
		e = mesh.nextFaceEdge(e);
	}
	while(e != mesh.firstFaceEdge(seedFace));

	// Perform iterations of the recursive refinement procedure.
	for(int iteration = 1; iteration < recursiveDepth; iteration++) {

		// Create new vertices at the midpoints of the existing edges.
		for(SurfaceMeshData::edge_index edge : edgeList) {
			Point3 midpoint = mesh.vertexPosition(mesh.vertex1(edge));
			midpoint += mesh.vertexPosition(mesh.vertex2(edge)) - Point3::Origin();
			Vector3 normal = (midpoint * FloatType(0.5)) - center;
			normal.normalizeSafely();
			SurfaceMeshData::vertex_index new_v = mesh.splitEdge(edge, center + normal * roundingRadius);
			vertexNormals.push_back(normal);
		}
		edgeList.clear();

		// Subdivide the faces.
		for(SurfaceMeshData::face_index face : faceList) {
			int order = mesh.topology()->countFaceEdges(face) / 2;
			SurfaceMeshData::edge_index e = mesh.firstFaceEdge(face);
			for(int i = 0; i < order; i++) {
				SurfaceMeshData::edge_index edge2 = mesh.nextFaceEdge(mesh.nextFaceEdge(e));
				e = mesh.splitFace(e, edge2);
				// Put edges and the sub-face itself into the list so that
				// they get refined during the next iteration of the algorithm.
				SurfaceMeshData::edge_index oe = mesh.oppositeEdge(e);
				for(int j = 0; j < 3; j++) {
					edgeList.insert((oe < mesh.oppositeEdge(oe)) ? oe : mesh.oppositeEdge(oe));
					oe = mesh.nextFaceEdge(oe);
				}
				faceList2.push_back(mesh.adjacentFace(oe));
			}
			faceList2.push_back(face);
		}
		faceList.clear();
		faceList.swap(faceList2);
	}
}

/******************************************************************************
* Parsing routine for 'ConvexPolyhedron' particle shape definitions.
******************************************************************************/
void GSDImporter::FrameLoader::parseConvexPolyhedronShape(int typeId, ParticleFrameData::TypeList* typeList, QJsonObject definition, const QByteArray& shapeSpecString)
{
	// Parse the list of vertices.
	std::vector<Point3> vertices;
	const QJsonValue vertexArrayVal = definition.value("vertices");
	if(!vertexArrayVal.isArray())
		throw Exception(tr("Missing or invalid 'vertex' array in 'ConvexPolyhedron' particle shape definition in GSD file."));
	for(QJsonValue val : vertexArrayVal.toArray()) {
		const QJsonArray coordinateArray = val.toArray();
		if(coordinateArray.size() != 3)
			throw Exception(tr("Invalid vertex value in 'vertex' array of 'ConvexPolyhedron' particle shape definition in GSD file."));
		Point3 vertex;
		for(int c = 0; c < 3; c++)
			vertex[c] = coordinateArray[c].toDouble();
		vertices.push_back(vertex);
	}
	if(vertices.size() < 4)
		throw Exception(tr("Invalid 'ConvexPolyhedron' particle shape definition in GSD file: Number of vertices must be at least 4."));

	// Construct the convex hull of the vertices.
	// This yields a half-edge surface mesh data structure.
	SurfaceMeshData mesh;
	mesh.constructConvexHull(std::move(vertices));
	mesh.joinCoplanarFaces();

	// Parse rounding radius.
	FloatType roundingRadius = definition.value("rounding_radius").toDouble();
	std::vector<Vector3> vertexNormals;
	if(roundingRadius > 0) {
		SurfaceMeshData roundedMesh;

		// Maps edges of the old mesh to edges of the new mesh.
		std::vector<SurfaceMeshData::edge_index> edgeMapping(mesh.edgeCount());

		// Copy the faces of the existing mesh over to the new mesh data structure.
		SurfaceMeshData::size_type originalFaceCount = mesh.faceCount();
		for(SurfaceMeshData::face_index face = 0; face <originalFaceCount; face++) {

			// Compute the offset by which the face needs to be extruded outward.
			Vector3 faceNormal = mesh.computeFaceNormal(face);
			Vector3 offset = faceNormal * roundingRadius;

			// Duplicate the vertices and shift them along the extrusion vector.
			SurfaceMeshData::size_type faceVertexCount = 0;
			SurfaceMeshData::edge_index e = mesh.firstFaceEdge(face);
			do {
				roundedMesh.createVertex(mesh.vertexPosition(mesh.vertex1(e)) + offset);
				vertexNormals.push_back(faceNormal);
				faceVertexCount++;
				e = mesh.nextFaceEdge(e);
			}
			while(e != mesh.firstFaceEdge(face));

			// Connect the duplicated vertices by a new face.
			SurfaceMeshData::face_index new_f = roundedMesh.createFace(roundedMesh.topology()->end_vertices() - faceVertexCount, roundedMesh.topology()->end_vertices());

			// Register the newly created edges.
			SurfaceMeshData::edge_index new_e = roundedMesh.firstFaceEdge(new_f);
			do {
				edgeMapping[e] = new_e;
				e = mesh.nextFaceEdge(e);
				new_e = roundedMesh.nextFaceEdge(new_e);
			}
			while(e != mesh.firstFaceEdge(face));
		}

		// Insert new faces in between two faces that share an edge.
		for(SurfaceMeshData::edge_index e = 0; e < mesh.edgeCount(); e++) {
			// Skip every other half-edge.
			if(e > mesh.oppositeEdge(e)) continue;

			SurfaceMeshData::edge_index edge = edgeMapping[e];
			SurfaceMeshData::edge_index opposite_edge = edgeMapping[mesh.oppositeEdge(e)];

			SurfaceMeshData::face_index new_f = roundedMesh.createFace({
				roundedMesh.vertex2(edge),
				roundedMesh.vertex1(edge),
				roundedMesh.vertex2(opposite_edge),
				roundedMesh.vertex1(opposite_edge) });

			roundedMesh.linkOppositeEdges(edge, roundedMesh.firstFaceEdge(new_f));
			roundedMesh.linkOppositeEdges(opposite_edge, roundedMesh.nextFaceEdge(roundedMesh.nextFaceEdge(roundedMesh.firstFaceEdge(new_f))));
		}

		// Fill in the holes at the vertices of the old mesh.
		for(SurfaceMeshData::edge_index original_edge = 0; original_edge < edgeMapping.size(); original_edge++) {
			SurfaceMeshData::edge_index new_edge = roundedMesh.oppositeEdge(edgeMapping[original_edge]);
			SurfaceMeshData::edge_index border_edges[2] = {
				roundedMesh.nextFaceEdge(new_edge),
				roundedMesh.prevFaceEdge(new_edge)
			};
			SurfaceMeshData::vertex_index corner_vertices[2] = {
				mesh.vertex1(original_edge),
				mesh.vertex2(original_edge)
			};
			for(int i = 0; i < 2; i++) {
				SurfaceMeshData::edge_index e = border_edges[i];
				if(roundedMesh.hasOppositeEdge(e)) continue;
				SurfaceMeshData::face_index new_f = roundedMesh.createFace({});
				SurfaceMeshData::edge_index edge = e;
				do {
					SurfaceMeshData::edge_index new_e = roundedMesh.createOppositeEdge(edge, new_f);
					edge = roundedMesh.prevFaceEdge(roundedMesh.oppositeEdge(roundedMesh.prevFaceEdge(roundedMesh.oppositeEdge(roundedMesh.prevFaceEdge(edge)))));
				}
				while(edge != e);

				// Tessellate the inserted corner element.
				tessellateCornerFacet(new_f, _roundingResolution, roundingRadius, roundedMesh, vertexNormals, mesh.vertexPosition(corner_vertices[i]));
			}
		}

		// Tessellate the inserted edge elements.
		for(SurfaceMeshData::edge_index e = 0; e < mesh.edgeCount(); e++) {
			// Skip every other half-edge.
			if(e > mesh.oppositeEdge(e)) continue;

			SurfaceMeshData::edge_index startEdge = roundedMesh.oppositeEdge(edgeMapping[e]);
			SurfaceMeshData::edge_index edge1 = roundedMesh.prevFaceEdge(roundedMesh.prevFaceEdge(startEdge));
			SurfaceMeshData::edge_index edge2 = roundedMesh.nextFaceEdge(startEdge);

			for(int i = 1; i < (1<<(_roundingResolution-1)); i++) {
				edge2 = roundedMesh.splitFace(edge1, edge2);
				edge1 = roundedMesh.prevFaceEdge(edge1);
				edge2 = roundedMesh.nextFaceEdge(edge2);
			}
		}

		OVITO_ASSERT(roundedMesh.topology()->isClosed());

		// Adopt the newly constructed mesh as particle shape.
		mesh.swap(roundedMesh);
	}

	// Convert half-edge mesh into a conventional triangle mesh for visualization.
	std::shared_ptr<TriMesh> triMesh = std::make_shared<TriMesh>();
	mesh.convertToTriMesh(*triMesh, false);
	if(triMesh->faceCount() == 0) {
		qWarning() << "GSD file reader: Convex hull construction did not produce a valid triangle mesh for particle type" << typeId;
		return;
	}

	// Assign precomputed vertex normals to triangle mesh for smooth shading of rounded edges.
	OVITO_ASSERT(vertexNormals.empty() || vertexNormals.size() == triMesh->vertexCount());
	if(!vertexNormals.empty()) {
		triMesh->setHasNormals(true);
		auto normal = triMesh->normals().begin();
		for(const TriMeshFace& face : triMesh->faces()) {
			for(int v = 0; v < 3; v++)
				*normal++ = vertexNormals[face.vertex(v)];
		}
	}

	// Store shape geometry in internal cache to avoid parsing the JSON string again for other animation frames.
	_importer->storeParticleShapeInCache(shapeSpecString, triMesh);

	// Assign shape to particle type.
	typeList->setTypeShape(typeId, std::move(triMesh));
}

/******************************************************************************
* Parsing routine for 'Mesh' particle shape definitions.
******************************************************************************/
void GSDImporter::FrameLoader::parseMeshShape(int typeId, ParticleFrameData::TypeList* typeList, QJsonObject definition, const QByteArray& shapeSpecString)
{
	// Parse the list of vertices.
	std::shared_ptr<TriMesh> triMesh = std::make_shared<TriMesh>();
	const QJsonValue vertexArrayVal = definition.value("vertices");
	if(!vertexArrayVal.isArray())
		throw Exception(tr("Missing or invalid 'vertex' array in 'Mesh' particle shape definition in GSD file."));
	for(QJsonValue val : vertexArrayVal.toArray()) {
		const QJsonArray coordinateArray = val.toArray();
		if(coordinateArray.size() != 3)
			throw Exception(tr("Invalid vertex value in 'vertex' array of 'Mesh' particle shape definition in GSD file."));
		Point3 vertex;
		for(int c = 0; c < 3; c++)
			vertex[c] = coordinateArray[c].toDouble();
		triMesh->addVertex(vertex);
	}
	if(triMesh->vertexCount() < 3)
		throw Exception(tr("Invalid 'Mesh' particle shape definition in GSD file: Number of vertices must be at least 3."));

	// Parse the face list.
	const QJsonValue faceArrayVal = definition.value("indices");
	if(!faceArrayVal.isArray())
		throw Exception(tr("Missing or invalid 'indices' array in 'Mesh' particle shape definition in GSD file."));
	for(QJsonValue val : faceArrayVal.toArray()) {
		const QJsonArray indicesArray = val.toArray();
		if(indicesArray.size() < 3)
			throw Exception(tr("Invalid face definition in 'indices' array of 'Mesh' particle shape definition in GSD file."));
		int nVertices = 0;
		int vindices[3];

		// Parse face vertex list and triangle the face in case it has more than 3 vertices.
		for(const QJsonValue val2 : indicesArray) {
			vindices[std::min(nVertices,2)] = val2.toInt();
			if(!val2.isDouble() || vindices[std::min(nVertices,2)] < 0 || vindices[std::min(nVertices,2)] >= triMesh->vertexCount())
				throw Exception(tr("Invalid face definition in 'indices' array of 'Mesh' particle shape definition in GSD file. Vertex index is out of range."));
			nVertices++;
			if(nVertices >= 3) {
				triMesh->addFace().setVertices(vindices[0], vindices[1], vindices[2]);
				vindices[1] = vindices[2];
			}
		}
	}
	if(triMesh->faceCount() < 1)
		throw Exception(tr("Invalid 'Mesh' particle shape definition in GSD file: Face list is empty."));

	// Render only sharp edges of the mesh in wireframe mode.
	triMesh->determineEdgeVisibility();

	// Store shape geometry in internal cache to avoid parsing the JSON string again for other animation frames.
	_importer->storeParticleShapeInCache(shapeSpecString, triMesh);

	// Assign shape to particle type.
	typeList->setTypeShape(typeId, std::move(triMesh));
}

/******************************************************************************
* Parsing routine for 'SphereUnion' particle shape definitions.
******************************************************************************/
void GSDImporter::FrameLoader::parseSphereUnionShape(int typeId, ParticleFrameData::TypeList* typeList, QJsonObject definition, const QByteArray& shapeSpecString)
{
	// Parse the list of sphere centers.
	std::vector<Point3> centers;
	const QJsonValue centersArrayVal = definition.value("centers");
	if(!centersArrayVal.isArray())
		throw Exception(tr("Missing or invalid 'centers' array in 'SphereUnion' particle shape definition in GSD file."));
	for(QJsonValue val : centersArrayVal.toArray()) {
		const QJsonArray coordinateArray = val.toArray();
		if(coordinateArray.size() != 3)
			throw Exception(tr("Invalid vertex value in 'centers' array of 'SphereUnion' particle shape definition in GSD file."));
		Point3 center;
		for(int c = 0; c < 3; c++)
			center[c] = coordinateArray[c].toDouble();
		centers.push_back(center);
	}
	if(centers.size() < 1)
		throw Exception(tr("Invalid 'SphereUnion' particle shape definition in GSD file: Number of spheres must be at least 1."));
	
	// Parse the list of sphere diameters.
	std::vector<FloatType> diameters;
	const QJsonValue diametersArrayVal = definition.value("diameters");
	if(!diametersArrayVal.isArray())
		throw Exception(tr("Missing or invalid 'diameters' array in 'SphereUnion' particle shape definition in GSD file."));
	for(QJsonValue val : diametersArrayVal.toArray()) {
		diameters.push_back(val.toDouble());
		if(diameters.back() <= 0)
			throw Exception(tr("Invalid diameters value in 'diameters' array of 'SphereUnion' particle shape definition in GSD file."));
	}
	if(diameters.size() != centers.size())
		throw Exception(tr("Invalid 'SphereUnion' particle shape definition in GSD file: Length of diameters array must match length of centers array."));

	// Template for a triangulated sphere:
	const FloatType unitSphereVertices[][3] = {{-0.214321,0.433781,0.126096}, {0.081534,0.470761,0.147433}, {0.240092,-0.208622,-0.385788}, {0.090232,-0.40869,0.273552}, {0.00345772,0.154322,-0.475577}, {-0.0387395,0.376704,0.326485}, {0.333229,-0.356237,0.10979}, {-0.091808,0.117787,0.477177}, {0.38883,-0.170079,-0.264356}, {0.183348,-0.465132,-0.00599245}, {0.313217,-0.388942,0.024885}, {-0.476455,-0.105021,-0.109366}, {-0.185157,0.455373,-0.0913885}, {0.25242,-0.366522,-0.227916}, {0.498854,0.0322648,-0.0101661}, {0.0797515,-0.491111,-0.0494916}, {-0.314592,0.362158,0.140971}, {-0.400942,-0.298,0.0210121}, {-0.192522,0.0092462,-0.461357}, {0.301702,0.209346,-0.339338}, {0.483318,0.109337,0.0667005}, {-0.398828,0.0488889,0.297567}, {0.108447,-0.0317376,-0.487065}, {0.0937935,-0.381736,0.308999}, {-0.357058,-0.277015,-0.213946}, {0.292643,-0.404942,-0.0195471}, {0.422608,0.0070385,-0.26712}, {0.238903,0.239781,-0.368009}, {-0.217173,0.443362,0.079153}, {0.057752,-0.364559,0.337286}, {-0.297328,-0.165936,0.366144}, {-0.20723,-0.0701565,-0.449593}, {0.0826165,-0.493123,-0.0018938}, {-0.0450074,0.227622,0.442903}, {-0.0677015,0.448578,-0.210225}, {0.289851,-0.240393,0.328935}, {0.262001,-0.096737,-0.414726}, {-0.471173,-0.073114,-0.150501}, {0.270127,0.379661,-0.181354}, {0.471778,0.0179393,-0.164633}, {0.325806,0.37852,0.0239214}, {-0.264653,0.413803,-0.093415}, {0.197117,0.235296,-0.394691}, {0.222323,-0.140253,0.425325}, {0.494399,-0.054201,-0.0513065}, {0.0507975,0.241781,0.434698}, {0.423574,0.249388,-0.0916015}, {0.478374,-0.0271013,-0.142909}, {0.129007,0.464118,0.133982}, {-0.217684,0.390392,-0.224071}, {-0.388974,-0.313139,-0.0253667}, {-0.438422,-0.127169,-0.203996}, {0.309761,0.391782,-0.0235663}, {-0.349428,-0.346129,0.0899665}, {0.381931,0.292511,-0.136258}, {-0.072019,-0.090283,0.48648}, {-0.491412,0.0429545,0.0816675}, {-0.386577,0.289585,0.129223}, {-0.114511,-0.0552735,-0.483562}, {-0.045925,0.27715,0.413616}, {0.314505,0.359308,0.148273}, {-0.288225,-0.39773,-0.0934705}, {0.30905,0.051092,-0.389715}, {-0.364227,-0.296105,-0.172221}, {0.29777,0.340525,-0.213015}, {0.402782,-0.0271909,-0.295003}, {0.276806,0.18863,-0.371211}, {0.294397,-0.351982,0.198593}, {-0.183254,-0.346689,-0.310202}, {0.270871,0.247933,-0.33935}, {0.276012,-0.415344,0.0361494}, {-0.0518145,-0.246856,-0.431715}, {-0.204091,0.371578,0.265098}, {0.34721,0.358544,-0.0298404}, {-0.174367,0.423437,0.200741}, {-0.34038,0.315225,-0.186479}, {0.11607,0.322612,-0.363936}, {0.0431574,-0.442943,0.227902}, {0.319646,0.309792,0.227718}, {0.46412,-0.103783,-0.154345}, {-0.24782,-0.22897,0.368995}, {0.409716,-0.27923,-0.064522}, {0.332856,0.0819995,0.363982}, {0.376431,0.202176,0.259662}, {-0.34617,0.329449,0.147072}, {-0.370804,0.328861,-0.0659915}, {-0.2851,-0.370769,0.17677}, {-0.114368,-0.087432,0.478828}, {-0.425635,-0.248043,-0.085499}, {-0.238743,0.242643,0.366233}, {-0.335891,-0.05198,-0.366708}, {-0.447582,0.146501,0.167951}, {0.279038,-0.385939,-0.152278}, {-0.178838,0.466917,0.00227338}, {-0.405715,-0.103195,0.273398}, {0.426028,-0.149011,0.215164}, {0.417942,-0.205864,-0.181505}, {-0.274027,-0.321837,0.267076}, {0.476627,0.12872,-0.079109}, {0.351645,0.348102,0.071911}, {0.381002,0.00549415,-0.32374}, {-0.25721,0.364455,0.225867}, {-0.373681,0.293774,-0.155111}, {-0.133295,-0.347869,0.333496}, {-0.468521,0.051852,-0.166732}, {-0.458322,-0.117289,-0.161817}, {0.460285,0.174544,-0.0875895}, {0.0338955,-0.127773,0.482208}, {0.101819,0.40397,-0.27648}, {-0.295395,-0.073813,-0.396603}, {0.0152941,0.454338,0.208191}, {-0.226188,-0.0144603,-0.445678}, {0.452516,0.192561,0.0902715}, {0.355923,-0.210424,-0.281141}, {-0.373201,-0.329917,0.0433101}, {0.14727,-0.400814,0.260114}, {0.36197,0.329198,-0.102985}, {0.382525,0.274498,0.168302}, {-0.406474,0.217826,-0.193212}, {-0.175534,-0.292197,-0.365799}, {-0.422955,0.072984,-0.25648}, {-0.250267,-0.388945,-0.189969}, {-0.147594,0.255687,-0.403535}, {0.0892095,-0.191721,-0.453083}, {0.0385095,0.429564,0.252966}, {-0.0164965,0.0442666,0.497764}, {0.3107,0.356378,-0.162666}, {-0.0320805,-0.313704,0.388022}, {0.0362094,-0.189563,-0.461253}, {-0.406529,-0.0514815,0.286504}, {-0.263066,-0.293982,0.307198}, {0.411693,-0.237176,0.155745}, {-0.149183,-0.441481,0.181216}, {-0.083442,0.422098,0.254698}, {0.384981,0.215257,-0.235485}, {-0.364594,-0.166718,-0.298792}, {-0.465502,-0.159857,-0.088058}, {-0.198594,-0.162212,-0.429241}, {0.228267,0.339075,0.287963}, {0.499569,-0.0196494,0.00669265}, {-0.181025,0.276292,-0.375357}, {-0.440878,0.14111,-0.188982}, {-0.165134,-0.453189,0.131721}, {-0.0438872,-0.20314,0.454761}, {-0.123249,0.430168,0.223081}, {0.281025,0.413359,0.0126548}, {0.44912,0.0804575,0.204494}, {-0.0776835,0.123844,-0.47815}, {0.424814,-0.247673,-0.090506}, {-0.469571,-0.167863,0.0363954}, {-0.0158612,-0.168911,-0.470338}, {-0.499237,-0.0272673,-0.00443443}, {0.186385,0.208111,0.41467}, {0.363986,0.291314,-0.180695}, {0.36406,-0.256447,0.227366}, {0.052584,-0.475121,0.146612}, {-0.0191235,0.366046,-0.340065}, {-0.0278783,0.464525,0.182865}, {0.248332,-0.366413,0.232535}, {0.449929,0.0235158,-0.216821}, {-0.170598,0.456467,0.111957}, {-0.16379,-0.110515,0.459303}, {0.400096,-0.105637,-0.280649}, {-0.397861,-0.154686,0.260343}, {-0.250671,-0.0405515,0.43072}, {-0.203059,0.054478,-0.453651}, {-0.088255,0.477204,-0.120361}, {-0.433117,-0.0731765,0.238861}, {0.177204,0.167157,0.436643}, {0.385559,-0.0666675,-0.311287}, {0.438952,0.208979,-0.116829}, {0.451738,-0.190496,0.0982025}, {-0.210765,-0.403303,-0.207181}, {-0.259971,0.266848,0.333478}, {-0.137329,0.474351,0.0783065}, {0.135548,0.046761,0.478999}, {-0.401274,0.157613,-0.253252}, {0.201417,0.29702,0.348152}, {0.470982,-0.16489,-0.031423}, {0.271734,0.40895,-0.0944475}, {0.422291,-0.243684,0.110854}, {0.444547,-0.184892,-0.134882}, {0.346838,-0.305387,0.190899}, {-0.149923,0.447398,-0.165402}, {0.359729,-0.0312123,-0.345863}, {-0.0265358,0.317534,0.385315}, {0.0151357,0.337823,-0.368303}, {-0.37248,-0.015187,0.333208}, {-0.371868,0.31842,0.1016}, {0.200847,-0.231181,0.395242}, {0.348363,0.165238,0.318339}, {0.0210492,-0.00180383,-0.499553}, {0.459541,0.131641,0.146603}, {-0.00876245,0.404811,-0.293345}, {0.298774,-0.0954745,0.389382}, {0.372038,-0.246222,-0.225749}, {-0.190318,-0.208204,-0.412832}, {0.248091,-0.0459336,-0.431672}, {0.491353,-0.025112,-0.089114}, {0.237854,0.196048,-0.393688}, {0.489695,0.0497746,0.087872}, {0.478909,0.093542,0.109073}, {-0.272496,-0.397387,0.133528}, {0.213757,-0.383024,-0.240002}, {-0.229737,0.412336,0.164925}, {-0.065454,0.059288,-0.492139}, {0.237245,0.21014,0.386725}, {0.269105,0.411452,0.091044}, {-0.078633,-0.49241,-0.036732}, {-0.398837,-0.218086,0.208249}, {0.03098,-0.498362,-0.0259948}, {0.371574,-0.333805,-0.02251}, {-0.3182,-0.080218,0.377246}, {0.14515,-0.055572,0.47523}, {-0.465559,0.136848,0.120529}, {0.336516,-0.332146,0.162593}, {-0.301792,0.339084,0.209627}, {-0.0373908,0.0197642,-0.498208}, {0.164255,0.097409,-0.462095}, {0.040622,-0.419905,0.268384}, {-0.192203,0.18618,0.422368}, {-0.0151985,0.488146,0.10716}, {0.230675,-0.411655,0.165315}, {-0.093326,0.476212,0.120466}, {-0.0149276,-0.209481,-0.453756}, {0.368984,-0.125126,-0.313359}, {-0.073663,-0.463811,-0.171621}, {0.127514,0.28514,-0.390429}, {-0.191478,-0.267283,0.37669}, {-0.225497,0.441392,-0.065759}, {-0.092144,0.365731,0.328254}, {-0.221809,-0.434673,-0.1089}, {-0.327209,0.204516,0.317974}, {0.342278,-0.282676,-0.230089}, {-0.461243,0.146976,-0.12511}, {-0.0414571,-0.349319,0.355328}, {0.493289,0.038251,-0.072128}, {0.185545,-0.117738,-0.449122}, {0.285888,0.0427484,0.407971}, {0.05865,-0.386544,-0.311679}, {0.071433,0.128139,0.477994}, {-0.12444,-0.359183,-0.324811}, {0.15292,0.329921,0.343174}, {0.0402825,0.482917,0.123162}, {0.182585,0.438019,-0.157487}, {-0.144522,-0.0161544,-0.478385}, {-0.101531,0.206059,0.444107}, {0.216196,0.442126,0.08823}, {-0.327954,-0.277736,-0.255555}, {-0.374789,-0.0737555,-0.322635}, {0.0721325,0.487485,-0.084588}, {0.211788,-0.353349,-0.283355}, {0.0751895,0.22009,-0.442613}, {-0.412012,0.255748,-0.121815}, {-0.176177,0.465572,-0.0469494}, {-0.348096,-0.331761,-0.136983}, {-0.493208,0.0051141,-0.0819775}, {-0.266458,-0.264519,-0.330197}, {0.353733,-0.085032,-0.34299}, {-0.249631,0.175753,0.395974}, {0.301553,0.299343,-0.263552}, {0.12891,-0.298332,0.379974}, {-0.217624,0.21395,0.396062}, {0.00827555,0.499418,0.0226367}, {0.147364,-0.475867,-0.0428387}, {-0.096557,0.166903,0.461325}, {0.233317,0.381424,0.223783}, {0.0401414,0.448304,-0.217744}, {0.360138,-0.164421,-0.305395}, {-0.30998,-0.315182,0.233607}, {-0.247282,0.128642,-0.415093}, {0.0119031,-0.308356,0.393414}, {-0.0395042,0.482772,-0.123977}, {-0.451068,-0.00235521,0.215714}, {-0.245212,-0.434494,-0.0329562}, {0.214333,-0.450257,-0.0364771}, {0.198322,-0.452267,0.0782485}, {-0.470074,0.0946945,0.141646}, {-0.0365735,-0.381056,-0.32165}, {-0.215218,-0.31241,0.325701}, {0.248363,-0.393533,-0.182884}, {-0.382566,-0.201951,-0.250717}, {0.417823,0.144523,0.233532}, {0.168663,-0.469073,0.0390228}, {-0.00146802,0.27041,-0.420566}, {0.319806,-0.167084,-0.346131}, {0.067927,0.0215931,0.494893}, {0.315696,-0.20529,0.328925}, {0.132916,0.481764,0.0153734}, {0.437019,0.0531485,0.237043}, {-0.215708,0.292994,-0.342964}, {0.191394,-0.00062267,-0.461917}, {0.235801,0.288654,-0.333281}, {-0.195578,-0.380707,0.258478}, {0.00465196,-0.447523,-0.222938}, {-0.325445,-0.189246,-0.329046}, {0.0855455,0.452636,-0.194427}, {-0.101597,0.330624,-0.361061}, {-0.292625,0.072004,-0.398982}, {-0.333427,-0.369193,0.050228}, {-0.151374,0.377639,-0.290645}, {0.377887,0.103223,-0.310719}, {-0.357835,0.206795,0.281406}, {-0.071412,-0.298211,0.394931}, {-0.168145,0.34065,-0.325092}, {0.355128,0.277701,0.216255}, {-0.490885,0.094694,-0.0080432}, {0.116674,0.422442,0.240687}, {-0.137056,-0.405069,0.259103}, {0.41969,-0.0621515,0.26457}, {0.238327,-0.320406,0.300899}, {0.404943,0.190896,0.222665}, {-0.377211,-0.20227,0.258454}, {-0.0692445,0.489559,-0.0744105}, {-0.0303108,-0.00551515,0.49905}, {-0.105202,0.073328,-0.483276}, {0.395691,-0.1738,0.251441}, {-0.151658,-0.249219,0.406066}, {-0.370295,0.239782,-0.235343}, {0.170371,-0.0663735,-0.465369}, {0.103771,-0.271803,0.406639}, {0.346042,0.0198424,-0.360363}, {-0.476253,0.0125262,-0.151742}, {0.384464,0.122562,0.29524}, {-0.476462,0.123226,-0.088315}, {-0.395238,0.30125,0.055096}, {-0.359359,-0.347647,0.00147784}, {-0.376328,0.090182,0.316614}, {0.0731005,0.439837,0.226272}, {-0.144264,0.406019,0.253645}, {0.178607,-0.143066,0.444558}, {0.101727,0.479762,0.097365}, {-0.211421,-0.0746015,0.446918}, {0.057321,-0.0324248,0.495644}, {0.323122,-0.316927,-0.212484}, {0.465369,-0.120419,0.13759}, {0.156478,0.197749,-0.431752}, {-0.26525,0.201169,-0.37306}, {0.428368,0.0062559,0.257802}, {0.121976,0.353795,-0.331588}, {-0.329641,0.182278,-0.328804}, {0.0229916,0.378252,-0.326186}, {-0.259942,0.414662,0.102398}, {0.409836,0.276164,0.075953}, {0.430048,0.253375,-0.0293379}, {0.133909,0.212984,0.432095}, {0.443622,0.148777,-0.176254}, {0.345077,-0.361818,0.00309046}, {-0.198623,0.419249,-0.186493}, {-0.282932,-0.412191,-0.006894}, {-0.070187,-0.109299,-0.482833}, {0.281457,-0.195266,0.364216}, {-0.186795,0.11569,0.449136}, {-0.375237,0.150382,-0.294249}, {-0.0042634,-0.499836,0.0120765}, {-0.205549,0.10119,-0.444421}, {-0.141733,-0.377362,0.29582}, {-0.0359488,0.498384,-0.0179164}, {0.24407,0.129242,0.416805}, {-0.053427,0.404096,0.289572}, {0.329122,-0.124837,-0.355098}, {0.258555,-0.412568,-0.113741}, {0.267114,0.0465486,-0.4201}, {0.260728,0.250306,0.345496}, {0.061118,0.479297,-0.128605}, {0.143888,-0.365455,-0.309417}, {0.334771,-0.283445,0.239973}, {0.230108,0.438416,-0.0695875}, {-0.258257,-0.132876,0.406998}, {0.30012,0.106334,0.385514}, {-0.152028,-0.328163,-0.345248}, {0.129412,-0.456032,-0.15902}, {0.493341,-0.0145816,0.080008}, {-0.121918,0.267965,0.404142}, {-0.0123164,0.0629765,-0.495865}, {0.056661,-0.273149,0.414945}, {0.314794,-0.352063,-0.164182}, {0.022577,-0.48342,-0.125682}, {-0.492893,-0.00423117,0.0838925}, {-0.433489,-0.204442,-0.142446}, {-0.0942145,0.485936,0.070635}, {0.408122,0.0888135,-0.274861}, {-0.398216,0.297918,-0.0516585}, {0.338182,-0.247959,-0.272303}, {0.146131,-0.377138,0.293959}, {-0.085154,-0.186276,0.456126}, {0.055512,0.496702,0.0143395}, {-0.424496,0.0224707,0.263244}, {-0.327342,-0.328871,-0.186257}, {0.22372,-0.423775,-0.142703}, {0.162283,-0.193488,0.43154}, {-0.178582,-0.407305,0.228497}, {0.342932,-0.00128743,0.363863}, {-0.219691,0.245128,-0.376362}, {-0.174005,-0.209666,0.419241}, {-0.29694,-0.11778,-0.384649}, {0.163257,0.287506,0.375083}, {0.353706,0.308555,0.172294}, {0.088744,-0.422772,-0.251771}, {0.0989185,-0.231734,-0.431874}, {0.475813,0.153464,0.00715315}, {0.258994,0.0949765,-0.417015}, {0.142228,0.379297,-0.293096}, {-0.378696,0.081841,-0.316055}, {-0.423133,0.0298509,-0.264702}, {0.0355403,-0.393155,0.306865}, {-0.322083,0.296142,0.241997}, {-0.0538955,-0.459342,0.190002}, {0.093217,0.264381,-0.414021}, {-0.0103904,0.415486,0.277964}, {0.0286149,0.0436257,-0.497271}, {0.29533,0.24734,0.318752}, {0.311734,-0.373945,-0.113961}, {-0.300138,0.168648,0.362596}, {0.134328,-0.433962,-0.208885}, {0.161238,0.369836,0.295337}, {0.460155,-0.0245902,-0.194044}, {-0.412046,0.151232,0.239472}, {0.053732,0.286905,-0.405954}, {0.301482,-0.1484,0.370252}, {0.405025,0.290087,-0.0424707}, {-0.45624,0.000264616,-0.20456}, {0.375018,0.081638,0.320463}, {-0.308368,0.393535,-0.00632205}, {0.00333229,0.307192,-0.394489}, {0.0231923,-0.320839,-0.382785}, {-0.111279,0.0189277,0.487092}, {0.284521,0.390573,0.128454}, {-0.0281837,-0.469552,-0.16949}, {0.222655,0.051657,-0.444698}, {-0.48707,-0.057335,-0.097343}, {-0.255337,0.249883,-0.349803}, {0.0230025,-0.494775,-0.068322}, {-0.0236811,0.494385,-0.0708695}, {0.321255,0.238354,-0.299971}, {-0.303644,0.380865,-0.112881}, {-0.111032,-0.419451,-0.248461}, {-0.237321,-0.181618,-0.400866}, {0.413249,-0.278276,0.0422811}, {0.350031,0.198256,-0.296938}, {-0.335612,0.098767,-0.357225}, {-0.318232,0.312009,-0.226668}, {-0.078157,-0.0162012,-0.493588}, {0.359818,-0.113337,0.328155}, {-0.482901,0.00445961,0.129567}, {-0.108903,-0.131149,-0.470042}, {0.144303,-0.202884,-0.433607}, {-0.186149,0.348371,0.306571}, {-0.125128,-0.431003,0.220407}, {-0.295463,-0.0266132,-0.402484}, {-0.392891,0.0414902,-0.306456}, {-0.091573,-0.380568,0.311098}, {-0.245251,-0.313546,-0.302557}, {0.19786,-0.382949,0.253381}, {-0.291339,0.00494415,0.406322}, {-0.0360005,0.428175,-0.255676}, {0.147257,-0.452444,0.153656}, {0.099212,0.181144,-0.45535}, {0.083623,-0.489712,0.056475}, {0.431337,0.252247,0.0178746}, {0.0695755,0.06158,-0.491291}, {-0.0895175,0.223072,-0.438435}, {-0.477341,-0.125293,0.0802955}, {0.159757,-0.101347,0.462825}, {-0.305044,0.268035,0.291729}, {0.132899,-0.15619,0.456008}, {-0.0985015,0.39146,-0.295054}, {0.480733,-0.067153,-0.119945}, {0.250372,-0.176488,0.395178}, {0.204832,-0.164011,-0.425611}, {-0.0477822,-0.489373,0.090724}, {0.211287,-0.400019,0.212938}, {-0.467006,0.174959,0.035972}, {-0.285992,-0.349854,0.214034}, {-0.238069,0.356544,-0.257293}, {0.433107,0.101881,-0.228119}, {-0.344961,0.1092,0.345076}, {0.17152,-0.380765,-0.274952}, {-0.369727,0.14658,0.303013}, {0.057227,0.495254,-0.0380508}, {-0.476834,0.130888,0.074143}, {0.292375,-0.3031,0.269531}, {-0.352641,-0.296967,0.193532}, {0.343959,-0.230981,0.279893}, {-0.0113373,-0.45517,0.206619}, {-0.337896,0.357837,-0.0881975}, {-0.182699,-0.253892,-0.390077}, {0.431723,0.196882,-0.157647}, {0.167532,-0.277983,-0.38034}, {0.431656,0.170368,0.186141}, {0.158777,-0.419816,0.220327}, {-0.299812,-0.118091,0.382319}, {0.170044,0.453145,0.125476}, {-0.161849,-0.0654615,0.468529}, {-0.179902,0.393398,-0.250746}, {0.230694,0.351872,-0.270122}, {0.230764,0.42514,0.126505}, {0.454083,-0.189408,-0.0890655}, {0.0671115,-0.484765,0.102466}, {0.0603765,-0.485768,-0.101902}, {0.275068,0.373647,0.186349}, {-0.306016,-0.279469,0.279734}, {0.0920215,-0.44758,-0.202988}, {-0.0244148,-0.125638,-0.483341}, {0.418265,-0.189518,0.197831}, {-0.335614,-0.364497,-0.0671235}, {-0.47721,0.0802595,-0.125812}, {0.16089,0.253202,-0.400004}, {-0.385124,0.201782,-0.246909}, {0.121713,-0.30275,-0.378851}, {0.356482,-0.347247,0.0483697}, {0.0639945,-0.0143667,-0.495679}, {0.307068,-0.283974,-0.273985}, {-0.067939,-0.163991,-0.467431}, {0.0409468,-0.226766,0.443735}, {-0.476513,0.0459584,0.144305}, {0.481434,-0.105551,0.084142}, {-0.176962,0.437837,0.164265}, {0.17179,-0.437694,-0.170036}, {0.0372856,0.244915,-0.434312}, {0.147392,0.441191,0.183374}, {-0.403375,-0.28404,-0.0812975}, {0.267741,0.330311,0.263078}, {-0.394778,-0.152762,-0.266108}, {0.0172449,-0.468369,-0.174163}, {-0.385687,-0.306868,0.08413}, {-0.157738,-0.151222,-0.449723}, {-0.092888,-0.489894,0.0370919}, {0.483108,-0.0958095,-0.0861855}, {-0.0481775,-0.334301,-0.368675}, {-0.35544,0.272734,0.221988}, {0.383346,0.181503,-0.264768}, {-0.238915,-0.425076,0.110589}, {0.206106,0.389581,-0.236108}, {-0.290009,-0.211856,0.347868}, {-0.142216,0.295617,0.377339}, {-0.437246,0.223727,0.093603}, {-0.498884,0.0132911,-0.030631}, {-0.413828,0.252738,0.121942}, {0.037247,0.291924,0.404219}, {-0.113787,0.43907,-0.210403}, {0.136623,-0.22731,0.423868}, {0.434558,-0.246389,-0.0212654}, {0.125843,-0.481549,0.0476863}, {0.406483,-0.126142,0.262411}, {0.208515,0.330139,-0.312298}, {-0.422086,0.258536,-0.0707315}, {-0.122468,0.163425,-0.456392}, {-0.416751,-0.224381,0.16116}, {0.478421,0.0653365,-0.129783}, {-0.444539,0.186151,0.133164}, {-0.286726,0.318989,0.256972}, {0.253346,-0.0834375,0.422911}, {-0.0411264,0.220963,-0.446636}, {0.437372,-0.240852,0.0263809}, {0.179167,0.0616915,0.462703}, {0.196618,0.186473,-0.420202}, {-0.223671,-0.348845,-0.279782}, {-0.0820685,-0.486575,-0.0806835}, {0.327567,0.250165,0.28305}, {0.140543,-0.11873,-0.464921}, {-0.333247,-0.14675,0.342652}, {-0.152146,0.082426,0.469102}, {-0.250225,0.00781555,0.432812}, {-0.283259,0.338007,-0.235618}, {-0.090214,-0.410986,0.270097}, {0.0221782,0.427787,-0.257887}, {0.00223808,-0.391779,-0.310652}, {0.494744,0.0604635,0.0396466}, {-0.390277,0.312418,0.0089152}, {-0.351901,-0.052334,0.351321}, {-0.445465,-0.18886,0.126067}, {-0.084284,0.491974,-0.0292917}, {-0.314598,-0.315538,-0.226857}, {-0.203816,-0.453649,-0.0515945}, {-0.149197,-0.209815,-0.428623}, {-0.134496,-0.091172,-0.472862}, {-0.297389,0.356289,-0.186059}, {0.248273,0.433804,-0.0132384}, {0.233039,-0.310684,-0.31491}, {0.155887,-0.0095059,0.474983}, {-0.191135,-0.429776,-0.169588}, {0.455006,-0.059264,0.19864}, {0.104928,0.465462,-0.14945}, {0.107383,0.082055,0.48139}, {-0.367416,-0.123541,-0.315822}, {0.305592,0.142328,-0.369264}, {-0.408392,-0.0490252,-0.284275}, {-0.133005,0.210788,-0.433449}, {-0.331678,0.0721085,0.367137}, {-0.09246,-0.483622,0.086953}, {-0.383499,-0.290579,0.135985}, {-0.233763,-0.388027,0.211637}, {0.334402,-0.0934975,0.359769}, {-0.215314,0.293899,0.342437}, {0.41899,-0.172843,-0.211123}, {0.00194483,-0.420164,-0.271033}, {0.280761,-0.185248,-0.369941}, {-0.219401,-0.171746,0.41517}, {0.272247,-0.286154,-0.306591}, {0.064775,-0.311775,0.385487}, {-0.48126,-0.134534,-0.0169983}, {0.276178,-0.395953,0.13018}, {-0.0950385,0.490548,0.0181918}, {0.377449,-0.0573505,0.322867}, {-0.0446227,0.188822,0.46082}, {0.159612,0.472685,-0.0330524}, {0.351036,-0.186297,0.303426}, {-0.261101,0.320885,-0.280819}, {-0.13801,0.348183,0.331242}, {-0.398211,0.116246,0.279132}, {0.440798,0.185896,0.145397}, {-0.111251,-0.270425,0.405577}, {-0.222485,0.446893,0.0280553}, {-0.0055881,-0.218171,0.449856}, {-0.420602,-0.111516,-0.246289}, {-0.297508,0.2131,-0.340701}, {0.175609,0.461923,0.0760835}, {-0.466792,0.175149,-0.037782}, {-0.0475532,0.346819,0.357009}, {0.020978,-0.112397,-0.486752}, {0.43958,-0.0261594,-0.236823}, {-0.365381,0.016682,-0.340909}, {-0.488274,0.0806815,-0.071265}, {0.0692945,0.377354,0.320628}, {-0.350191,-0.106058,0.340761}, {-0.205244,-0.433764,0.140439}, {-0.313418,0.0359833,-0.38791}, {-0.229214,-0.269155,-0.353576}, {0.306997,-0.0487826,0.391628}, {0.0436436,-0.230731,-0.441428}, {0.453105,0.103626,-0.184274}, {-0.0403198,-0.448265,-0.21779}, {0.0793955,-0.141585,0.472917}, {-0.165855,-0.377366,-0.282996}, {0.0653215,-0.455439,0.195727}, {-0.224268,0.446491,-0.0186975}, {-0.434939,-0.17262,-0.176154}, {-0.121438,-0.299909,-0.381192}, {0.366385,0.147804,-0.306459}, {0.0144606,-0.468521,0.174007}, {-0.10467,-0.235217,-0.428622}, {0.494037,-0.051296,0.057413}, {0.309951,0.202324,0.336148}, {-0.307307,-0.393338,0.0291137}, {0.212974,-0.303671,0.335299}, {0.462342,-0.186591,0.0377236}, {0.177677,0.461804,-0.071889}, {0.299337,0.0961855,-0.388775}, {-0.44918,-0.076775,-0.205772}, {-0.422279,-0.265083,0.0375723}, {-0.106639,-0.458221,0.169298}, {-0.15761,0.079249,-0.467845}, {-0.274705,-0.394361,-0.1379}, {-0.109184,0.073484,0.482368}, {0.244004,-0.226806,0.372855}, {-0.416297,0.276722,-0.0110374}, {0.00266067,-0.29143,-0.406278}, {0.0278229,0.127553,0.482655}, {-0.330294,-0.328481,0.181676}, {0.0110779,0.495511,0.065927}, {-0.144718,-0.475665,-0.0529115}, {-0.127821,-0.140394,0.462549}, {-0.433306,-0.178886,0.173912}, {0.429003,0.228126,0.11796}, {-0.0547805,0.46699,-0.170056}, {-0.468389,-0.169882,-0.0418569}, {0.112522,0.339672,0.34923}, {-0.413023,-0.17492,0.22094}, {-0.252835,-0.358057,-0.240561}, {-0.445947,0.221569,0.0451435}, {0.141357,-0.466732,-0.110364}, {0.195881,0.127152,0.442112}, {-0.0993195,0.263325,-0.413274}, {0.305149,-0.095076,-0.384506}, {0.284388,-0.402121,0.086151}, {0.345859,-0.354135,-0.0705}, {-0.488838,-0.081942,0.065749}, {-0.276079,0.38997,-0.147323}, {0.0112705,0.362586,0.344099}, {-0.351011,-0.232142,0.270004}, {-0.459043,-0.143038,0.137187}, {0.0283185,-0.058563,-0.49575}, {-0.0183305,0.106383,0.488208}, {-0.119459,0.449715,0.18299}, {0.0638995,0.149592,-0.472799}, {0.0425747,0.468243,-0.170105}, {0.0337233,-0.362936,-0.342258}, {-0.140396,-0.474785,0.069772}, {-0.191897,0.439766,-0.140645}, {-0.337409,0.356857,0.093856}, {0.406008,-0.278597,0.0868395}, {-0.118085,-0.390719,-0.28878}, {-0.295339,-0.229021,-0.332151}, {0.405268,0.0307629,0.291224}, {0.083624,0.262363,0.41734}, {-0.340349,0.365389,0.0255544}, {0.31613,-0.00133591,-0.387376}, {-0.296232,0.114658,-0.386134}, {0.470019,0.0385528,0.16612}, {0.313617,0.15221,0.358436}, {-0.15878,-0.0166927,0.473825}, {0.262784,-0.0434751,0.423148}, {-0.436255,-0.244088,-0.0101233}, {-0.303148,0.38703,0.0911585}, {-0.306395,0.392381,0.0464784}, {-0.198566,0.063463,0.454471}, {-0.175142,0.1325,-0.449187}, {0.0517635,-0.267711,-0.419108}, {0.349432,-0.318165,-0.163304}, {0.385286,0.318503,-0.0104925}, {0.397535,-0.272809,-0.132441}, {0.0758025,0.310391,0.384593}, {0.488283,-0.101177,-0.036645}, {-0.45548,0.206154,-0.0061688}, {0.125177,-0.350539,0.333846}, {0.186376,-0.447269,-0.123346}, {0.229305,0.0428223,0.44225}, {0.248051,0.40173,0.164572}, {-0.0170409,-0.0720815,-0.494484}, {-0.42994,0.215287,-0.137125}, {0.184259,-0.335726,-0.32146}, {0.214753,0.450376,0.0322926}, {0.161349,0.103392,0.461819}, {0.444822,0.220446,-0.059471}, {0.178662,0.05273,-0.464004}, {-0.244891,0.345765,0.265472}, {-0.485978,-0.0542925,0.104299}, {-0.216566,0.160137,-0.421254}, {-0.156007,-0.428857,-0.204312}, {0.323291,-0.204772,-0.321794}, {-0.180569,-0.458417,0.0851355}, {0.121953,0.431946,-0.220341}, {-0.336419,0.233855,-0.286591}, {-0.401236,0.206416,0.215411}, {-0.411291,-0.2712,0.0853825}, {-0.319042,-0.250889,-0.292005}, {0.119426,0.0789525,-0.479065}, {-0.0429761,-0.297399,-0.399633}, {0.484901,0.0229615,0.119763}, {0.483086,0.1251,-0.0312685}, {-0.073444,-0.47863,-0.124576}, {0.489352,-0.0960985,0.0360534}, {-0.418911,0.0802995,0.260893}, {-0.406813,-0.212282,-0.198594}, {-0.0495785,-0.425802,0.257361}, {0.10062,-0.187018,0.452659}, {-0.118392,-0.485517,-0.0160276}, {0.178514,0.138788,-0.445949}, {-0.2983,0.291126,-0.276157}, {0.165517,0.416641,0.221393}, {0.43859,-0.122817,-0.206289}, {-0.214574,0.143451,0.428229}, {0.195465,-0.27574,0.368457}, {-0.462831,-0.169988,0.0830165}, {-0.442194,0.052235,0.227457}, {0.469363,-0.074776,0.155263}, {-0.231037,-0.443297,0.0104653}, {-0.260947,-0.424418,0.0421441}, {0.0756115,-0.085267,0.486839}, {-0.390979,-0.279738,-0.137411}, {-0.180759,0.222827,-0.409481}, {0.439226,-0.0821925,-0.224332}, {0.372414,0.258431,-0.211002}, {-0.0345729,-0.0619035,0.494947}, {0.195062,-0.299169,-0.349926}, {0.273967,-0.229087,-0.349944}, {0.198482,-0.209617,-0.408247}, {-0.443184,-0.133355,0.18922}, {0.0434247,-0.149113,-0.475268}, {0.207605,0.409237,0.198557}, {0.441857,-0.0300819,0.232071}, {-0.427927,-0.120084,0.229038}, {0.38028,-0.30423,-0.113275}, {0.081558,-0.116074,-0.479453}, {-0.459569,-0.0914845,0.174433}, {0.0238033,-0.494171,0.072305}, {-0.253655,0.053638,0.42753}, {-0.143989,-0.478262,0.0230879}, {0.194606,-0.0547885,0.457304}, {-0.0101928,-0.404075,0.294312}, {-0.475628,-0.0488277,0.146266}, {-0.129022,-0.473884,-0.093744}, {-0.235192,0.0927945,0.431362}, {-0.260902,0.134324,0.40483}, {-0.122811,-0.463745,-0.140922}, {-0.222209,-0.121303,-0.431171}, {0.131322,0.168267,0.452151}, {0.0721515,0.171704,0.464017}, {0.00494469,-0.168155,0.47085}, {0.052102,0.462512,0.182671}, {0.107466,0.44969,0.190341}, {0.243327,-0.156677,-0.407731}, {0.213758,-0.251549,-0.37554}, {0.10005,-0.428696,0.237085}, {0.0086751,0.107524,-0.488225}, {0.0371978,0.187002,-0.462219}, {-0.035679,0.168905,-0.469253}, {0.000403462,0.215911,-0.450979}, {-0.0411792,0.110157,-0.485973}, {0.05357,0.105632,-0.48577}, {0.0140034,0.391402,0.310818}, {0.321859,-0.375488,0.0735915}, {0.303768,-0.368154,0.148954}, {0.364333,-0.321207,0.118693}, {0.37785,-0.319541,0.0715785}, {-0.0563305,0.0794565,0.490422}, {-0.138852,0.133136,0.461514}, {-0.0485798,0.142431,0.476816}, {0.389126,-0.208332,-0.234901}, {0.412311,-0.133587,-0.249306}, {0.127022,-0.483577,-0.00432928}, {0.219657,-0.448211,0.0292938}, {0.179306,-0.460816,-0.0741475}, {0.324271,-0.378274,-0.0419176}, {-0.456913,-0.154591,-0.131652}, {-0.482675,-0.112552,-0.0660045}, {-0.144348,0.464469,-0.115894}, {-0.231439,0.426926,-0.119037}, {-0.122764,0.478229,-0.0789045}, {0.289042,-0.35315,-0.204302}, {0.288928,-0.321548,-0.251252}, {0.253344,-0.33386,-0.27268}, {0.498017,0.0134465,0.0424055}, {0.498055,-0.00308028,-0.043958}, {0.49393,0.073373,-0.0254903}, {0.110907,-0.481222,-0.0782575}, {-0.285174,0.369544,0.1792}, {-0.336323,0.318135,0.188884}, {-0.272043,0.395017,0.14126}, {-0.416144,-0.275205,-0.0329509}, {-0.148483,0.0348152,-0.476172}, {-0.175549,-0.0458372,-0.46592}, {-0.24635,0.0369357,-0.433529}, {0.329819,0.171366,-0.334445}, {0.47298,0.153201,0.053097}, {0.489446,0.100834,0.016591}, {0.46481,0.150435,0.1064}, {-0.398654,-0.00364812,0.301764}, {-0.36849,0.0423784,0.33529}, {0.095057,0.0226625,-0.490358}, {0.122979,-0.0767185,-0.478529}, {0.139344,0.0309488,-0.479193}, {0.15231,-0.0181332,-0.475892}, {0.069514,-0.0732195,-0.4897}, {-0.364365,-0.238669,-0.24551}, {-0.390767,-0.246805,-0.190756}, {0.249733,-0.433161,-0.0021812}, {0.260887,-0.423249,-0.052903}, {0.292982,-0.397053,-0.0806895}, {0.405668,0.0425353,-0.289179}, {0.434949,0.0569775,-0.239943}, {0.200672,0.281227,-0.361445}, {-0.178634,0.463637,0.0559595}, {-0.262187,0.422111,0.0554975}, {0.092693,-0.33335,0.360951}, {0.0240245,-0.342645,0.363341}, {-0.0054252,-0.368557,0.337841}, {-0.257397,-0.180805,0.38866}, {-0.32907,-0.196891,0.320854}, {-0.175277,-0.107023,-0.455877}, {-0.253456,-0.0911985,-0.421239}, {-0.25298,-0.0435092,-0.429078}, {0.0431988,-0.497347,0.0279154}, {-0.0861585,0.250482,0.42407}, {0.00200888,0.231706,0.443068}, {-0.000340489,0.274236,0.418085}, {-0.105869,0.461187,-0.161549}, {-0.0133558,0.448599,-0.220411}, {-0.082125,0.422187,-0.254976}, {0.245191,-0.262656,0.347697}, {0.270662,-0.284012,0.309966}, {0.31602,-0.260647,0.286695}, {0.225123,-0.106428,-0.433581}, {0.281681,-0.139544,-0.388823}, {0.288128,-0.0485394,-0.405741}, {-0.482407,-0.0236676,-0.129319}, {-0.46378,-0.037645,-0.183004}, {0.281338,0.391309,-0.133137}, {0.237266,0.415283,-0.145757}, {0.219091,0.409217,-0.18585}, {0.248781,0.374225,-0.219234}, {0.486361,0.0191803,-0.114389}, {0.46174,0.0607535,-0.181947}, {0.359569,0.34685,0.0201338}, {0.302117,0.394002,0.059054}, {-0.267265,0.420772,-0.0389974}, {-0.301301,0.394215,-0.061746}, {0.266545,-0.129063,0.402861}, {0.206079,-0.189639,0.414208}, {0.211606,-0.0945925,0.44303}, {0.496123,-0.0621415,0.00072179}, {0.0868995,0.208277,0.446172}, {0.0273255,0.195304,0.459466}, {0.407513,0.253482,-0.140287}, {0.398259,0.289428,-0.0872975}, {0.461244,-0.0686685,-0.180385}, {0.136657,0.476322,0.0666485}, {-0.240185,0.405458,-0.167076}, {-0.159388,0.423434,-0.212836}, {-0.259047,0.374655,-0.206226}, {-0.368422,-0.332823,-0.0591055}, {-0.414044,-0.168718,-0.223835}, {0.320246,0.377126,-0.072239}, {0.276744,0.413206,-0.0517115}, {-0.355212,-0.325611,0.133423}, {-0.309837,-0.379634,0.0993915}, {-0.317752,-0.358893,0.142231}, {0.343728,0.330754,-0.149843}, {-0.0818475,-0.139664,0.473069}, {-0.073376,-0.0271489,0.493841}, {-0.113908,-0.0370352,0.485442}, {-0.0360586,-0.149338,0.475813}, {-0.0190587,-0.107945,0.487837}, {-0.487766,0.100772,0.043927}, {-0.499045,0.0152984,0.0268319}, {-0.484166,0.079967,0.095854}, {-0.496175,0.0551175,0.0277882}, {-0.372423,0.282993,0.176681}, {-0.39709,0.246104,0.178193}, {-0.413925,0.27045,0.074318}, {-0.065562,-0.0646855,-0.491444}, {-0.0798415,0.303631,0.389145}, {0.316375,0.337296,0.190102}, {0.323136,0.369237,0.0961635}, {0.351758,0.331198,0.128742}, {-0.25307,-0.424161,-0.077738}, {-0.32547,-0.363221,-0.11018}, {-0.298644,-0.398325,-0.0463573}, {0.342326,0.073365,-0.356974}, {0.276819,0.00210989,-0.416374}, {0.330171,0.314141,-0.205676}, {0.270983,0.336363,-0.25185}, {0.42077,-0.064439,-0.262299}, {0.265625,0.143748,-0.398472}, {0.260929,-0.383834,0.185976}, {0.306474,-0.323781,0.226363}, {0.259119,-0.339342,0.260201}, {-0.209762,-0.308789,-0.332639}, {0.291075,0.272618,-0.301587}, {0.243325,-0.430836,0.071925}, {-0.00478953,-0.251464,-0.432138}, {-0.071949,-0.20461,-0.450509}, {-0.0819945,-0.280122,-0.405966}, {-0.193925,0.397374,0.233425}, {-0.225565,0.323062,0.307818}, {-0.151489,0.378922,0.288909}, {0.367634,0.333353,-0.060996}, {-0.233377,0.393704,0.201326}, {-0.363251,0.274703,-0.206367}, {-0.32435,0.349152,-0.151294}, {-0.356528,0.328265,-0.123004}, {0.061272,0.352129,-0.349643}, {0.0701665,0.317733,-0.379634}, {0.167174,0.304478,-0.359646}, {0.169928,0.339928,-0.324921}, {-0.00735255,-0.43383,0.24847}, {0.309693,0.289832,0.264742}, {0.276072,0.349141,0.227782}, {0.445547,-0.151982,-0.168491}, {0.466143,-0.135648,-0.119628}, {-0.226804,-0.274778,0.350795}, {-0.27385,-0.256526,0.330454}, {-0.211228,-0.221915,0.395141}, {0.409841,-0.285971,-0.0158411}, {0.381404,-0.316787,-0.0646305}, {0.325309,0.0406243,0.377523}, {0.347032,0.121693,0.338762}, {0.367169,0.0424471,0.336728}, {0.39118,0.234778,0.204592}, {0.344286,0.206438,0.298077}, {0.358947,0.246157,0.246096}, {0.385252,0.159138,0.276144}, {-0.390499,0.293768,-0.105879}, {-0.366501,0.339851,-0.0133546}, {-0.336753,0.367771,-0.0366266}, {-0.241328,-0.403365,0.170462}, {-0.447825,-0.217515,-0.0462596}, {-0.413459,-0.243714,-0.140196}, {-0.445849,-0.205758,-0.0942475}, {-0.27456,0.205269,0.36398}, {-0.296287,0.232454,0.328906}, {-0.192244,0.268692,0.375296}, {-0.371919,-0.0299017,-0.33284}, {-0.332551,-0.00514435,-0.37334}, {-0.33779,-0.0956155,-0.356027}, {-0.452437,0.100077,0.187844}, {-0.432758,0.11277,0.223614}, {-0.426584,0.17207,0.196004}, {-0.132627,0.480917,-0.0336017}, {-0.137662,0.480173,0.0219728}, {-0.380943,-0.0744305,0.315186}, {-0.376408,-0.12984,0.30242}, {0.448433,-0.11285,0.190193}, {0.431943,-0.094985,0.233245}, {0.447008,-0.15713,0.159669}, {0.424796,-0.227942,-0.132629}, {0.396444,-0.246287,-0.179374}, {-0.231106,-0.338495,0.286376}, {-0.244885,-0.361513,0.243598}, {0.470846,0.106336,-0.130372}, {0.466077,0.176719,-0.0392789}, {0.458083,0.151362,-0.131338}, {0.486992,0.0836335,-0.0764455}, {0.379832,0.306225,0.109333}, {0.383596,0.315624,0.0568815}, {0.374987,0.0557725,-0.325997}, {-0.396029,0.252308,-0.171762}, {-0.185636,-0.357229,0.296524}, {-0.173487,-0.324253,0.338765}, {-0.111708,-0.314353,0.37243}, {-0.155556,-0.295478,0.372149}, {-0.0841415,-0.34702,0.349996}, {-0.449071,0.045928,-0.215002}, {-0.48794,0.0390612,-0.101928}, {-0.447207,0.089875,-0.204766}, {-0.461713,0.109012,-0.157914}, {0.0490187,-0.182636,0.462862}, {0.0253039,-0.082767,0.492453}, {0.0828345,0.378196,-0.316395}, {0.0475916,0.404195,-0.290451}, {0.078236,0.427863,-0.246602}, {0.150132,0.406302,-0.249758}, {-0.0168242,0.438542,0.239578}, {0.00555075,0.476063,0.152753}, {-0.275761,0.0124897,-0.416893}, {0.433905,0.239294,0.0668195}, {0.456045,0.201088,0.0398443}, {0.322645,0.366067,-0.109062}, {0.413714,0.224299,0.168911}, {0.403166,0.266454,0.128296}, {-0.419307,0.178685,-0.205556}, {-0.441306,0.179359,-0.151918}, {-0.137566,-0.259906,-0.404382}, {-0.399373,0.104186,-0.282218}, {-0.423579,0.122296,-0.235849}, {-0.234793,-0.416096,-0.147429}, {-0.282963,-0.359011,-0.202591}, {-0.303997,-0.364343,-0.15761}, {-0.118744,0.297419,-0.383981}, {0.105482,-0.154632,-0.463641}, {0.056574,0.406813,0.285136}, {0.021902,0.074773,0.493892}, {0.0230871,0.015965,0.499211}, {-0.068845,0.0315165,0.494234}, {-0.0343799,-0.264622,0.422839}, {-0.430623,-0.0246033,0.252901}, {0.436291,-0.196868,0.144545}, {0.386331,-0.28707,0.135422}, {0.379361,-0.273151,0.177407}, {0.396174,-0.226974,0.203787}, {-0.193009,-0.421453,0.187415}, {-0.060845,0.442646,0.224414}, {-0.103383,0.392313,0.292237}, {0.414129,0.191331,-0.20467}, {0.35049,0.241312,-0.262537}, {0.401857,0.23692,-0.179945}, {-0.330735,-0.142625,-0.346803}, {-0.346187,-0.212963,-0.291207}, {0.239075,0.297427,0.32308}, {0.274452,0.292629,0.298403}, {0.191314,0.337966,0.314925}, {0.212328,0.371399,0.258805}, {-0.159234,0.313159,-0.355776}, {-0.122183,-0.469071,0.12265}, {-0.070399,-0.245374,0.429925}, {-0.075888,0.460258,0.18001}, {0.252119,0.428995,0.0489887}, {0.419857,0.106716,0.249663}, {0.445858,0.121467,0.190936}, {0.469797,0.078252,0.152208}, {0.454454,0.0175477,0.207757}, {-0.127464,0.118444,-0.468747}, {-0.0786745,0.176588,-0.461115}, {0.446732,-0.217195,-0.0570655}, {-0.484403,-0.121246,0.0255725}, {-0.460718,-0.194252,0.00230012}, {-0.445738,-0.222731,0.0413388}, {-0.49531,-0.0385216,-0.056432}, {-0.492175,-0.0811495,-0.0343368}, {-0.496551,-0.0332083,0.0483121}, {-0.494273,-0.074333,0.0130054}, {0.181524,0.246584,0.39528}, {0.227323,0.17251,0.410567}, {0.334973,0.280158,-0.243525}, {0.376594,-0.210793,0.252474}, {0.100709,-0.464415,0.155486}, {0.0090592,-0.486735,0.114047}, {-0.0430767,0.329946,-0.373202}, {-0.0668075,0.361909,-0.338465}, {-0.052492,0.395456,-0.301428}, {-0.053857,0.478442,0.134882}, {0.205663,-0.355561,0.285095}, {-0.132875,0.461413,0.139436}, {-0.215314,-0.117032,0.435825}, {-0.183181,-0.155406,0.438513}, {-0.360276,-0.174655,0.299494}, {-0.261302,-0.0887765,0.416941}, {-0.205068,-0.0275785,0.455178}, {-0.29057,-0.0495298,0.403876}, {-0.251383,0.083447,-0.424079}, {-0.453929,-0.057254,0.201668}, {0.441211,-0.224915,0.0688865}, {0.466817,-0.150276,0.0974625}, {-0.159548,-0.406629,-0.243306}, {-0.206411,-0.382716,-0.246826}, {-0.267983,0.299137,0.29783}, {0.190102,0.0136957,0.462249}, {0.109599,0.00971065,0.487743}, {-0.356767,0.196129,-0.29026}, {0.222126,0.254232,0.368817}, {0.472795,-0.145351,-0.073062}, {0.458664,-0.198954,-0.0066567}, {0.482934,-0.129505,-0.0016833}, {0.217483,0.436423,-0.110616}, {0.327522,-0.050457,-0.37441}, {0.0162706,0.328799,0.376333}, {-0.325037,-0.0198997,0.379414}, {-0.342096,0.0199359,0.364106}, {-0.361653,0.341035,0.0538745}, {0.159663,-0.25842,0.397149}, {-0.0223898,-0.023061,-0.498966}, {0.369298,-0.284873,-0.180185}, {-0.228108,-0.23061,-0.380507}, {0.231571,0.00150044,-0.44314}, {0.20803,-0.0555625,-0.451261}, {0.225377,0.145952,-0.421786}, {-0.279992,-0.406828,0.0780725}, {0.206576,-0.412543,-0.192703}, {0.17218,-0.410348,-0.227966}, {-0.103055,0.01777,-0.488941}, {0.276644,0.198683,0.366051}, {0.276083,0.15204,0.388152}, {-0.0261207,-0.493691,-0.074746}, {-0.0603665,-0.496081,0.0161152}, {-0.0292361,-0.498318,-0.0287137}, {-0.387321,-0.26634,0.170427}, {-0.372024,-0.251945,0.219365}, {0.389439,-0.31312,0.0171261}, {0.102099,-0.0425931,0.487608}, {0.115015,-0.106972,0.474688}, {-0.459073,0.178937,0.0850495}, {0.135944,0.154374,-0.455728}, {0.215686,0.0991885,-0.440047}, {0.107009,0.118664,-0.47378}, {-0.142561,0.217912,0.426838}, {-0.171175,0.241887,0.402728}, {-0.155134,0.169934,0.44391}, {-0.0476224,0.492343,0.0730065}, {0.182384,-0.429212,0.180314}, {0.190235,-0.443607,0.130474}, {0.233819,-0.425362,0.119982}, {-0.0853355,-0.439648,-0.222324}, {-0.121236,-0.448165,-0.185608}, {0.124403,0.226949,-0.427806}, {-0.1014,0.328599,0.362961}, {-0.181366,-0.456598,-0.0928695}, {-0.168673,-0.449846,-0.138522}, {-0.336892,0.24949,0.272506}, {-0.339108,0.160846,0.330354}, {-0.459403,0.178153,-0.0849125}, {-0.0489627,-0.389816,0.309267}, {0.158121,-0.165904,-0.444381}, {0.300965,-0.00659795,0.39922}, {0.270836,0.0831,0.411998}, {0.2568,0.0082922,0.428935}, {0.0999495,-0.353965,-0.338702}, {0.0475254,-0.41601,-0.273271}, {0.102068,-0.388809,-0.297337}, {0.06593,0.0718545,0.490398}, {0.122094,0.129363,0.467288}, {-0.0774555,-0.367453,-0.33012}, {-0.094023,-0.329634,-0.364007}, {0.118926,0.295437,0.385452}, {0.118329,0.373703,0.310394}, {0.0557705,0.490409,0.0799325}, {0.167142,0.456512,-0.116879}, {0.141049,0.447033,-0.173974}, {0.175048,0.419105,-0.209067}, {-0.280662,-0.324166,-0.257188}, {-0.285233,-0.289782,-0.290978}, {-0.399608,-0.098771,-0.283826}, {0.123466,0.472326,-0.108005}, {0.118807,0.48156,-0.063125}, {0.021382,0.492204,-0.0853105}, {-0.44296,0.215435,-0.0858735}, {-0.373827,-0.315808,-0.10256}, {-0.495816,0.055011,-0.0337647}, {-0.267906,-0.209745,-0.36638}, {0.255638,0.309881,-0.297696}, {0.160808,-0.31431,0.354048}, {0.011555,0.498973,-0.0298718}, {-0.0447383,0.497222,0.0277237}, {-0.00610595,0.466964,-0.178625}, {-0.338483,-0.276527,0.24282}, {-0.27274,0.161026,-0.386889}, {0.00946325,-0.266296,0.423081}, {0.0051572,0.482816,-0.129854}, {-0.467845,-0.009612,0.17615}, {-0.460908,0.0476207,0.187872}, {0.225652,-0.438005,-0.085046}, {0.154761,-0.465614,0.09619}, {-0.0709,-0.403869,-0.286116}, {-0.0424997,-0.422635,-0.263767}, {-0.00945355,-0.348311,-0.358594}, {-0.0467033,0.264717,-0.421597}, {0.330987,-0.152576,0.342299}, {0.0881845,0.490391,0.0417172}, {0.177754,0.467015,0.0173376}, {0.102271,0.489055,-0.0191337}, {0.408147,0.0734205,0.27933}, {-0.218821,0.322794,-0.312924}, {-0.263041,0.283562,-0.316862}, {0.0466377,-0.444063,-0.225019}, {-0.293971,-0.170523,-0.366746}, {-0.0611355,0.300517,-0.394908}, {-0.119136,0.358809,-0.327205}, {-0.346851,0.0577415,-0.355471}, {-0.332701,-0.372883,-0.0163784}, {-0.200297,0.358583,-0.285131}, {-0.12964,0.409928,-0.25525}, {0.40126,0.136942,-0.265023}, {0.340453,0.118072,-0.346627}, {-0.385298,0.177268,0.264804}, {-0.375017,0.232747,0.234927}, {-0.480992,0.130466,-0.0403119}, {-0.478739,0.14385,0.0107607}, {0.10723,0.403666,0.274873}, {0.167925,0.393726,0.25842}, {0.392257,-0.094046,0.295449}, {0.404828,-0.0278234,0.29213}, {0.176862,-0.342794,0.318138}, {0.0100564,-0.0359959,0.498601}, {0.373856,-0.150776,0.295801}, {-0.112479,-0.221816,0.433758}, {-0.334346,0.271425,-0.254049}, {0.088213,-0.229746,0.435241}, {-0.428752,0.255999,0.0252302}, {0.48333,-0.0671145,0.109034}, {-0.314462,0.152783,-0.357451}, {-0.222778,0.200204,-0.400361}, {-0.355453,0.12954,-0.326914}, {0.406409,0.290495,0.0210681}, {0.454818,0.207665,-0.00401806}, {0.130317,0.251611,0.411957}, {0.420886,0.150679,-0.223945}, {-0.0247929,-0.496149,0.05676}, {0.224995,0.085842,0.438188}, {0.130826,-0.407209,-0.25897}, {0.142282,-0.327635,-0.349873}, {0.209751,0.453116,-0.0262734}, {0.091608,-0.472517,-0.135408}, {0.062035,-0.465853,-0.170683}, {0.481517,-0.0232863,0.132662}, {0.347465,-0.339989,-0.116942}, {-0.0240471,-0.485027,-0.119041}, {0.301069,-0.24468,-0.31542}, {-0.140318,-0.183048,0.443626}, {0.348438,-0.0453235,0.35572}, {0.380237,-0.000378857,0.324684}, {-0.261703,-0.144506,-0.400786}, {0.114693,-0.263993,-0.408844}, {0.161946,-0.240169,-0.407544}, {0.186052,0.37221,-0.277209}, {-0.402336,-0.00675235,-0.296783}, {-0.435754,-0.011167,-0.244936}, {-0.079559,-0.441693,0.220403}, {-0.0247677,-0.478768,0.142015}, {-0.069597,-0.474593,0.14113}, {-0.306969,0.126571,0.373831}, {-0.433161,-0.0577815,-0.242966}, {-0.269625,0.420967,0.0094432}, {0.0725925,-0.29777,-0.395049}, {0.07114,-0.335122,-0.364187}, {-0.154879,0.0308747,0.474404}, {-0.301044,0.249543,-0.311611}, {-0.120492,-0.179451,-0.450865}, {-0.169675,0.316569,0.347843}, {-0.301721,0.0476813,0.395842}, {0.119627,-0.442995,0.198606}, {0.114701,-0.474807,0.106782}, {-0.472911,-0.103087,0.125412}, {0.19338,0.432542,0.159721}, {-0.316704,-0.240474,0.303102}, {0.475507,-0.148034,0.0444874}, {-0.221596,-0.44241,0.0718955}, {-0.422661,0.214743,0.158881}, {-0.440333,0.23444,-0.0338239}, {-0.169975,0.177845,-0.435293}, {-0.419181,-0.24423,0.120991}, {0.221544,-0.0229057,0.447653}, {-0.205625,0.0202464,0.455311}, {-0.441793,-0.218557,0.0839745}, {-0.180208,-0.466114,-0.0162005}, {0.243157,-0.267412,-0.345493}, {0.46784,-0.0158771,0.175708}, {-0.284649,0.0918795,0.400666}, {4.31089e-05,0.165152,0.471938}, {0.062356,0.347253,0.354297}, {-0.19198,-0.460647,0.0307916}};
	const int unitSphereFaces[][3] = {{28,342,0}, {160,28,0}, {204,517,0}, {342,204,0}, {517,160,0}, {331,1,48}, {243,790,1}, {331,243,1}, {790,791,1}, {791,48,1}, {2,469,792}, {766,2,597}, {766,793,2}, {469,2,767}, {792,597,2}, {793,767,2}, {405,3,23}, {384,3,115}, {219,794,3}, {3,384,23}, {3,405,219}, {794,115,3}, {799,4,795}, {685,4,796}, {798,4,797}, {4,798,796}, {4,799,797}, {800,4,685}, {4,800,795}, {230,619,5}, {359,230,5}, {5,409,359}, {619,679,5}, {801,5,679}, {409,5,801}, {510,6,802}, {6,215,803}, {805,6,510}, {6,602,675}, {6,675,802}, {602,6,803}, {215,6,804}, {6,805,804}, {265,807,7}, {806,7,654}, {806,808,7}, {807,654,7}, {808,265,7}, {8,113,268}, {8,162,810}, {8,225,162}, {225,8,268}, {8,595,809}, {809,113,8}, {595,8,810}, {813,9,264}, {275,812,9}, {283,811,9}, {9,811,264}, {812,283,9}, {9,813,275}, {814,10,25}, {70,25,10}, {675,10,802}, {510,10,347}, {10,510,802}, {10,675,70}, {10,814,347}, {105,11,37}, {815,11,105}, {11,136,816}, {430,37,11}, {136,11,815}, {816,430,11}, {229,254,12}, {819,12,254}, {817,689,12}, {818,12,689}, {12,818,229}, {12,819,817}, {251,13,203}, {13,251,822}, {280,203,13}, {820,280,13}, {13,821,820}, {821,13,822}, {14,139,824}, {14,236,825}, {14,568,823}, {14,823,139}, {236,14,824}, {825,568,14}, {32,210,15}, {432,15,210}, {15,264,811}, {15,432,499}, {15,811,32}, {15,499,826}, {264,15,826}, {828,16,84}, {16,216,827}, {829,16,827}, {216,16,828}, {690,84,16}, {16,704,690}, {829,704,16}, {326,17,50}, {525,17,114}, {17,326,114}, {735,17,525}, {650,830,17}, {17,735,650}, {17,830,50}, {833,18,111}, {165,831,18}, {832,18,245}, {245,18,831}, {832,111,18}, {18,833,165}, {66,69,19}, {69,434,19}, {439,19,434}, {19,439,834}, {834,66,19}, {20,200,568}, {200,20,201}, {20,568,836}, {837,20,835}, {836,835,20}, {20,837,201}, {610,21,327}, {387,838,21}, {838,839,21}, {21,610,743}, {743,387,21}, {839,327,21}, {840,22,511}, {842,22,840}, {844,22,841}, {22,842,843}, {843,841,22}, {844,511,22}, {405,23,29}, {23,384,715}, {715,29,23}, {63,846,24}, {573,24,248}, {24,388,63}, {388,24,573}, {845,248,24}, {846,845,24}, {25,70,847}, {25,847,848}, {849,25,848}, {25,849,814}, {100,26,65}, {26,100,850}, {26,159,621}, {850,851,26}, {621,65,26}, {159,26,851}, {42,852,27}, {27,66,199}, {69,66,27}, {199,42,27}, {27,292,69}, {292,27,852}, {28,160,853}, {853,613,28}, {854,28,613}, {342,28,854}, {29,715,855}, {855,856,29}, {857,29,856}, {29,857,405}, {30,368,491}, {491,561,30}, {534,858,30}, {859,30,561}, {368,30,858}, {30,859,534}, {111,832,31}, {832,860,31}, {786,31,860}, {861,31,786}, {31,861,862}, {111,31,862}, {32,458,863}, {543,32,811}, {32,543,458}, {210,32,863}, {33,59,864}, {246,605,33}, {605,865,33}, {246,33,864}, {866,33,865}, {33,866,59}, {34,455,869}, {867,34,540}, {665,868,34}, {34,867,665}, {455,34,868}, {869,540,34}, {351,35,287}, {655,35,351}, {35,655,870}, {35,870,871}, {871,872,35}, {872,287,35}, {875,36,197}, {874,36,674}, {873,197,36}, {792,36,874}, {36,875,674}, {36,792,873}, {876,877,37}, {37,430,876}, {649,105,37}, {649,37,877}, {64,881,38}, {64,38,126}, {878,126,38}, {38,879,878}, {879,38,880}, {880,38,881}, {47,416,39}, {39,159,883}, {159,39,416}, {39,549,882}, {882,47,39}, {549,39,883}, {145,40,52}, {40,73,52}, {40,99,884}, {40,145,885}, {73,40,884}, {885,99,40}, {229,818,41}, {41,435,887}, {41,678,435}, {229,41,886}, {818,678,41}, {886,41,887}, {42,199,556}, {42,336,507}, {507,852,42}, {336,42,556}, {330,889,43}, {43,468,888}, {552,43,888}, {43,552,890}, {468,43,889}, {890,330,43}, {44,139,891}, {824,44,198}, {44,467,198}, {467,44,528}, {528,44,713}, {139,44,824}, {891,713,44}, {866,45,539}, {539,45,695}, {893,45,865}, {892,695,45}, {893,892,45}, {45,866,865}, {170,894,46}, {46,344,724}, {46,420,344}, {724,170,46}, {894,895,46}, {420,46,895}, {47,198,467}, {467,896,47}, {198,47,882}, {47,896,416}, {492,897,48}, {520,492,48}, {48,897,331}, {48,791,520}, {899,49,348}, {474,900,49}, {494,474,49}, {49,898,348}, {49,899,494}, {898,49,900}, {50,521,901}, {521,50,830}, {901,326,50}, {105,649,51}, {615,902,51}, {637,105,51}, {649,615,51}, {637,51,902}, {73,903,52}, {903,904,52}, {904,145,52}, {114,299,53}, {299,906,53}, {114,53,525}, {905,525,53}, {907,53,906}, {53,907,905}, {116,895,54}, {908,54,153}, {54,908,116}, {894,153,54}, {895,894,54}, {87,909,55}, {912,55,909}, {911,55,910}, {910,55,764}, {55,911,87}, {55,912,913}, {55,913,764}, {444,56,378}, {515,56,444}, {56,515,916}, {56,914,917}, {56,915,378}, {914,56,916}, {915,56,917}, {57,84,188}, {188,920,57}, {919,57,538}, {918,84,57}, {57,919,918}, {57,920,538}, {245,442,58}, {58,350,576}, {58,442,921}, {832,58,576}, {58,832,245}, {350,58,921}, {185,922,59}, {922,864,59}, {866,185,59}, {60,397,925}, {427,500,60}, {60,500,923}, {397,60,923}, {924,427,60}, {925,924,60}, {61,505,927}, {926,928,61}, {653,926,61}, {927,653,61}, {505,61,928}, {62,321,697}, {362,648,62}, {648,929,62}, {930,62,697}, {321,62,929}, {62,930,362}, {255,760,63}, {388,255,63}, {760,846,63}, {126,931,64}, {64,260,932}, {931,260,64}, {881,64,932}, {184,65,169}, {65,184,100}, {65,621,933}, {933,169,65}, {66,586,934}, {934,199,66}, {66,834,586}, {67,158,935}, {67,182,936}, {182,67,215}, {935,803,67}, {937,67,936}, {215,67,803}, {67,937,158}, {68,241,634}, {241,68,370}, {557,938,68}, {634,557,68}, {938,370,68}, {292,939,69}, {434,69,939}, {70,812,847}, {70,675,940}, {812,70,940}, {71,224,941}, {71,641,942}, {941,738,71}, {71,738,943}, {942,224,71}, {943,641,71}, {72,101,726}, {72,329,944}, {946,72,447}, {945,72,726}, {944,101,72}, {72,945,447}, {329,72,946}, {947,903,73}, {73,710,947}, {710,73,884}, {684,74,144}, {74,204,948}, {74,329,144}, {204,74,517}, {74,684,517}, {329,74,944}, {74,948,944}, {951,75,102}, {441,949,75}, {577,441,75}, {949,102,75}, {577,75,950}, {75,951,950}, {227,953,76}, {955,76,339}, {76,952,339}, {952,76,953}, {76,954,227}, {76,955,954}, {956,77,219}, {640,77,484}, {635,794,77}, {77,640,635}, {484,77,956}, {794,219,77}, {397,78,305}, {78,397,923}, {78,522,957}, {958,78,923}, {957,305,78}, {522,78,958}, {467,960,79}, {751,79,959}, {960,959,79}, {751,896,79}, {896,467,79}, {80,228,963}, {228,80,961}, {962,80,534}, {962,961,80}, {858,534,80}, {963,858,80}, {148,542,81}, {81,542,964}, {81,711,148}, {711,81,773}, {964,965,81}, {773,81,965}, {369,966,82}, {968,82,966}, {82,422,967}, {967,369,82}, {422,82,968}, {969,83,311}, {83,969,971}, {970,972,83}, {971,970,83}, {972,311,83}, {84,690,188}, {84,918,828}, {382,974,85}, {951,85,485}, {85,973,382}, {975,85,974}, {85,975,485}, {85,951,973}, {976,86,202}, {659,86,473}, {592,473,86}, {86,659,907}, {592,86,976}, {907,202,86}, {87,161,662}, {161,87,493}, {909,87,662}, {911,493,87}, {830,88,521}, {88,977,979}, {830,977,88}, {88,760,521}, {760,88,978}, {979,978,88}, {981,89,173}, {262,982,89}, {89,594,173}, {980,262,89}, {89,981,980}, {594,89,982}, {109,985,90}, {90,249,983}, {90,449,109}, {983,984,90}, {449,90,984}, {249,90,985}, {277,91,214}, {91,277,986}, {550,214,91}, {987,91,986}, {91,987,988}, {988,550,91}, {820,92,280}, {389,92,361}, {376,412,92}, {92,389,280}, {412,361,92}, {92,820,376}, {853,990,93}, {254,636,93}, {613,853,93}, {613,93,636}, {93,989,254}, {989,93,990}, {129,167,94}, {94,163,992}, {94,167,772}, {94,991,129}, {772,163,94}, {992,991,94}, {316,504,95}, {995,95,504}, {95,544,316}, {994,95,993}, {95,994,544}, {95,995,993}, {96,181,996}, {959,96,595}, {96,959,181}, {96,809,595}, {996,997,96}, {997,809,96}, {501,97,130}, {473,97,269}, {97,473,999}, {501,269,97}, {998,130,97}, {97,999,998}, {1001,98,106}, {740,1003,98}, {1002,98,1000}, {98,1001,740}, {98,1002,106}, {1003,1000,98}, {99,1004,1005}, {925,99,924}, {1005,884,99}, {99,885,924}, {1004,99,925}, {184,321,100}, {321,1006,100}, {850,100,1006}, {551,101,216}, {101,827,216}, {101,551,726}, {101,944,948}, {948,827,101}, {102,253,973}, {102,973,951}, {102,949,1007}, {253,102,1007}, {451,103,356}, {103,451,1012}, {103,1008,356}, {1008,103,1009}, {1011,103,1010}, {103,1011,1009}, {1010,103,1012}, {1014,104,322}, {104,421,322}, {421,104,1013}, {1016,104,506}, {104,1014,506}, {104,1015,1013}, {104,1016,1015}, {105,637,815}, {170,724,106}, {106,724,1001}, {1002,170,106}, {759,107,633}, {107,759,1018}, {107,1017,633}, {913,107,1018}, {1017,107,789}, {913,789,107}, {402,1019,108}, {1020,108,1019}, {1020,1021,108}, {732,108,1021}, {108,732,1022}, {108,1022,402}, {985,109,395}, {862,109,449}, {861,395,109}, {109,862,861}, {328,110,124}, {157,1023,110}, {110,328,790}, {1024,110,790}, {1023,124,110}, {110,1024,157}, {111,1025,833}, {1025,111,862}, {112,1026,664}, {611,837,112}, {1026,112,1027}, {112,835,1027}, {611,112,664}, {835,112,837}, {113,195,383}, {383,730,113}, {195,113,809}, {730,268,113}, {326,299,114}, {453,384,115}, {490,453,115}, {794,490,115}, {908,1028,116}, {116,903,947}, {947,895,116}, {903,116,1028}, {305,969,117}, {305,117,397}, {1029,117,969}, {1029,1030,117}, {1004,117,1030}, {1004,397,117}, {508,118,318}, {118,508,1031}, {1007,318,118}, {1032,118,1031}, {720,1007,118}, {118,1032,720}, {370,938,119}, {1033,119,486}, {628,486,119}, {119,638,370}, {119,1033,638}, {628,119,938}, {404,1013,120}, {120,450,404}, {1015,120,1013}, {1034,450,120}, {1035,1034,120}, {120,1015,1035}, {172,1036,121}, {653,121,1036}, {1038,121,653}, {669,172,121}, {669,121,1037}, {121,1038,1037}, {140,1039,122}, {122,588,761}, {673,588,122}, {761,140,122}, {122,1039,673}, {769,123,128}, {630,123,399}, {123,446,399}, {123,630,128}, {123,769,1040}, {1040,446,123}, {409,1041,124}, {124,1023,409}, {1041,328,124}, {314,1043,125}, {125,683,806}, {1044,125,806}, {683,125,1042}, {1043,1042,125}, {125,1044,314}, {931,126,908}, {1028,126,878}, {1028,908,126}, {856,127,235}, {271,1045,127}, {303,235,127}, {127,856,271}, {127,1045,303}, {150,769,128}, {150,128,224}, {630,224,128}, {129,187,838}, {838,1046,129}, {187,129,991}, {167,129,1046}, {961,130,279}, {130,961,962}, {962,501,130}, {130,998,279}, {180,1047,131}, {131,504,1050}, {504,131,1047}, {1048,180,131}, {1048,131,1049}, {1050,1049,131}, {142,651,132}, {132,391,1051}, {391,132,448}, {132,626,142}, {651,448,132}, {626,132,1051}, {329,133,144}, {133,329,1053}, {359,1023,133}, {1052,144,133}, {133,1023,1052}, {133,1053,359}, {531,1055,134}, {1054,531,134}, {1056,134,763}, {1055,763,134}, {134,1056,1054}, {135,281,523}, {135,295,1058}, {523,585,135}, {1057,135,585}, {135,1057,295}, {281,135,1058}, {815,979,136}, {666,816,136}, {979,666,136}, {196,437,137}, {437,786,137}, {575,137,526}, {137,575,196}, {137,860,526}, {860,137,786}, {522,1062,138}, {1060,138,1059}, {138,1060,522}, {1061,1059,138}, {1062,1061,138}, {139,642,891}, {823,642,139}, {290,1063,140}, {393,290,140}, {1039,140,1063}, {140,761,393}, {141,234,1032}, {1015,141,1035}, {1031,1035,141}, {141,1015,1016}, {1032,1031,141}, {234,141,1016}, {626,731,142}, {142,688,1064}, {688,142,731}, {651,142,1064}, {385,1065,143}, {789,143,614}, {1045,143,1065}, {143,1045,614}, {143,789,912}, {912,385,143}, {1066,144,1052}, {144,1066,684}, {578,1067,145}, {1067,885,145}, {145,904,578}, {1071,146,289}, {146,699,1070}, {146,1068,289}, {1069,1068,146}, {1070,1069,146}, {146,1071,699}, {147,205,315}, {147,315,1072}, {147,547,1073}, {147,797,799}, {205,147,799}, {547,147,1072}, {797,147,1073}, {148,497,1074}, {148,711,996}, {542,148,1074}, {497,148,996}, {149,462,1075}, {601,1076,149}, {601,149,1075}, {462,149,754}, {1077,149,1076}, {1077,754,149}, {942,150,224}, {620,150,503}, {513,503,150}, {150,620,769}, {150,942,513}, {537,1078,151}, {1079,151,1078}, {151,1079,1081}, {151,1080,915}, {1081,1080,151}, {915,537,151}, {168,1083,152}, {206,1082,152}, {345,168,152}, {1082,345,152}, {206,152,1083}, {931,908,153}, {1084,153,763}, {153,894,1056}, {153,1084,931}, {153,1056,763}, {154,182,1049}, {182,154,366}, {483,366,154}, {483,154,1085}, {1049,1050,154}, {1050,1085,154}, {498,1086,155}, {635,640,155}, {640,1087,155}, {635,155,1086}, {1087,498,155}, {186,1088,156}, {156,193,341}, {341,186,156}, {156,1088,1089}, {1090,156,1089}, {193,156,1090}, {1023,157,1052}, {157,1091,1066}, {1024,1091,157}, {1052,157,1066}, {158,453,471}, {471,935,158}, {158,1092,453}, {1092,158,937}, {621,159,416}, {851,883,159}, {853,160,174}, {160,517,1093}, {1093,174,160}, {161,332,1094}, {332,161,493}, {161,1094,1095}, {1095,662,161}, {169,933,162}, {225,169,162}, {933,810,162}, {1096,163,312}, {668,312,163}, {163,1096,992}, {772,668,163}, {164,332,1098}, {1097,332,164}, {164,454,1099}, {454,164,563}, {1098,563,164}, {1097,164,1099}, {355,652,165}, {165,1100,355}, {831,165,652}, {1100,165,833}, {665,166,272}, {313,272,166}, {166,817,819}, {166,665,867}, {867,817,166}, {819,313,166}, {1046,1101,167}, {1101,772,167}, {168,345,787}, {672,1083,168}, {168,723,672}, {787,723,168}, {169,225,258}, {184,169,258}, {487,894,170}, {170,1002,487}, {171,180,1102}, {646,171,1102}, {180,171,1047}, {646,1103,171}, {995,171,1103}, {171,995,1047}, {1036,172,581}, {172,669,1105}, {172,1104,729}, {172,729,581}, {1104,172,1105}, {173,464,981}, {594,1106,173}, {464,173,1106}, {174,223,380}, {380,990,174}, {174,990,853}, {174,1093,223}, {723,175,555}, {1107,175,580}, {584,1108,175}, {175,723,584}, {175,1107,555}, {580,175,1108}, {1034,176,353}, {1109,176,508}, {176,1034,1035}, {1035,1031,176}, {1031,508,176}, {176,1109,353}, {177,242,396}, {396,1110,177}, {1059,1061,177}, {1110,1059,177}, {242,177,1061}, {178,497,1111}, {178,646,1112}, {178,713,1113}, {497,178,1074}, {713,178,1111}, {1074,178,1112}, {646,178,1113}, {179,367,904}, {1028,179,903}, {879,179,878}, {179,1028,878}, {1114,179,879}, {903,179,904}, {179,1114,367}, {691,1102,180}, {180,1048,691}, {497,996,181}, {960,181,959}, {960,497,181}, {215,1048,182}, {936,182,366}, {182,1048,1049}, {183,348,689}, {817,867,183}, {183,540,899}, {689,817,183}, {540,183,867}, {348,183,899}, {184,258,1115}, {184,697,321}, {697,184,1115}, {619,922,185}, {1116,619,185}, {866,1116,185}, {186,341,952}, {953,186,952}, {1088,186,424}, {953,424,186}, {1117,187,570}, {187,991,570}, {187,1117,1118}, {839,187,1118}, {839,838,187}, {920,188,325}, {1119,188,690}, {325,188,1119}, {541,189,390}, {189,541,1120}, {655,889,189}, {753,655,189}, {753,189,1120}, {390,189,889}, {190,323,972}, {190,643,700}, {323,190,967}, {190,700,967}, {643,190,970}, {972,970,190}, {217,410,191}, {410,511,191}, {682,191,511}, {682,1121,191}, {217,191,1121}, {201,837,192}, {192,489,1069}, {489,192,611}, {1069,1070,192}, {201,192,1070}, {837,611,192}, {455,566,193}, {1020,193,566}, {193,1020,341}, {1090,455,193}, {419,593,194}, {419,194,888}, {194,552,888}, {593,629,194}, {629,552,194}, {383,195,233}, {809,997,195}, {1122,195,997}, {195,1122,233}, {1123,196,486}, {575,486,196}, {437,196,1123}, {930,197,1124}, {1125,1124,197}, {197,873,1125}, {197,930,875}, {198,236,824}, {236,198,882}, {199,934,1126}, {199,1126,556}, {201,739,200}, {200,372,823}, {200,823,568}, {372,200,739}, {739,201,1070}, {532,976,202}, {1127,202,906}, {202,1127,532}, {202,907,906}, {203,280,1128}, {203,477,251}, {203,1128,1129}, {1129,477,203}, {204,342,829}, {204,827,948}, {827,204,829}, {217,1130,205}, {205,374,217}, {205,799,374}, {205,1130,315}, {206,363,1110}, {1082,206,1110}, {363,206,1131}, {1132,206,1083}, {206,1132,1131}, {207,247,496}, {924,207,427}, {496,427,207}, {247,207,1067}, {207,924,885}, {885,1067,207}, {1133,208,558}, {208,1133,1135}, {1134,747,208}, {1134,208,1135}, {208,661,558}, {661,208,747}, {312,668,209}, {1136,209,548}, {209,663,548}, {663,209,668}, {209,1136,1137}, {1137,312,209}, {1135,210,354}, {1135,432,210}, {210,863,354}, {814,211,347}, {211,676,965}, {1138,347,211}, {211,964,1138}, {965,964,211}, {211,814,676}, {212,1097,1099}, {1097,212,491}, {625,212,570}, {625,491,212}, {212,1117,570}, {1117,212,1099}, {463,779,213}, {580,1139,213}, {1139,1140,213}, {779,580,213}, {213,1140,463}, {916,214,480}, {1141,214,550}, {214,1141,480}, {214,916,277}, {1048,215,804}, {216,406,551}, {828,406,216}, {410,217,374}, {1130,217,442}, {1121,442,217}, {218,1142,748}, {725,737,218}, {1144,218,737}, {1143,218,748}, {1143,725,218}, {1142,218,1144}, {780,219,405}, {219,780,956}, {220,259,752}, {259,220,262}, {1146,220,1145}, {220,1146,262}, {1145,220,1147}, {752,1147,220}, {243,660,221}, {660,1148,221}, {1091,1024,221}, {1148,1091,221}, {243,221,1024}, {1149,222,471}, {222,602,935}, {935,471,222}, {1150,222,1149}, {222,1150,1151}, {602,222,1151}, {1148,223,1091}, {223,1148,380}, {223,1066,1091}, {1093,1066,223}, {224,630,941}, {268,360,225}, {225,360,258}, {428,741,226}, {226,632,428}, {785,226,741}, {632,226,1152}, {226,785,1153}, {1152,226,1153}, {227,1154,408}, {408,953,227}, {1154,227,507}, {954,507,227}, {228,279,1009}, {317,963,228}, {279,228,961}, {228,1009,1011}, {1011,317,228}, {636,254,229}, {886,636,229}, {230,359,1053}, {609,1155,230}, {1155,619,230}, {1053,609,230}, {581,231,1036}, {653,231,926}, {574,926,231}, {231,581,1157}, {231,653,1036}, {574,231,1156}, {231,1157,1156}, {302,1159,232}, {980,232,413}, {232,1158,302}, {232,980,981}, {981,1158,232}, {413,232,1159}, {821,233,334}, {233,512,383}, {233,821,512}, {1122,334,233}, {324,1160,234}, {234,506,324}, {1160,1032,234}, {506,234,1016}, {235,303,1012}, {1012,1161,235}, {235,1161,857}, {856,235,857}, {236,549,1003}, {549,236,882}, {825,236,1003}, {1125,237,319}, {1162,237,469}, {237,1125,873}, {560,319,237}, {873,469,237}, {237,1162,560}, {238,369,1164}, {966,369,238}, {238,717,1165}, {1163,966,238}, {717,238,1164}, {1165,1163,238}, {239,567,687}, {687,1166,239}, {239,1166,1168}, {1167,567,239}, {1168,1167,239}, {240,584,1170}, {658,1169,240}, {584,240,1169}, {788,658,240}, {1170,788,240}, {370,1172,241}, {634,241,692}, {1171,692,241}, {241,1172,1171}, {1174,242,415}, {1173,242,667}, {242,1173,396}, {242,1174,667}, {415,242,1061}, {243,331,1175}, {790,243,1024}, {660,243,1175}, {1114,244,1176}, {1177,1176,244}, {244,879,880}, {1178,1177,244}, {1178,244,880}, {879,244,1114}, {1130,245,831}, {442,245,1130}, {605,246,265}, {1147,246,1145}, {864,1145,246}, {246,1147,265}, {492,496,247}, {492,247,617}, {722,617,247}, {722,247,1067}, {1180,248,736}, {248,1179,573}, {1179,248,1180}, {845,736,248}, {1181,249,585}, {587,983,249}, {587,249,1181}, {985,585,249}, {364,1184,250}, {479,1183,250}, {1182,364,250}, {1182,250,1183}, {1184,479,250}, {477,721,251}, {251,579,822}, {579,251,721}, {1154,457,252}, {252,408,1154}, {457,796,252}, {519,408,252}, {796,519,252}, {546,973,253}, {1007,720,253}, {1185,253,720}, {253,1185,546}, {819,254,989}, {1038,255,388}, {927,1186,255}, {255,1038,927}, {760,255,1186}, {430,256,876}, {256,430,1078}, {1187,256,537}, {1078,537,256}, {1014,876,256}, {1187,1014,256}, {257,452,1180}, {628,452,257}, {1188,257,693}, {1188,628,257}, {257,736,693}, {736,257,1180}, {674,258,360}, {258,674,1115}, {262,980,259}, {413,784,259}, {413,259,980}, {752,259,784}, {260,931,1084}, {1189,260,939}, {260,434,939}, {260,1189,932}, {434,260,1084}, {600,261,320}, {261,600,855}, {261,715,1190}, {715,261,855}, {1120,320,261}, {1190,1120,261}, {982,262,1146}, {263,357,1192}, {386,1191,263}, {660,386,263}, {357,263,1191}, {1192,660,263}, {264,671,813}, {671,264,826}, {265,1147,807}, {605,265,808}, {718,266,500}, {266,522,958}, {266,718,770}, {770,1062,266}, {522,266,1062}, {500,266,958}, {296,1021,267}, {566,868,267}, {1021,566,267}, {686,296,267}, {267,1193,686}, {1193,267,868}, {360,268,285}, {285,268,730}, {269,482,659}, {269,501,1194}, {659,473,269}, {482,269,1194}, {270,355,1100}, {1100,698,270}, {270,698,1195}, {728,355,270}, {1195,728,270}, {271,375,1196}, {375,271,600}, {856,600,271}, {1045,271,1196}, {272,313,433}, {433,1197,272}, {272,1193,665}, {1193,272,1197}, {273,387,755}, {1199,273,755}, {387,273,1046}, {1101,273,1198}, {1101,1046,273}, {1199,1198,273}, {928,274,349}, {926,574,274}, {574,757,274}, {757,349,274}, {274,928,926}, {812,275,847}, {275,848,847}, {813,1200,275}, {848,275,1200}, {283,812,276}, {276,812,940}, {1201,283,276}, {1150,1201,276}, {276,1151,1150}, {1151,276,940}, {1199,277,515}, {916,515,277}, {277,1199,986}, {278,529,1204}, {567,1203,278}, {1202,1171,278}, {529,278,1171}, {278,1203,1202}, {1204,567,278}, {279,1008,1009}, {1008,279,998}, {280,389,1128}, {281,744,902}, {281,902,523}, {281,1058,845}, {845,744,281}, {311,972,282}, {282,489,311}, {1068,1069,282}, {282,1069,489}, {972,1068,282}, {811,283,543}, {283,1201,543}, {284,418,519}, {418,284,424}, {798,284,519}, {284,553,1205}, {553,284,798}, {1205,424,284}, {874,285,597}, {730,597,285}, {285,874,360}, {333,1108,286}, {286,584,1169}, {1042,286,1169}, {286,1042,1043}, {1043,333,286}, {584,286,1108}, {287,483,607}, {1206,287,607}, {1206,351,287}, {287,872,483}, {606,1209,288}, {1207,897,288}, {897,1208,288}, {606,288,1208}, {1209,1207,288}, {338,1071,289}, {289,694,338}, {289,1068,1210}, {694,289,1210}, {290,304,1063}, {431,290,393}, {290,431,1212}, {304,290,1211}, {1212,1211,290}, {291,319,843}, {1124,1125,291}, {291,429,1124}, {319,291,1125}, {429,291,725}, {291,842,725}, {842,291,843}, {292,545,1189}, {852,545,292}, {939,292,1189}, {356,293,308}, {293,356,1008}, {308,293,391}, {998,293,1008}, {293,592,391}, {293,998,999}, {592,293,999}, {294,428,632}, {428,294,524}, {596,1213,294}, {1203,294,632}, {294,1203,596}, {1213,524,294}, {693,1058,295}, {1214,295,1057}, {1214,693,295}, {1177,296,583}, {686,583,296}, {1021,296,732}, {296,1177,732}, {1063,1216,297}, {1215,297,1088}, {1088,297,1089}, {297,1215,1039}, {1063,297,1039}, {1216,1089,297}, {833,298,1100}, {298,440,698}, {1217,298,627}, {698,1100,298}, {298,1217,440}, {298,1025,627}, {298,833,1025}, {1218,299,326}, {1127,299,644}, {299,1218,644}, {299,1127,906}, {304,1219,300}, {300,466,1216}, {494,1220,300}, {1219,494,300}, {466,300,1220}, {304,300,1216}, {381,1006,301}, {1221,301,639}, {301,1221,381}, {929,301,1006}, {1222,639,301}, {301,929,1222}, {1159,302,478}, {1158,1224,302}, {478,302,1223}, {1224,1223,302}, {612,1010,303}, {303,1065,612}, {1012,303,1010}, {1065,303,1045}, {304,1211,1219}, {1216,1063,304}, {957,971,305}, {969,305,971}, {306,623,1187}, {914,1226,306}, {623,306,1225}, {1187,917,306}, {306,1226,1225}, {306,917,914}, {328,1227,307}, {307,520,791}, {520,307,750}, {307,791,328}, {1227,1228,307}, {307,1228,750}, {391,448,308}, {448,565,308}, {565,356,308}, {309,338,1230}, {309,544,994}, {994,771,309}, {544,309,1229}, {1230,1229,309}, {338,309,771}, {310,481,871}, {1231,310,645}, {1092,937,310}, {310,1231,1092}, {871,645,310}, {481,310,937}, {1029,311,489}, {311,1029,969}, {680,1096,312}, {312,1137,680}, {313,357,433}, {357,313,572}, {819,572,313}, {764,314,910}, {314,764,1232}, {1044,910,314}, {1043,314,1232}, {315,831,652}, {315,652,1072}, {831,315,1130}, {1233,316,544}, {316,607,1085}, {1050,316,1085}, {607,316,1233}, {316,1050,504}, {963,317,394}, {612,1234,317}, {1234,394,317}, {1011,612,317}, {949,1235,318}, {318,1007,949}, {733,508,318}, {733,318,1235}, {319,560,841}, {841,843,319}, {600,320,375}, {320,541,1236}, {1236,375,320}, {320,1120,541}, {1006,321,929}, {876,1014,322}, {877,322,421}, {322,877,876}, {1210,323,422}, {323,967,422}, {1068,972,323}, {1068,323,1210}, {324,506,623}, {623,1225,324}, {1225,1160,324}, {569,1237,325}, {325,1237,920}, {1119,569,325}, {901,1218,326}, {476,478,327}, {327,478,610}, {327,589,476}, {589,327,839}, {791,790,328}, {1227,328,1041}, {1053,329,946}, {889,330,390}, {463,465,330}, {390,330,465}, {330,890,463}, {331,1207,1175}, {1207,331,897}, {332,1097,1094}, {493,1098,332}, {333,759,1139}, {1108,333,1139}, {333,1043,1232}, {333,1018,759}, {1018,333,1232}, {334,376,820}, {820,821,334}, {376,334,709}, {334,1122,709}, {1238,335,516}, {335,993,995}, {993,335,756}, {516,335,1103}, {335,1238,756}, {995,1103,335}, {507,336,1154}, {336,457,1154}, {748,336,556}, {457,336,1142}, {336,748,1142}, {337,393,1240}, {393,337,431}, {616,431,337}, {337,1239,616}, {1240,1195,337}, {1239,337,1195}, {694,1230,338}, {1071,338,771}, {339,952,1019}, {402,955,339}, {1019,402,339}, {340,353,1109}, {616,1239,340}, {1241,340,1239}, {733,616,340}, {733,340,1109}, {353,340,1241}, {341,1019,952}, {1019,341,1020}, {704,829,342}, {854,704,342}, {343,459,1242}, {459,343,1026}, {343,664,1026}, {664,343,1030}, {343,1004,1030}, {343,1242,1005}, {1004,343,1005}, {1242,344,420}, {1243,344,459}, {344,1242,459}, {344,1243,724}, {1244,892,345}, {345,1082,1244}, {892,787,345}, {487,1002,346}, {346,1054,487}, {631,1245,346}, {346,1000,631}, {1000,346,1002}, {1054,346,1245}, {347,1138,510}, {898,689,348}, {1218,349,644}, {349,757,758}, {758,644,349}, {349,1218,928}, {576,350,445}, {503,513,350}, {350,513,445}, {350,719,503}, {719,350,921}, {419,468,351}, {468,655,351}, {351,1206,419}, {562,807,352}, {352,706,562}, {1147,752,352}, {783,352,752}, {352,783,706}, {1147,352,807}, {1241,1034,353}, {354,1134,1135}, {354,776,1246}, {776,354,863}, {1134,354,1246}, {707,652,355}, {355,728,707}, {356,565,451}, {572,1192,357}, {433,357,1191}, {358,369,1132}, {358,672,1247}, {369,358,1164}, {1083,672,358}, {1164,358,1247}, {1132,1083,358}, {1023,359,409}, {360,874,674}, {361,412,849}, {361,848,1200}, {848,361,849}, {1200,389,361}, {362,1124,429}, {648,362,401}, {429,401,362}, {1124,362,930}, {1060,363,411}, {1059,1110,363}, {363,643,411}, {643,363,1131}, {363,1060,1059}, {583,686,364}, {686,1197,364}, {364,1182,583}, {1184,364,1197}, {1248,365,477}, {721,477,365}, {1249,365,1166}, {365,1248,1168}, {365,1249,721}, {1168,1166,365}, {366,481,936}, {366,483,872}, {481,366,872}, {367,578,904}, {367,647,1250}, {578,367,1250}, {1114,647,367}, {1097,491,368}, {598,1094,368}, {1094,1097,368}, {598,368,858}, {369,967,700}, {369,700,1132}, {1172,370,638}, {371,414,518}, {371,502,414}, {716,371,518}, {671,1251,371}, {1252,371,1251}, {371,716,671}, {371,1252,502}, {642,823,372}, {372,739,1253}, {372,1238,642}, {1238,372,1253}, {1145,864,373}, {535,1146,373}, {1145,373,1146}, {922,535,373}, {864,922,373}, {795,410,374}, {374,799,795}, {375,514,1196}, {375,1236,514}, {1254,376,709}, {376,1254,412}, {377,428,524}, {432,1255,377}, {432,377,499}, {1252,377,524}, {428,377,1255}, {377,1251,499}, {1251,377,1252}, {727,444,378}, {1080,727,378}, {378,915,1080}, {744,379,637}, {379,815,637}, {744,978,379}, {978,979,379}, {979,815,379}, {990,380,603}, {1192,380,1148}, {380,1192,603}, {1006,381,850}, {381,475,851}, {1221,475,381}, {851,850,381}, {973,546,382}, {546,656,382}, {974,382,569}, {569,382,656}, {512,1256,383}, {730,383,1256}, {384,453,1092}, {384,1092,1231}, {1231,715,384}, {662,385,909}, {385,662,1257}, {1065,385,1234}, {385,1257,1234}, {385,912,909}, {1191,386,479}, {386,660,1175}, {1207,1209,386}, {386,1209,479}, {386,1175,1207}, {387,743,755}, {1046,838,387}, {573,1037,388}, {388,1037,1038}, {389,518,1128}, {518,389,716}, {389,1200,716}, {746,390,465}, {390,746,541}, {391,592,1051}, {966,1163,392}, {1258,1259,392}, {392,629,1258}, {629,392,1163}, {968,966,392}, {968,392,1259}, {393,761,1240}, {394,598,963}, {394,1234,1257}, {1095,394,1257}, {598,394,1095}, {395,1057,985}, {395,861,1260}, {1057,395,1214}, {1214,395,1260}, {396,1244,1082}, {1110,396,1082}, {1244,396,1173}, {397,1004,925}, {398,414,502}, {398,502,1213}, {414,398,1248}, {1167,1168,398}, {398,1168,1248}, {1213,1167,398}, {1262,399,446}, {399,708,630}, {1261,708,399}, {399,1262,1261}, {400,1027,835}, {835,836,400}, {740,1001,400}, {1243,400,1001}, {400,836,740}, {1027,400,1243}, {401,429,1143}, {934,648,401}, {401,1126,934}, {1126,401,1143}, {1263,955,402}, {1022,1263,402}, {403,440,1217}, {450,1034,403}, {1034,1241,403}, {1217,450,403}, {440,403,1241}, {1264,404,450}, {1264,1265,404}, {1265,1013,404}, {405,857,780}, {464,551,406}, {530,1158,406}, {828,530,406}, {464,406,1158}, {484,1266,407}, {1266,651,407}, {407,651,1268}, {1267,484,407}, {1267,407,1268}, {953,408,418}, {408,519,418}, {1041,409,801}, {800,410,795}, {840,410,460}, {410,800,460}, {410,840,511}, {559,1060,411}, {970,411,643}, {411,970,559}, {676,849,412}, {676,412,1254}, {1159,1269,413}, {784,413,1269}, {1129,518,414}, {414,1248,1129}, {415,1227,1174}, {1061,1062,415}, {415,1062,1228}, {1227,415,1228}, {416,762,621}, {762,416,896}, {610,1223,417}, {417,734,988}, {734,417,1223}, {987,610,417}, {988,987,417}, {424,953,418}, {468,419,888}, {593,419,1206}, {420,947,710}, {1242,420,710}, {947,420,895}, {1013,1265,421}, {421,1270,877}, {1270,421,1265}, {422,694,1210}, {694,422,968}, {423,886,887}, {423,696,705}, {705,1271,423}, {423,887,975}, {886,423,1271}, {975,696,423}, {424,1215,1088}, {424,1205,1215}, {708,425,657}, {425,687,1204}, {425,708,1272}, {425,738,657}, {425,1272,1273}, {687,425,1273}, {1204,738,425}, {426,910,1044}, {654,1274,426}, {426,701,911}, {701,426,1274}, {910,426,911}, {1044,654,426}, {427,496,718}, {500,427,718}, {741,428,1255}, {725,1143,429}, {430,1079,1078}, {1079,430,816}, {1275,431,616}, {431,1275,1212}, {1255,432,1133}, {432,1135,1133}, {1191,1184,433}, {1197,433,1184}, {1055,439,434}, {434,1084,1055}, {485,887,435}, {678,950,435}, {950,485,435}, {1202,436,692}, {1104,692,436}, {1203,436,1202}, {436,729,1104}, {1153,436,1152}, {436,1153,729}, {436,1203,1152}, {1214,437,1188}, {1188,437,1123}, {437,1214,1260}, {786,437,1260}, {1102,438,554}, {1102,691,438}, {805,438,691}, {964,438,1138}, {964,554,438}, {438,805,1138}, {531,639,439}, {439,639,834}, {1055,531,439}, {440,1239,698}, {1239,440,1241}, {564,749,441}, {441,577,564}, {949,441,1235}, {441,749,1235}, {1121,921,442}, {593,1206,443}, {604,593,443}, {1206,1233,443}, {1233,1229,443}, {604,443,1229}, {444,727,781}, {444,781,1198}, {1198,515,444}, {513,1276,445}, {526,576,445}, {526,445,1276}, {446,767,1262}, {446,1040,1162}, {1162,767,446}, {447,609,946}, {447,1277,609}, {945,1277,447}, {565,448,1266}, {448,651,1266}, {449,627,1025}, {627,449,984}, {1025,862,449}, {622,1264,450}, {450,1217,622}, {565,1161,451}, {1161,1012,451}, {557,1179,452}, {452,628,938}, {452,1179,1180}, {938,557,452}, {453,490,471}, {777,454,563}, {1099,454,1117}, {454,777,1278}, {1118,454,1278}, {454,1118,1117}, {566,455,868}, {1090,869,455}, {456,1279,1086}, {456,1201,1150}, {1280,456,1086}, {1149,1279,456}, {1150,1149,456}, {456,1280,1201}, {1144,457,1142}, {796,457,685}, {457,1144,685}, {776,458,498}, {1280,458,543}, {458,776,863}, {458,1280,498}, {1027,459,1026}, {459,1027,1243}, {1144,460,800}, {737,840,460}, {460,1144,737}, {553,1073,461}, {588,673,461}, {1205,461,673}, {1073,588,461}, {461,1205,553}, {677,1075,462}, {462,681,1281}, {681,462,754}, {1281,677,462}, {465,463,1140}, {779,463,890}, {1158,981,464}, {551,464,1106}, {465,633,746}, {633,465,1140}, {466,1089,1216}, {869,1090,466}, {466,1220,869}, {1089,466,1090}, {960,467,528}, {889,655,468}, {469,873,792}, {767,1162,469}, {527,1246,470}, {527,470,590}, {470,1267,1268}, {1268,590,470}, {1246,1087,470}, {470,1087,1267}, {471,490,1149}, {472,480,1141}, {670,714,472}, {714,1226,472}, {1141,670,472}, {1226,480,472}, {473,592,999}, {474,494,1219}, {900,474,564}, {564,474,608}, {474,1211,608}, {1211,474,1219}, {883,475,631}, {475,1221,1245}, {475,883,851}, {1245,631,475}, {589,1269,476}, {478,476,1159}, {1269,1159,476}, {477,1129,1248}, {1223,610,478}, {1183,479,1209}, {479,1184,1191}, {914,916,480}, {480,1226,914}, {937,936,481}, {872,871,481}, {905,482,591}, {482,905,659}, {1137,482,1194}, {591,482,1136}, {482,1137,1136}, {1085,607,483}, {1266,484,745}, {484,1267,640}, {745,484,956}, {485,950,951}, {887,485,975}, {486,575,1033}, {486,628,1123}, {1056,487,1054}, {894,487,1056}, {1261,488,509}, {488,1261,1262}, {488,1249,509}, {1249,488,765}, {1262,793,488}, {793,765,488}, {1029,489,611}, {1279,1149,490}, {490,794,1279}, {491,625,561}, {492,520,1282}, {897,492,617}, {496,492,1282}, {1098,493,701}, {493,911,701}, {899,1220,494}, {881,495,533}, {545,1263,495}, {533,495,1263}, {495,1189,545}, {495,881,932}, {1189,495,932}, {1282,718,496}, {497,960,1111}, {1086,498,1280}, {498,1087,776}, {1251,826,499}, {958,923,500}, {501,962,1283}, {501,680,1194}, {680,501,1283}, {1252,1213,502}, {719,620,503}, {995,504,1047}, {1218,901,505}, {505,928,1218}, {1186,505,901}, {505,1186,927}, {623,506,1014}, {507,954,852}, {508,733,1109}, {509,1166,1273}, {1166,509,1249}, {1272,1261,509}, {509,1273,1272}, {510,1138,805}, {511,844,682}, {1256,512,599}, {821,599,512}, {1276,513,942}, {614,1196,514}, {1236,1017,514}, {1017,614,514}, {515,1198,1199}, {516,642,1238}, {642,516,742}, {1284,516,1103}, {516,1284,742}, {517,684,1093}, {518,1129,1128}, {519,796,798}, {750,1282,520}, {521,760,1186}, {901,521,1186}, {957,522,1060}, {1181,523,615}, {902,615,523}, {585,523,1181}, {524,1213,1252}, {591,735,525}, {525,905,591}, {576,526,860}, {575,526,1276}, {688,527,590}, {1246,527,1134}, {527,688,778}, {747,1134,527}, {747,527,778}, {713,1111,528}, {960,528,1111}, {738,1204,529}, {529,1171,1172}, {1172,738,529}, {530,828,918}, {1158,530,1224}, {919,530,918}, {530,919,1224}, {531,1054,1245}, {639,531,1221}, {531,1245,1221}, {976,532,626}, {1285,626,532}, {1127,1285,532}, {1022,533,1263}, {880,533,1178}, {533,1022,1178}, {880,881,533}, {534,1283,962}, {1283,534,859}, {535,609,1277}, {535,922,1155}, {1277,982,535}, {609,535,1155}, {1146,535,982}, {920,536,538}, {1286,536,550}, {670,1141,536}, {550,536,1141}, {536,1286,538}, {920,670,536}, {917,537,915}, {537,917,1187}, {919,538,1286}, {695,712,539}, {712,1116,539}, {1116,866,539}, {540,869,1220}, {540,1220,899}, {541,746,1236}, {554,964,542}, {1112,542,1074}, {1112,554,542}, {1280,543,1201}, {1229,1233,544}, {545,852,954}, {545,954,955}, {955,1263,545}, {1185,1287,546}, {656,546,1287}, {588,1073,547}, {547,707,1288}, {707,547,1072}, {547,1288,588}, {571,1289,548}, {571,548,663}, {1289,1136,548}, {1000,1003,549}, {883,1000,549}, {550,988,1286}, {1106,726,551}, {552,629,702}, {1290,552,702}, {552,779,890}, {779,552,1290}, {1073,553,797}, {798,797,553}, {554,646,1102}, {646,554,1112}, {555,717,1247}, {717,555,1107}, {1247,723,555}, {748,556,1126}, {557,634,1105}, {1179,557,669}, {1105,669,557}, {558,1255,1133}, {558,661,782}, {1255,558,741}, {782,741,558}, {970,971,559}, {1060,559,957}, {971,957,559}, {560,774,841}, {774,560,1040}, {1162,1040,560}, {992,561,625}, {561,1096,859}, {561,992,1096}, {654,807,562}, {706,1274,562}, {1274,654,562}, {1291,777,563}, {563,1098,1291}, {564,577,900}, {749,564,608}, {565,1266,745}, {745,1161,565}, {1021,1020,566}, {1203,567,596}, {567,1167,596}, {567,1204,687}, {568,825,836}, {1237,569,656}, {1119,974,569}, {625,570,991}, {663,681,571}, {681,754,571}, {754,1292,571}, {1289,571,1292}, {1192,572,603}, {989,603,572}, {572,819,989}, {1179,1037,573}, {574,661,1293}, {661,574,1156}, {757,574,1293}, {1033,575,641}, {1276,641,575}, {860,832,576}, {678,900,577}, {950,678,577}, {1067,578,722}, {722,578,1250}, {599,822,579}, {579,721,765}, {765,1294,579}, {579,1294,599}, {779,1107,580}, {1139,580,1108}, {729,1157,581}, {993,756,582}, {582,994,993}, {756,1295,582}, {771,994,582}, {1295,771,582}, {583,1176,1177}, {1176,583,1182}, {584,723,1170}, {585,985,1057}, {648,934,586}, {1222,648,586}, {834,1222,586}, {1265,587,1270}, {983,587,1264}, {1181,1270,587}, {1265,1264,587}, {761,588,1288}, {1296,589,1278}, {589,1296,1269}, {1118,1278,589}, {589,839,1118}, {590,1268,1064}, {1064,688,590}, {1136,1289,591}, {735,591,1289}, {1051,592,976}, {593,604,1258}, {629,593,1258}, {1277,945,594}, {1106,594,945}, {982,1277,594}, {959,595,751}, {595,810,751}, {1213,596,1167}, {597,730,766}, {597,792,874}, {858,963,598}, {1094,598,1095}, {599,821,822}, {1294,1256,599}, {856,855,600}, {601,1079,816}, {1076,601,666}, {1081,601,1075}, {816,666,601}, {601,1081,1079}, {935,602,803}, {1151,675,602}, {603,989,990}, {1229,1230,604}, {1259,604,1230}, {1259,1258,604}, {605,893,865}, {893,605,1297}, {808,1297,605}, {647,1183,606}, {1208,1250,606}, {1250,647,606}, {1209,606,1183}, {1233,1206,607}, {1211,1212,608}, {1212,749,608}, {609,1053,946}, {610,987,743}, {664,1029,611}, {1234,612,1065}, {612,1011,1010}, {636,1271,613}, {613,1271,854}, {614,1045,1196}, {614,1017,789}, {615,649,1270}, {1270,1181,615}, {616,733,1275}, {617,722,1208}, {1208,897,617}, {1226,618,1225}, {1287,618,714}, {1160,1225,618}, {618,1185,1160}, {1226,714,618}, {1185,618,1287}, {922,619,1155}, {619,1116,679}, {844,620,682}, {620,719,682}, {620,774,769}, {620,844,774}, {621,762,933}, {1264,622,983}, {984,983,622}, {1217,984,622}, {1014,1187,623}, {624,667,1174}, {624,679,1298}, {679,624,801}, {667,624,1298}, {624,1227,1041}, {1227,624,1174}, {801,624,1041}, {991,992,625}, {976,626,1051}, {626,1285,731}, {984,1217,627}, {628,1188,1123}, {1163,702,629}, {708,941,630}, {1000,883,631}, {1152,1203,632}, {759,633,1140}, {746,633,1017}, {692,1104,634}, {1105,634,1104}, {794,635,1279}, {1279,635,1086}, {1271,636,886}, {744,637,902}, {1033,943,638}, {943,1172,638}, {639,1222,834}, {1087,640,1267}, {943,1033,641}, {641,1276,942}, {891,642,742}, {1131,700,643}, {1127,644,758}, {870,753,645}, {1190,645,753}, {645,1190,1231}, {645,871,870}, {1103,646,1284}, {646,1113,1284}, {1182,647,1176}, {647,1182,1183}, {647,1114,1176}, {648,1222,929}, {877,1270,649}, {830,650,703}, {1292,650,735}, {1077,703,650}, {650,1292,1077}, {1064,1268,651}, {652,707,1072}, {653,927,1038}, {654,1044,806}, {655,753,870}, {1287,1237,656}, {941,708,657}, {738,941,657}, {1042,658,683}, {1169,658,1042}, {893,658,788}, {658,893,1297}, {1297,683,658}, {659,905,907}, {660,1192,1148}, {747,1293,661}, {661,1156,782}, {662,1095,1257}, {668,768,663}, {681,663,768}, {1030,1029,664}, {868,665,1193}, {977,1076,666}, {666,979,977}, {667,712,1173}, {712,667,1298}, {768,668,772}, {1037,1179,669}, {1237,714,670}, {670,920,1237}, {813,671,716}, {826,1251,671}, {723,1247,672}, {673,1215,1205}, {1039,1215,673}, {674,875,1115}, {675,1151,940}, {1254,965,676}, {849,676,814}, {727,1080,677}, {1080,1081,677}, {1075,677,1081}, {677,1281,727}, {900,678,898}, {678,818,898}, {1116,1298,679}, {680,1137,1194}, {859,680,1283}, {680,859,1096}, {775,681,768}, {681,775,1281}, {1121,682,719}, {683,1297,808}, {808,806,683}, {1066,1093,684}, {1144,800,685}, {1197,686,1193}, {1166,687,1273}, {1299,688,731}, {688,1299,778}, {689,898,818}, {690,696,1119}, {705,690,704}, {696,690,705}, {804,691,1048}, {691,804,805}, {692,1171,1202}, {1058,693,736}, {693,1214,1188}, {694,968,1259}, {1230,694,1259}, {1173,695,1244}, {712,695,1173}, {892,1244,695}, {974,1119,696}, {696,975,974}, {875,697,1115}, {697,875,930}, {1195,698,1239}, {739,1070,699}, {1295,739,699}, {1295,699,1071}, {700,1131,1132}, {701,1291,1098}, {1291,701,1274}, {702,1163,1165}, {702,1165,1290}, {977,830,703}, {703,1076,977}, {1076,703,1077}, {704,854,705}, {1271,705,854}, {1274,706,1291}, {777,1291,706}, {777,706,783}, {707,728,1288}, {708,1261,1272}, {709,773,1254}, {1122,773,709}, {1005,710,884}, {710,1005,1242}, {773,1122,711}, {997,996,711}, {711,1122,997}, {1116,712,1298}, {891,1113,713}, {714,1237,1287}, {1190,715,1231}, {716,1200,813}, {717,1164,1247}, {1290,717,1107}, {717,1290,1165}, {718,1282,770}, {921,1121,719}, {1160,720,1032}, {720,1160,1185}, {1249,765,721}, {722,1250,1208}, {723,787,1170}, {724,1243,1001}, {737,725,842}, {726,1106,945}, {1281,781,727}, {1240,1288,728}, {728,1195,1240}, {1157,729,1153}, {766,730,1256}, {1299,731,1285}, {1177,1178,732}, {732,1178,1022}, {749,733,1235}, {733,749,1275}, {1223,1224,734}, {1224,919,734}, {1286,734,919}, {734,1286,988}, {1292,735,1289}, {736,845,1058}, {840,737,842}, {738,1172,943}, {739,1295,1253}, {1003,740,825}, {740,836,825}, {741,782,785}, {742,1284,1113}, {1113,891,742}, {987,755,743}, {744,845,846}, {978,744,846}, {1161,745,780}, {780,745,956}, {1017,1236,746}, {1293,747,778}, {1143,748,1126}, {1275,749,1212}, {1282,750,770}, {1228,770,750}, {896,751,762}, {810,762,751}, {784,783,752}, {1120,1190,753}, {754,1077,1292}, {755,986,1199}, {986,755,987}, {1253,756,1238}, {1295,756,1253}, {757,1285,758}, {1285,757,1299}, {757,1293,1299}, {1285,1127,758}, {1140,1139,759}, {846,760,978}, {1288,1240,761}, {810,933,762}, {763,1055,1084}, {764,1018,1232}, {1018,764,913}, {1294,765,793}, {793,766,1294}, {1256,1294,766}, {793,1262,767}, {772,775,768}, {1040,769,774}, {770,1228,1062}, {771,1295,1071}, {772,1101,775}, {965,1254,773}, {841,774,844}, {781,1281,775}, {1101,781,775}, {1087,1246,776}, {783,1296,777}, {1278,777,1296}, {1293,778,1299}, {1107,779,1290}, {780,857,1161}, {781,1101,1198}, {1157,782,1156}, {782,1157,785}, {1296,783,784}, {784,1269,1296}, {1157,1153,785}, {1260,861,786}, {788,1170,787}, {788,787,892}, {892,893,788}, {789,913,912}};
	const int unitSphereVertexCount = sizeof(unitSphereVertices) / sizeof(unitSphereVertices[0]);
	const int unitSphereFaceCount = sizeof(unitSphereFaces) / sizeof(unitSphereFaces[0]);

	// Generate the triangle mesh for the union of spheres by duplicating the unit sphere template.
	std::shared_ptr<TriMesh> triMesh = std::make_shared<TriMesh>();
	triMesh->setVertexCount(unitSphereVertexCount * centers.size());
	triMesh->setFaceCount(unitSphereFaceCount * centers.size());
	triMesh->setHasNormals(true);
	auto vertex = triMesh->vertices().begin();
	auto face = triMesh->faces().begin();
	auto normal = triMesh->normals().begin();
	for(size_t sphereIndex = 0; sphereIndex < centers.size(); sphereIndex++) {
		const Point3 center = centers[sphereIndex];
		const FloatType diameter = diameters[sphereIndex];
		const int baseVertex = sphereIndex * unitSphereVertexCount;
		for(int i = 0; i < unitSphereVertexCount; i++)
			*vertex++ = Point3(unitSphereVertices[i][0] * diameter + center.x(), unitSphereVertices[i][1] * diameter + center.y(), unitSphereVertices[i][2] * diameter + center.z());
		for(int i = 0; i < unitSphereFaceCount; i++, ++face) {
			for(int v = 0; v < 3; v++) {
				face->setVertex(v, unitSphereFaces[i][v] + baseVertex);
				*normal++ = Vector3(unitSphereVertices[unitSphereFaces[i][v]][0]*2, unitSphereVertices[unitSphereFaces[i][v]][1]*2, unitSphereVertices[unitSphereFaces[i][v]][2]*2);
			}
		}
	}

	// Store shape geometry in internal cache to avoid parsing the JSON string again for other animation frames.
	_importer->storeParticleShapeInCache(shapeSpecString, triMesh);

	// Assign shape to particle type.
	typeList->setTypeShape(typeId, std::move(triMesh));
}

}	// End of namespace
}	// End of namespace
