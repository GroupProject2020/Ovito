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

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(GSDImporter);
DEFINE_PROPERTY_FIELD(GSDImporter, roundingResolution);
SET_PROPERTY_FIELD_LABEL(GSDImporter, roundingResolution, "Rounding resolution");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(GSDImporter, roundingResolution, IntegerParameterUnit, 1, 6);

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void GSDImporter::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(roundingResolution)) {
		// Clear shape cache and reload GSD file when the rounding resolution is changed.
		_cacheSynchronization.lockForWrite();
		_particleShapeCache.clear();
		_cacheSynchronization.unlock();
		requestReload();
	}
	ParticleImporter::propertyChanged(field);
}

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool GSDImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	QString filename = QDir::toNativeSeparators(input.fileName());

	gsd_handle handle;
	if(::gsd_open(&handle, filename.toLocal8Bit().constData(), GSD_OPEN_READONLY) == 0) {
		::gsd_close(&handle);
		return true;
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
void GSDImporter::FrameFinder::discoverFramesInFile(QFile& file, const QUrl& sourceUrl, QVector<FileSourceImporter::Frame>& frames)
{
	setProgressText(tr("Scanning file %1").arg(sourceUrl.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// First close text stream, we don't need it here.
	QString filename = QDir::toNativeSeparators(file.fileName());

	// Open GSD file for reading.
	GSDFile gsd(filename.toLocal8Bit().constData());
	uint64_t nFrames = gsd.numerOfFrames();

	QFileInfo fileInfo(filename);
	QDateTime lastModified = fileInfo.lastModified();
	for(uint64_t i = 0; i < nFrames; i++) {
		Frame frame;
		frame.sourceFile = sourceUrl;
		frame.byteOffset = i;
		frame.lastModificationTime = lastModified;
		frame.label = tr("Frame %1").arg(i);
		frames.push_back(frame);
	}
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr GSDImporter::FrameLoader::loadFile(QFile& file)
{
	setProgressText(tr("Reading GSD file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Open GSD file for reading.
	QString filename = QDir::toNativeSeparators(file.fileName());
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


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
