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
#include <ovito/particles/import/ParticleFrameData.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include "CIFImporter.h"

#include <3rdparty/gemmi/cif.hpp>
#include <3rdparty/gemmi/smcif.hpp>	// for reading small molecules

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

namespace cif = gemmi::cif;

IMPLEMENT_OVITO_CLASS(CIFImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool CIFImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	CompressedTextReader stream(file);

	// Read the first N lines of the file which are not comments.
	int maxLines = 12;
	bool foundData = false;
	for(int i = 0; i < maxLines && !stream.eof(); i++) {
		// Note: Maximum line length of CIF files is 2048 characters.
		const char* line = stream.readLine(2048);

		if(stream.lineStartsWith("#", true)) {
			maxLines++;
			continue;
		}
		else if(stream.lineStartsWith("data_", false)) {
			// Make sure the "data_XXX" block appears.
			foundData = true;
		}
		else if(stream.lineStartsWith("_", false)) {
			// Make sure at least one "_XXX" block appears.
			return foundData;
		}
	}
	return false;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr CIFImporter::FrameLoader::loadFile()
{
	// Open file for reading.
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Reading CIF file %1").arg(fileHandle().toString()));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset, frame().lineNumber);

	// Create the destination container for loaded data.
	std::shared_ptr<ParticleFrameData> frameData = std::make_shared<ParticleFrameData>();

	// Map the whole file into memory for parsing.
	const char* buffer_start;
	const char* buffer_end;
	std::tie(buffer_start, buffer_end) = stream.mmap();
	if(!buffer_start)
		throw Exception(tr("Could not map CIF file into memory."));

	try {
		// Parse the CIF file's contents.
		cif::Document doc = cif::read_memory(buffer_start, buffer_end - buffer_start, qPrintable(frame().sourceFile.path()));

		// Unmap the input file from memory.
		stream.munmap();
		if(isCanceled()) return {};

		// Parse the CIF data into an atomic structure representation.
		const cif::Block& block = doc.sole_block();
		gemmi::AtomicStructure structure = gemmi::make_atomic_structure_from_block(block);
		if(isCanceled()) return {};

		// Parse list of atomic sites.
		std::vector<gemmi::AtomicStructure::Site> sites = structure.get_all_unit_cell_sites();
		PropertyAccess<Point3> posProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(sites.size(), ParticlesObject::PositionProperty, false));
		PropertyAccess<int> typeProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(sites.size(), ParticlesObject::TypeProperty, false));
		ParticleFrameData::TypeList* typeList = frameData->createPropertyTypesList(typeProperty);
		Point3* posIter = posProperty.begin();
		int* typeIter = typeProperty.begin();
		bool hasOccupancy = false;
		for(const gemmi::AtomicStructure::Site& site : sites) {
			gemmi::Position pos = structure.cell.orthogonalize(site.fract.wrap_to_unit());
			posIter->x() = pos.x;
			posIter->y() = pos.y;
			posIter->z() = pos.z;
			++posIter;
			*typeIter++ = typeList->addTypeName(site.type_symbol.empty() ? site.label.c_str() : site.type_symbol.c_str());
			if(site.occ != 1) hasOccupancy = true;
		}
		if(isCanceled()) return {};

		// Parse the optional site occupancy information.
		if(hasOccupancy) {
			PropertyAccess<FloatType> occupancyProperty = frameData->addParticleProperty(std::make_shared<PropertyStorage>(sites.size(), PropertyStorage::Float, 1, 0, QStringLiteral("Occupancy"), false));
			FloatType* occupancyIter = occupancyProperty.begin();
			for(const gemmi::AtomicStructure::Site& site : sites) {
				*occupancyIter++ = site.occ;
			}
		}

		// Since we created particle types on the go while reading the particles, the assigned particle type IDs
		// depend on the storage order of particles in the file We rather want a well-defined particle type ordering, that's
		// why we sort them now.
		typeList->sortTypesByName(typeProperty);

		// Parse unit cell.
		if(structure.cell.is_crystal()) {
			// Process periodic unit cell definition.
			AffineTransformation cell = AffineTransformation::Identity();
			if(structure.cell.alpha == 90 && structure.cell.beta == 90 && structure.cell.gamma == 90) {
				cell(0,0) = structure.cell.a;
				cell(1,1) = structure.cell.b;
				cell(2,2) = structure.cell.c;
			}
			else if(structure.cell.alpha == 90 && structure.cell.beta == 90) {
				FloatType gamma = qDegreesToRadians(structure.cell.gamma);
				cell(0,0) = structure.cell.a;
				cell(0,1) = structure.cell.b * std::cos(gamma);
				cell(1,1) = structure.cell.b * std::sin(gamma);
				cell(2,2) = structure.cell.c;
			}
			else {
				FloatType alpha = qDegreesToRadians(structure.cell.alpha);
				FloatType beta = qDegreesToRadians(structure.cell.beta);
				FloatType gamma = qDegreesToRadians(structure.cell.gamma);
				FloatType v = structure.cell.a * structure.cell.b * structure.cell.c * sqrt(1.0 - std::cos(alpha)*std::cos(alpha) - std::cos(beta)*std::cos(beta) - std::cos(gamma)*std::cos(gamma) + 2.0 * std::cos(alpha) * std::cos(beta) * std::cos(gamma));
				cell(0,0) = structure.cell.a;
				cell(0,1) = structure.cell.b * std::cos(gamma);
				cell(1,1) = structure.cell.b * std::sin(gamma);
				cell(0,2) = structure.cell.c * std::cos(beta);
				cell(1,2) = structure.cell.c * (std::cos(alpha) - std::cos(beta)*std::cos(gamma)) / std::sin(gamma);
				cell(2,2) = v / (structure.cell.a * structure.cell.b * std::sin(gamma));
			}
			frameData->simulationCell().setMatrix(cell);
		}
		else if(posProperty.size() != 0) {
			// Use bounding box of atomic coordinates as non-periodic simulation cell.
			Box3 boundingBox;
			boundingBox.addPoints(posProperty);
			frameData->simulationCell().setPbcFlags(false, false, false);
			frameData->simulationCell().setMatrix(AffineTransformation(
					Vector3(boundingBox.sizeX(), 0, 0),
					Vector3(0, boundingBox.sizeY(), 0),
					Vector3(0, 0, boundingBox.sizeZ()),
					boundingBox.minc - Point3::Origin()));
		}

		frameData->setStatus(tr("Number of atoms: %1").arg(posProperty.size()));
	}
	catch(const std::exception& e) {
		throw Exception(tr("CIF file reader: %1").arg(e.what()));
	}

	return frameData;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
