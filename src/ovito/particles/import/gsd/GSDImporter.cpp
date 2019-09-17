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

#include <ovito/particles/Particles.h>
#include <ovito/mesh/surface/SurfaceMeshData.h>
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
	simCell.column(3) = simCell * Vector3(FloatType(-0.5));
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
	PropertyPtr posProperty = ParticlesObject::OOClass().createStandardStorage(numParticles, ParticlesObject::PositionProperty, false);
	frameData->addParticleProperty(posProperty);
	gsd.readFloatArray("particles/position", frameNumber, posProperty->dataPoint3(), numParticles, posProperty->componentCount());
	if(isCanceled()) return {};

	// Create particle types.
	PropertyPtr typeProperty = ParticlesObject::OOClass().createStandardStorage(numParticles, ParticlesObject::TypeProperty, false);
	frameData->addParticleProperty(typeProperty);
	ParticleFrameData::TypeList* typeList = frameData->propertyTypesList(typeProperty);
	for(int i = 0; i < particleTypeNames.size(); i++)
		typeList->addTypeId(i, QString::fromUtf8(particleTypeNames[i]));

	// Read particle types.
	if(gsd.hasChunk("particles/typeid", frameNumber))
		gsd.readIntArray("particles/typeid", frameNumber, typeProperty->dataInt(), numParticles);
	else
		std::fill(typeProperty->dataInt(), typeProperty->dataInt() + typeProperty->size(), 0);
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
	PropertyStorage* radiusProperty = readOptionalProperty(gsd, "particles/diameter", frameNumber, numParticles, ParticlesObject::RadiusProperty, false, frameData);
	if(radiusProperty) {
		// Convert particle diameters to radii.
		std::for_each(radiusProperty->dataFloat(), radiusProperty->dataFloat() + radiusProperty->size(), [](FloatType& r) { r /= 2; });
	}
	PropertyStorage* orientationProperty = readOptionalProperty(gsd, "particles/orientation", frameNumber, numParticles, ParticlesObject::OrientationProperty, false, frameData);
	if(orientationProperty) {
		// Convert quaternion representation from GSD format to OVITO's internal format.
		// Left-shift all quaternion components by one: (W,X,Y,Z) -> (X,Y,Z,W).
		std::for_each(orientationProperty->dataQuaternion(), orientationProperty->dataQuaternion() + orientationProperty->size(), [](Quaternion& q) {
			std::rotate(q.begin(), q.begin() + 1, q.end());
		});
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
		PropertyPtr bondTopologyProperty = BondsObject::OOClass().createStandardStorage(numBonds, BondsObject::TopologyProperty, false);
		frameData->addBondProperty(bondTopologyProperty);
		auto bondTopoPtr = bondTopologyProperty->dataInt64();
		for(auto b = bondList.cbegin(); b != bondList.cend(); ++b, ++bondTopoPtr) {
			if(*b >= numParticles)
				throw Exception(tr("Nonexistent atom tag in bond list in GSD file."));
			*bondTopoPtr = *b;
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
			PropertyPtr bondTypeProperty = BondsObject::OOClass().createStandardStorage(numBonds, BondsObject::TypeProperty, false);
			frameData->addBondProperty(bondTypeProperty);
			ParticleFrameData::TypeList* bondTypeList = frameData->propertyTypesList(bondTypeProperty);
			for(int i = 0; i < bondTypeNames.size(); i++)
				bondTypeList->addTypeId(i, QString::fromUtf8(bondTypeNames[i]));

			// Read bond types.
			if(gsd.hasChunk("bonds/typeid", frameNumber)) {
				gsd.readIntArray("bonds/typeid", frameNumber, bondTypeProperty->dataInt(), numBonds);
			}
			else {
				std::fill(bondTypeProperty->dataInt(), bondTypeProperty->dataInt() + bondTypeProperty->size(), 0);
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
			gsd.readFloatArray(chunkName, frameNumber, prop->dataFloat(), numElements, prop->componentCount());
		else if(prop->dataType() == PropertyStorage::Int)
			gsd.readIntArray(chunkName, frameNumber, prop->dataInt(), numElements, prop->componentCount());
		else if(prop->dataType() == PropertyStorage::Int64)
			gsd.readIntArray(chunkName, frameNumber, prop->dataInt64(), numElements, prop->componentCount());
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
	else if(shapeType == "ConvexPolyhedron") {
		parseConvexPolyhedronShape(typeId, typeList, shapeSpec.object());
	}
	else if(shapeType == "Mesh") {
		parseMeshShape(typeId, typeList, shapeSpec.object());
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

	// Create the 'Aspherical Shape' particle property.
	PropertyPtr ashapeProperty = frameData->findStandardParticleProperty(ParticlesObject::AsphericalShapeProperty);
	if(!ashapeProperty) {
		ashapeProperty = ParticlesObject::OOClass().createStandardStorage(numParticles, ParticlesObject::AsphericalShapeProperty, true);
		frameData->addParticleProperty(ashapeProperty);
	}

	// Assigns the [a,b,c] values to those particles which are of the given type.
	PropertyPtr typeProperty = frameData->findStandardParticleProperty(ParticlesObject::TypeProperty);
	for(size_t i = 0; i < numParticles; i++) {
		if(typeProperty->getInt(i) == typeId)
			ashapeProperty->setVector3(i, abc);
	}
}

/******************************************************************************
* Parsing routine for 'ConvexPolyhedron' particle shape definitions.
******************************************************************************/
void GSDImporter::FrameLoader::parseConvexPolyhedronShape(int typeId, ParticleFrameData::TypeList* typeList, QJsonObject definition)
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

	// Convert half-edge mesh into a conventional triangle mesh for visualization.
	std::shared_ptr<TriMesh> triMesh = std::make_shared<TriMesh>();
	mesh.convertToTriMesh(*triMesh, false);
	if(triMesh->faceCount() == 0) {
		qWarning() << "GSD file reader: Convex hull construction did not produce a valid triangle mesh for particle type" << typeId;
		return;
	}

	// Render only sharp edges of the mesh in wireframe mode.
	triMesh->determineEdgeVisibility();

	// Assign shape to particle type.
	typeList->setTypeShape(typeId, std::move(triMesh));
}

/******************************************************************************
* Parsing routine for 'Mesh' particle shape definitions.
******************************************************************************/
void GSDImporter::FrameLoader::parseMeshShape(int typeId, ParticleFrameData::TypeList* typeList, QJsonObject definition)
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

	// Assign shape to particle type.
	typeList->setTypeShape(typeId, std::move(triMesh));
}


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
