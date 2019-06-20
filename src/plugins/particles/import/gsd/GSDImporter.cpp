///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <plugins/particles/import/ParticleFrameData.h>
#include "GSDImporter.h"
#include "GSDFile.h"

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
	QStringList particleTypeNames = gsd.readStringTable("particles/types", frameNumber);
	if(particleTypeNames.empty())
		particleTypeNames.push_back(QStringLiteral("A"));

	// Read particle positions.
	PropertyPtr posProperty = ParticlesObject::OOClass().createStandardStorage(numParticles, ParticlesObject::PositionProperty, false);
	frameData->addParticleProperty(posProperty);
	gsd.readFloatArray("particles/position", frameNumber, posProperty->dataPoint3(), numParticles, posProperty->componentCount());

	// Create particle types.
	PropertyPtr typeProperty = ParticlesObject::OOClass().createStandardStorage(numParticles, ParticlesObject::TypeProperty, false);
	frameData->addParticleProperty(typeProperty);
	ParticleFrameData::TypeList* typeList = frameData->propertyTypesList(typeProperty);
	for(int i = 0; i < particleTypeNames.size(); i++)
		typeList->addTypeId(i, particleTypeNames[i]);

	// Read particle types.
	if(gsd.hasChunk("particles/typeid", frameNumber))
		gsd.readIntArray("particles/typeid", frameNumber, typeProperty->dataInt(), numParticles);
	else
		std::fill(typeProperty->dataInt(), typeProperty->dataInt() + typeProperty->size(), 0);

	readOptionalParticleProperty(gsd, "particles/mass", frameNumber, numParticles, ParticlesObject::MassProperty, frameData);
	readOptionalParticleProperty(gsd, "particles/charge", frameNumber, numParticles, ParticlesObject::ChargeProperty, frameData);
	readOptionalParticleProperty(gsd, "particles/velocity", frameNumber, numParticles, ParticlesObject::VelocityProperty, frameData);
	readOptionalParticleProperty(gsd, "particles/image", frameNumber, numParticles, ParticlesObject::PeriodicImageProperty, frameData);
	PropertyStorage* radiusProperty = readOptionalParticleProperty(gsd, "particles/diameter", frameNumber, numParticles, ParticlesObject::RadiusProperty, frameData);
	if(radiusProperty) {
		// Convert particle diameter to radius by dividing by 2.
		std::for_each(radiusProperty->dataFloat(), radiusProperty->dataFloat() + radiusProperty->size(), [](FloatType& r) { r /= 2; });
	}
	PropertyStorage* orientationProperty = readOptionalParticleProperty(gsd, "particles/orientation", frameNumber, numParticles, ParticlesObject::OrientationProperty, frameData);
	if(orientationProperty) {
		// Convert quaternion representation from GSD format to internal format.
		// Left-shift all quaternion components by one: (W,X,Y,Z) -> (X,Y,Z,W).
		std::for_each(orientationProperty->dataQuaternion(), orientationProperty->dataQuaternion() + orientationProperty->size(), [](Quaternion& q) {
			std::rotate(q.begin(), q.begin() + 1, q.end());
		});
	}

	// Parse number of bonds.
	uint32_t numBonds = gsd.readOptionalScalar<uint32_t>("bonds/N", frameNumber, 0);
	if(numBonds != 0) {
		// Read bond list.
		std::vector<int> bondList(numBonds * 2);
		gsd.readIntArray("bonds/group", frameNumber, bondList.data(), numBonds, 2);

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

		// Read bond types.
		if(gsd.hasChunk("bonds/types", frameNumber)) {

			// Parse list of bond type names.
			QStringList bondTypeNames = gsd.readStringTable("bonds/types", frameNumber);
			if(bondTypeNames.empty())
				bondTypeNames.push_back(QStringLiteral("A"));

			// Create bond types.
			PropertyPtr bondTypeProperty = BondsObject::OOClass().createStandardStorage(numBonds, BondsObject::TypeProperty, false);
			frameData->addBondProperty(bondTypeProperty);
			ParticleFrameData::TypeList* bondTypeList = frameData->propertyTypesList(bondTypeProperty);
			for(int i = 0; i < bondTypeNames.size(); i++)
				bondTypeList->addTypeId(i, bondTypeNames[i]);

			// Read bond types.
			if(gsd.hasChunk("bonds/typeid", frameNumber)) {
				gsd.readIntArray("bonds/typeid", frameNumber, bondTypeProperty->dataInt(), numBonds);
			}
			else {
				std::fill(bondTypeProperty->dataInt(), bondTypeProperty->dataInt() + bondTypeProperty->size(), 0);
			}
		}
	}

	QString statusString = tr("Number of particles: %1").arg(numParticles);
	if(numBonds != 0)
		statusString += tr("\nNumber of bonds: %1").arg(numBonds);
	frameData->setStatus(statusString);
	return frameData;
}

/******************************************************************************
* Reads the values of a particle property from the GSD file.
******************************************************************************/
PropertyStorage* GSDImporter::FrameLoader::readOptionalParticleProperty(GSDFile& gsd, const char* chunkName, uint64_t frameNumber, uint32_t numParticles, ParticlesObject::Type propertyType, const std::shared_ptr<ParticleFrameData>& frameData)
{
	if(gsd.hasChunk(chunkName, frameNumber)) {
		PropertyPtr prop = ParticlesObject::OOClass().createStandardStorage(numParticles, propertyType, false);
		frameData->addParticleProperty(prop);
		if(prop->dataType() == PropertyStorage::Float)
			gsd.readFloatArray(chunkName, frameNumber, prop->dataFloat(), numParticles, prop->componentCount());
		else if(prop->dataType() == PropertyStorage::Int)
			gsd.readIntArray(chunkName, frameNumber, prop->dataInt(), numParticles, prop->componentCount());
		else
			throw Exception(tr("Particle property '%1' cannot be read from GSD file, because it has an unsupported data type.").arg(prop->name()));
		return prop.get();
	}
	else return nullptr;
}


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
