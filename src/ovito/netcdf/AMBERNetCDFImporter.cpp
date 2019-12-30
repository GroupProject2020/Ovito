////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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

///////////////////////////////////////////////////////////////////////////////
//
//  This module implements import of AMBER-style NetCDF trajectory files.
//  For specification documents see <http://ambermd.org/netcdf/>.
//
//  Extensions to this specification are supported through OVITO's
//  file column to particle property mapping.
//
//  A LAMMPS dump style for this file format can be found at
//  <https://github.com/pastewka/lammps-netcdf>.
//
//  An ASE trajectory container is found in ase.io.netcdftrajectory.
//  <https://wiki.fysik.dtu.dk/ase/epydoc/ase.io.netcdftrajectory-module.html>.
//
//  Please contact Lars Pastewka <lars.pastewka@iwm.fraunhofer.de> for
//  questions and suggestions.
//
///////////////////////////////////////////////////////////////////////////////

#include <ovito/particles/Particles.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/core/utilities/concurrent/Future.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/io/FileSource.h>
#include "AMBERNetCDFImporter.h"

#include <3rdparty/netcdf_integration/NetCDFIntegration.h>
#include <netcdf.h>
#include <QtMath>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(AMBERNetCDFImporter);
DEFINE_PROPERTY_FIELD(AMBERNetCDFImporter, useCustomColumnMapping);
SET_PROPERTY_FIELD_LABEL(AMBERNetCDFImporter, useCustomColumnMapping, "Custom file column mapping");

// Convert full tensor to Voigt tensor
template<typename T>
void fullToVoigt(size_t particleCount, T *full, T *voigt) {
	for (size_t i = 0; i < particleCount; i++) {
		voigt[6*i] = full[9*i];
		voigt[6*i+1] = full[9*i+4];
		voigt[6*i+2] = full[9*i+8];
		voigt[6*i+3] = (full[9*i+5]+full[9*i+7])/2;
		voigt[6*i+4] = (full[9*i+2]+full[9*i+6])/2;
		voigt[6*i+5] = (full[9*i+1]+full[9*i+3])/2;
    }
}

/******************************************************************************
 * Sets the user-defined mapping between data columns in the input file and
 * the internal particle properties.
 *****************************************************************************/
void AMBERNetCDFImporter::setCustomColumnMapping(const InputColumnMapping& mapping)
{
	_customColumnMapping = mapping;
	notifyTargetChanged();
}

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool AMBERNetCDFImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Only serial access to NetCDF functions is allowed, because they are not thread-safe.
	NetCDFExclusiveAccess locker;

	// Check if we can open the input file for reading.
	int tmp_ncid;
	int err = nc_open(QDir::toNativeSeparators(input.fileName()).toLocal8Bit().constData(), NC_NOWRITE, &tmp_ncid);
	if(err == NC_NOERR) {

		// Particle data may be stored in a subgroup named "AMBER" instead of the root group.
		int amber_ncid = tmp_ncid;
		if(nc_inq_ncid(tmp_ncid, "AMBER", &amber_ncid) == NC_NOERR) {
			amber_ncid = amber_ncid;
		}

		// Make sure we have the right file conventions.
		size_t len;
		if(nc_inq_attlen(amber_ncid, NC_GLOBAL, "Conventions", &len) == NC_NOERR) {
			std::unique_ptr<char[]> conventions_str(new char[len+1]);
			if(nc_get_att_text(amber_ncid, NC_GLOBAL, "Conventions", conventions_str.get()) == NC_NOERR) {
				conventions_str[len] = '\0';
				if(strcmp(conventions_str.get(), "AMBER") == 0) {
					nc_close(tmp_ncid);
					return true;
				}
			}
		}

		nc_close(tmp_ncid);
	}

	return false;
}

/******************************************************************************
* Inspects the header of the given file and returns the number of file columns.
******************************************************************************/
Future<InputColumnMapping> AMBERNetCDFImporter::inspectFileHeader(const Frame& frame)
{
	// Retrieve file.
	return Application::instance()->fileManager()->fetchUrl(dataset()->container()->taskManager(), frame.sourceFile)
		.then(executor(), [this, frame](const QString& filename) {

			// Start task that inspects the file header to determine the contained data columns.
			FrameLoaderPtr inspectionTask = std::make_shared<FrameLoader>(frame, filename);
			return dataset()->container()->taskManager().runTaskAsync(inspectionTask)
				.then([](const FileSourceImporter::FrameDataPtr& frameData) {
					return static_cast<FrameData*>(frameData.get())->detectedColumnMapping();
				});
		});
}

/******************************************************************************
* Scans the given input file to find all contained simulation frames.
******************************************************************************/
void AMBERNetCDFImporter::FrameFinder::discoverFramesInFile(QFile& file, const QUrl& sourceUrl, QVector<FileSourceImporter::Frame>& frames)
{
	// Only serial access to NetCDF functions is allowed, because they are not thread-safe.
	NetCDFExclusiveAccess locker(this);
	if(!locker.isLocked()) return;

	QString filename = QDir::toNativeSeparators(file.fileName());

	// Open the input NetCDF file.
	int ncid;
	int root_ncid;
	NCERR( nc_open(filename.toLocal8Bit().constData(), NC_NOWRITE, &ncid) );
	root_ncid = ncid;

	// Particle data may be stored in a subgroup named "AMBER" instead of the root group.
	int amber_ncid;
	if (nc_inq_ncid(root_ncid, "AMBER", &amber_ncid) == NC_NOERR) {
		ncid = amber_ncid;
	}

	// Read number of frames.
	int frame_dim;
	NCERR( nc_inq_dimid(ncid, "frame", &frame_dim) );
	size_t nFrames;
	NCERR( nc_inq_dimlen(ncid, frame_dim, &nFrames) );
	NCERR( nc_close(root_ncid) );

	QFileInfo fileInfo(file.fileName());
	QDateTime lastModified = fileInfo.lastModified();
	for(size_t i = 0; i < nFrames; i++) {
		Frame frame;
		frame.sourceFile = sourceUrl;
		frame.byteOffset = 0;
		frame.lineNumber = i;
		frame.lastModificationTime = lastModified;
		frame.label = tr("Frame %1").arg(i);
		frames.push_back(frame);
	}
}

/******************************************************************************
* Open file if not already opened
******************************************************************************/
void AMBERNetCDFImporter::FrameLoader::openNetCDF(const QString &filename, FrameData* frameData)
{
	closeNetCDF();

	// Open the input file for reading.
	NCERR( nc_open(filename.toLocal8Bit().constData(), NC_NOWRITE, &_ncid) );
	_root_ncid = _ncid;
	_ncIsOpen = true;

	// Particle data may be stored in a subgroup named "AMBER" instead of the root group.
	int amber_ncid;
	if (nc_inq_ncid(_root_ncid, "AMBER", &amber_ncid) == NC_NOERR) {
		_ncid = amber_ncid;
	}

	// Make sure we have the right file conventions
	size_t len;
	NCERR( nc_inq_attlen(_ncid, NC_GLOBAL, "Conventions", &len) );
	std::unique_ptr<char[]> conventions_str(new char[len+1]);
	NCERR( nc_get_att_text(_ncid, NC_GLOBAL, "Conventions", conventions_str.get()) );
	conventions_str[len] = '\0';
	if (strcmp(conventions_str.get(), "AMBER"))
		throw Exception(tr("NetCDF file %1 follows '%2' conventions, expected 'AMBER'.").arg(filename, conventions_str.get()));

	// Read optional file title.
	if(nc_inq_attlen(_ncid, NC_GLOBAL, "title", &len) == NC_NOERR) {
		std::unique_ptr<char[]> title_str(new char[len+1]);
		NCERR( nc_get_att_text(_ncid, NC_GLOBAL, "title", title_str.get()) );
		title_str[len] = '\0';
		frameData->attributes().insert(QStringLiteral("NetCDF_Title"), QVariant::fromValue(QString::fromLocal8Bit(title_str.get())));
	}

	// Get dimensions
	NCERR( nc_inq_dimid(_ncid, "frame", &_frame_dim) );
	NCERR( nc_inq_dimid(_ncid, "atom", &_atom_dim) );
	NCERR( nc_inq_dimid(_ncid, "spatial", &_spatial_dim) );

	// Number of particles.
	size_t particleCount;
	NCERR( nc_inq_dimlen(_ncid, _atom_dim, &particleCount) );

	// Extensions used by the SimPARTIX program.
	// We only read particle properties from files that either contain SPH or DEM particles but not both.
	size_t sphParticleCount, demParticleCount;
	if (nc_inq_dimid(_ncid, "sph", &_sph_dim) != NC_NOERR || nc_inq_dimlen(_ncid, _sph_dim, &sphParticleCount) != NC_NOERR || sphParticleCount != particleCount)
		_sph_dim = -1;
	if (nc_inq_dimid(_ncid, "dem", &_dem_dim) != NC_NOERR || nc_inq_dimlen(_ncid, _dem_dim, &demParticleCount) != NC_NOERR || demParticleCount != particleCount)
		_dem_dim = -1;

	// Get some variables
	if (nc_inq_varid(_ncid, "cell_origin", &_cell_origin_var) != NC_NOERR)
		_cell_origin_var = -1;
	if (nc_inq_varid(_ncid, "cell_lengths", &_cell_lengths_var) != NC_NOERR)
		_cell_lengths_var = -1;
	if (nc_inq_varid(_ncid, "cell_angles", &_cell_angles_var) != NC_NOERR)
		_cell_angles_var = -1;
	if (nc_inq_varid(_ncid, "shear_dx", &_shear_dx_var) != NC_NOERR)
		_shear_dx_var = -1;
}

/******************************************************************************
* Close the current NetCDF file.
******************************************************************************/
void AMBERNetCDFImporter::FrameLoader::closeNetCDF()
{
	if (_ncIsOpen) {
		NCERR( nc_close(_root_ncid) );
		_ncid = -1;
		_root_ncid = -1;
		_ncIsOpen = false;
	}
}

/******************************************************************************
* Close the current NetCDF file.
******************************************************************************/
bool AMBERNetCDFImporter::FrameLoader::detectDims(int movieFrame, int particleCount, int nDims, int* dimIds, int& nDimsDetected, size_t& componentCount, size_t* startp, size_t* countp)
{
	if(nDims < 1) return false;

	nDimsDetected = 0;
	if(*dimIds == _frame_dim) {
		// This is a per-frame property
		*startp++ = movieFrame;
		*countp++ = 1;
		dimIds++;
		nDimsDetected++;
		nDims--;
	}
	if(nDims == 0 || nDims > 3) return false;
	if(*dimIds != _atom_dim && *dimIds != _sph_dim && *dimIds != _dem_dim) return false;

	*startp++ = 0;
	*countp++ = particleCount;
	nDimsDetected++;
	nDims--;
	dimIds++;
	componentCount = 1;

	// Is it a vector property?
	if(nDims >= 1) {
		size_t dimLength;
		NCERR(nc_inq_dimlen(_ncid, *dimIds, &dimLength));
		*startp++ = 0;
		*countp++ = dimLength;
		componentCount = dimLength;
		nDimsDetected++;
		dimIds++;

		// Is it a matrix property?
		if(nDims == 2) {
			// We map the matrix elements to a linear vector property in OVITO.
			NCERR(nc_inq_dimlen(_ncid, *dimIds, &dimLength));
			*startp++ = 0;
			*countp++ = dimLength;
			componentCount *= dimLength;
			nDimsDetected++;
		}
	}

	return true;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr AMBERNetCDFImporter::FrameLoader::loadFile(QFile& file)
{
	setProgressText(tr("Reading NetCDF file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Get frame number.
	size_t movieFrame = frame().lineNumber;

	// Create the destination container for loaded data.
	auto frameData = std::make_shared<FrameData>();

	// Only serial access to NetCDF functions is allowed, because they are not thread-safe.
	NetCDFExclusiveAccess locker(this);
	if(!locker.isLocked()) return {};

	try {
		openNetCDF(file.fileName(), frameData.get());

		// Scan NetCDF and iterate supported column names.
		InputColumnMapping columnMapping;

		// Now iterate over all variables and see whether they start with either atom or frame dimensions.
		int nVars;
		int coordinatesVar = -1;
		NCERR( nc_inq_nvars(_ncid, &nVars) );
		for(int varId = 0; varId < nVars; varId++) {
			char name[NC_MAX_NAME+1];
			nc_type type;

			// Retrieve NetCDF meta-information.
			int nDims, dimIds[NC_MAX_VAR_DIMS];
			NCERR( nc_inq_var(_ncid, varId, name, &type, &nDims, dimIds, NULL) );
			OVITO_ASSERT(nDims >= 1);

			int nDimsDetected;
			size_t componentCount;
			size_t startp[4];
			size_t countp[4];
			// Check if dimensions make sense and we can understand them.
			if(detectDims(movieFrame, 0, nDims, dimIds, nDimsDetected, componentCount, startp, countp)) {
				// Do we support this data type?
				if(type == NC_BYTE || type == NC_SHORT || type == NC_INT || type == NC_LONG) {
					columnMapping.push_back(mapVariableToColumn(name, PropertyStorage::Int, componentCount));
				}
				else if(type == NC_INT64) {
					columnMapping.push_back(mapVariableToColumn(name, PropertyStorage::Int64, componentCount));
				}
				else if(type == NC_FLOAT || type == NC_DOUBLE) {
					columnMapping.push_back(mapVariableToColumn(name, PropertyStorage::Float, componentCount));
					if(qstrcmp(name, "coordinates") == 0 || qstrcmp(name, "unwrapped_coordinates") == 0)
						coordinatesVar = varId;
				}
				else {
					qDebug() << "Skipping NetCDF variable " << name << " because data type is not known.";
				}
			}

			// Read in scalar values as attributes.
			if(nDims == 1 && dimIds[0] == _frame_dim) {
				if(type == NC_SHORT || type == NC_INT || type == NC_LONG) {
					size_t startp[2] = { movieFrame, 0 };
					size_t countp[2] = { 1, 1 };
					int value;
					NCERR( nc_get_vara_int(_ncid, varId, startp, countp, &value) );
					frameData->attributes().insert(QString::fromLocal8Bit(name), QVariant::fromValue(value));
				}
				else if(type == NC_INT64) {
					size_t startp[2] = { movieFrame, 0 };
					size_t countp[2] = { 1, 1 };
					qlonglong value;
					NCERR( nc_get_vara_longlong(_ncid, varId, startp, countp, &value) );
					frameData->attributes().insert(QString::fromLocal8Bit(name), QVariant::fromValue(value));
				}
				else if(type == NC_FLOAT || type == NC_DOUBLE) {
					size_t startp[2] = { movieFrame, 0 };
					size_t countp[2] = { 1, 1 };
					double value;
					NCERR( nc_get_vara_double(_ncid, varId, startp, countp, &value) );
					frameData->attributes().insert(QString::fromLocal8Bit(name), QVariant::fromValue(value));
				}
			}
		}
		if(coordinatesVar == -1)
			throw Exception(tr("NetCDF file contains no variable named 'coordinates' or 'unwrapped_coordinates'."));

		// Check if the only thing we need to do is read column information.
		if(_parseFileHeaderOnly) {
			frameData->detectedColumnMapping() = std::move(columnMapping);
			closeNetCDF();
			return frameData;
		}

		// Set up column-to-property mapping.
		if(_useCustomColumnMapping && !_customColumnMapping.empty())
			columnMapping = _customColumnMapping;

		// Total number of particles.
		size_t particleCount;
		NCERR( nc_inq_dimlen(_ncid, _atom_dim, &particleCount) );

		// Simulation cell. Note that cell_origin is an extension to the AMBER specification.
		double o[3] = { 0.0, 0.0, 0.0 };
		double l[3] = { 0.0, 0.0, 0.0 };
		double a[3] = { 90.0, 90.0, 90.0 };
		double d[3] = { 0.0, 0.0, 0.0 };
		size_t startp[4] = { movieFrame, 0, 0, 0 };
		size_t countp[4] = { 1, 3, 0, 0 };
		if(_cell_origin_var != -1)
			NCERR( nc_get_vara_double(_ncid, _cell_origin_var, startp, countp, o) );
		if(_cell_lengths_var != -1)
			NCERR( nc_get_vara_double(_ncid, _cell_lengths_var, startp, countp, l) );
		if(_cell_angles_var != -1)
			NCERR( nc_get_vara_double(_ncid, _cell_angles_var, startp, countp, a) );
		if(_shear_dx_var != -1)
			NCERR( nc_get_vara_double(_ncid, _shear_dx_var, startp, countp, d) );

		// Periodic boundary conditions. Non-periodic dimensions have length zero
		// according to AMBER specification.
		std::array<bool,3> pbc;
		bool isCellOrthogonal = true;
		for (int i = 0; i < 3; i++) {
			if (std::abs(l[i]) < 1e-12)  pbc[i] = false;
			else pbc[i] = true;
			if(std::abs(a[i] - 90.0) > 1e-12 || std::abs(d[i]) > 1e-12)
				isCellOrthogonal = false;
		}
		frameData->simulationCell().setPbcFlags(pbc);

		Vector3 va, vb, vc;
		if(isCellOrthogonal) {
			va = Vector3(l[0], 0, 0);
			vb = Vector3(0, l[1], 0);
			vc = Vector3(0, 0, l[2]);
		}
		else {
			// Express cell vectors va, vb and vc in the X,Y,Z-system
			a[0] = qDegreesToRadians(a[0]);
			a[1] = qDegreesToRadians(a[1]);
			a[2] = qDegreesToRadians(a[2]);
			double cosines[3];
			for(size_t i = 0; i < 3; i++)
				cosines[i] = (std::abs(a[i] - qRadiansToDegrees(90.0)) > 1e-12) ? cos(a[i]) : 0.0;
			va = Vector3(l[0], 0, 0);
			vb = Vector3(l[1]*cosines[2], l[1]*sin(a[2]), 0);
			double cx = cosines[1];
			double cy = (cosines[0] - cx*cosines[2])/sin(a[2]);
			double cz = sqrt(1. - cx*cx - cy*cy);
			vc = Vector3(l[2]*cx+d[0], l[2]*cy+d[1], l[2]*cz);
		}
		frameData->simulationCell().setMatrix(AffineTransformation(va, vb, vc, Vector3(o[0], o[1], o[2])));

		// Report to user.
		beginProgressSubSteps(columnMapping.size());

		// We inpsect the particle coordinate array in the NetCDF first before any properties are loaded
		// in order to determine the number of particles (which might actually be lower than the size of the "atoms" dimension).

		// Retrieve NetCDF variable meta-information.
		int nDims, dimIds[NC_MAX_VAR_DIMS];
		NCERR( nc_inq_var(_ncid, coordinatesVar, NULL, NULL, &nDims, dimIds, NULL) );

		// Detect dims
		int nDimsDetected;
		size_t componentCount;
		if(detectDims(movieFrame, particleCount, nDims, dimIds, nDimsDetected, componentCount, startp, countp)) {
			auto data = std::make_unique<FloatType[]>(componentCount * particleCount);
#ifdef FLOATTYPE_FLOAT
			NCERRI( nc_get_vara_float(_ncid, coordinatesVar, startp, countp, data.get()), tr("(While reading variable 'coordinates'.)"));
			while(particleCount > 0 && data[componentCount * (particleCount-1)] == NC_FILL_FLOAT) particleCount--;
#else
			NCERRI( nc_get_vara_double(_ncid, coordinatesVar, startp, countp, data.get()), tr("(While reading variable 'coordinates'.)"));
			while(particleCount > 0 && data[componentCount * (particleCount-1)] == NC_FILL_DOUBLE) particleCount--;
#endif
		}

		// Now iterate over all NetCDF variables and load the appropriate frame data.
		for(const InputColumnInfo& column : columnMapping) {
			if(isCanceled()) {
				closeNetCDF();
				return {};
			}
			if(&column != &columnMapping.front())
				nextProgressSubStep();

			PropertyPtr property;
			QString columnName = column.columnName;
			QString propertyName = column.property.name();
			int dataType = column.dataType;
			if(dataType == QMetaType::Void) continue;

			if(dataType != PropertyStorage::Int && dataType != PropertyStorage::Int64 && dataType != PropertyStorage::Float)
				throw Exception(tr("Invalid custom particle property (data type %1) for input file column '%2' of NetCDF file.").arg(dataType).arg(columnName));

			// Retrieve NetCDF variable meta-information.
			nc_type type;
			int varId;
			NCERR( nc_inq_varid(_ncid, columnName.toLocal8Bit().constData(), &varId) );
			NCERR( nc_inq_var(_ncid, varId, NULL, &type, &nDims, dimIds, NULL) );
			if(nDims == 0) continue;

			// Construct pointers to NetCDF dimension indices.
			if(!detectDims(movieFrame, particleCount, nDims, dimIds, nDimsDetected, componentCount, startp, countp))
				continue;

			// Create property to load this information into.
			ParticlesObject::Type propertyType = (ParticlesObject::Type)column.property.type();
			if(propertyType != ParticlesObject::UserProperty) {
				// Look for existing standard property.
				property = frameData->findStandardParticleProperty(propertyType);
				if(!property) {
					// Create standard property.
					property = ParticlesObject::OOClass().createStandardStorage(particleCount, propertyType, true);
					frameData->addParticleProperty(property);
				}
			}
			else {
				// Look for existing user-defined property with the same name.
				property = frameData->findParticleProperty(propertyName);
				// Discard existing property storage if is has the wrong data type or component count.
				if(property && (property->dataType() != dataType || property->componentCount() != componentCount)) {
					frameData->removeParticleProperty(property);
					property.reset();
				}
				if(!property) {
					// Create a new user-defined property for the column.
					property = std::make_shared<PropertyStorage>(particleCount, dataType, componentCount, 0, propertyName, true);
					frameData->addParticleProperty(property);
				}
			}
			OVITO_ASSERT(property != nullptr);
			property->setName(propertyName);

			// Make sure the dimensions match.
			bool doVoigtConversion = false;
			if(componentCount != property->componentCount()) {
				// For standard particle properties describing symmetric tensors in Voigt notion, we perform automatic
				// conversion from the 3x3 full tensors stored in the NetCDF file.
				if(componentCount == 9 && property->componentCount() == 6 && property->dataType() == PropertyStorage::Float) {
					doVoigtConversion = true;
				}
				else {
					throw Exception(tr("NetCDF data array '%1' with %2 components cannot be mapped to OVITO particle property '%3', which consists of %4 components.")
						.arg(columnName).arg(componentCount).arg(propertyName).arg(property->componentCount()));
				}
			}

			if(property->dataType() == PropertyStorage::Int) {
				// Read integer property data in chunks so that we can report I/O progress.
				size_t totalCount = countp[1];
				size_t remaining = totalCount;
				countp[1] = 1000000;
				setProgressMaximum(totalCount / countp[1] + 1);
				OVITO_ASSERT(totalCount <= property->size());
				PropertyAccess<int,true> propertyArray(property);
				for(size_t chunk = 0; chunk < totalCount; chunk += countp[1], startp[1] += countp[1]) {
					countp[1] = std::min(countp[1], remaining);
					remaining -= countp[1];
					OVITO_ASSERT(countp[1] >= 1);
					NCERRI( nc_get_vara_int(_ncid, varId, startp, countp, propertyArray.begin() + chunk * property->componentCount()), tr("(While reading variable '%1'.)").arg(columnName) );
					if(!incrementProgressValue()) {
						closeNetCDF();
						return {};
					}
				}
				OVITO_ASSERT(remaining == 0);

				// Create particles types if this is the particle type property.
				if(propertyType == ParticlesObject::TypeProperty || propertyType == ParticlesObject::StructureTypeProperty) {

					ParticleFrameData::TypeList* typeList = frameData->propertyTypesList(property);

					// Create particle types.
					for(int ptype : ConstPropertyAccess<int>(property)) {
						typeList->addTypeId(ptype);
					}

					// Since we created particle types on the go while reading the particles, the assigned particle type IDs
					// depend on the storage order of particles in the file. We rather want a well-defined particle type ordering, that's
					// why we sort them now according to their numeric IDs.
					typeList->sortTypesById();
				}
			}
			else if(property->dataType() == PropertyStorage::Int64) {
				// Read 64-bit integer property data in chunks so that we can report I/O progress.
				size_t totalCount = countp[1];
				size_t remaining = totalCount;
				countp[1] = 1000000;
				setProgressMaximum(totalCount / countp[1] + 1);
				OVITO_ASSERT(totalCount <= property->size());
				PropertyAccess<qlonglong,true> propertyArray(property);
				for(size_t chunk = 0; chunk < totalCount; chunk += countp[1], startp[1] += countp[1]) {
					countp[1] = std::min(countp[1], remaining);
					remaining -= countp[1];
					OVITO_ASSERT(countp[1] >= 1);
					NCERRI( nc_get_vara_longlong(_ncid, varId, startp, countp, propertyArray.begin() + chunk * property->componentCount()), tr("(While reading variable '%1'.)").arg(columnName) );
					if(!incrementProgressValue()) {
						closeNetCDF();
						return {};
					}
				}
				OVITO_ASSERT(remaining == 0);
			}
			else if(property->dataType() == PropertyStorage::Float) {
				PropertyAccess<FloatType,true> propertyArray(property);

				// Special handling for tensor arrays that need to be converted to Voigt notation.
				if(doVoigtConversion) {
					auto data = std::make_unique<FloatType[]>(9 * particleCount);
#ifdef FLOATTYPE_FLOAT
					NCERRI( nc_get_vara_float(_ncid, varId, startp, countp, data.get()), tr("(While reading variable '%1'.)").arg(columnName) );
#else
					NCERRI( nc_get_vara_double(_ncid, varId, startp, countp, data.get()), tr("(While reading variable '%1'.)").arg(columnName) );
#endif
					fullToVoigt(particleCount, data.get(), propertyArray.begin());
				}
				else {

					// Read property data in chunks so that we can report I/O progress.
					size_t totalCount = countp[1];
					size_t remaining = totalCount;
					countp[1] = 1000000;
					setProgressMaximum(totalCount / countp[1] + 1);
					for(size_t chunk = 0; chunk < totalCount; chunk += countp[1], startp[1] += countp[1]) {
						countp[1] = std::min(countp[1], remaining);
						remaining -= countp[1];
						OVITO_ASSERT(countp[1] >= 1);
#ifdef FLOATTYPE_FLOAT
						NCERRI( nc_get_vara_float(_ncid, varId, startp, countp, propertyArray.begin() + chunk * property->componentCount()), tr("(While reading variable '%1'.)").arg(columnName) );
#else
						NCERRI( nc_get_vara_double(_ncid, varId, startp, countp, propertyArray.begin() + chunk * property->componentCount()), tr("(While reading variable '%1'.)").arg(columnName) );
#endif
						if(!incrementProgressValue()) {
							closeNetCDF();
							return {};
						}
					}
				}
			}
			else {
				qDebug() << "Warning: Skipping field '" << columnName << "' of NetCDF file because it has an unrecognized data type.";
			}
		}

		endProgressSubSteps();

		// If the input file does not contain simulation cell size, use bounding box of particles as simulation cell.
		if(!pbc[0] || !pbc[1] || !pbc[2]) {

			ConstPropertyAccess<Point3> posProperty = frameData->findStandardParticleProperty(ParticlesObject::PositionProperty);
			if(posProperty && posProperty.size() != 0) {
				Box3 boundingBox;
				boundingBox.addPoints(posProperty);

				AffineTransformation cell = frameData->simulationCell().matrix();
				for(size_t dim = 0; dim < 3; dim++) {
					if(!pbc[dim]) {
						cell.column(3)[dim] = boundingBox.minc[dim];
						cell.column(dim).setZero();
						cell.column(dim)[dim] = boundingBox.maxc[dim] - boundingBox.minc[dim];
					}
				}
				frameData->simulationCell().setMatrix(cell);
			}
		}

		closeNetCDF();

		// Sort particles by ID if requested.
		if(_sortParticles)
			frameData->sortParticlesById();

		frameData->setStatus(tr("Loaded %1 particles").arg(particleCount));
		return frameData;
	}
	catch(...) {
		closeNetCDF();
		throw;
	}
}

/******************************************************************************
 * Guesses the mapping of a NetCDF variable to one of OVITO's particle properties.
 *****************************************************************************/
InputColumnInfo AMBERNetCDFImporter::mapVariableToColumn(const QString& name, int dataType, size_t componentCount)
{
	ParticlesObject::Type standardType = ParticlesObject::UserProperty;

	// Map variables of the AMBER convention and some more to OVITO's standard properties.
	QString loweredName = name.toLower();
	if(loweredName == "coordinates" || loweredName == "unwrapped_coordinates") standardType = ParticlesObject::PositionProperty;
	else if(loweredName == "velocities") standardType = ParticlesObject::VelocityProperty;
	else if(loweredName == "id" || loweredName == "identifier") standardType = ParticlesObject::IdentifierProperty;
	else if(loweredName == "type" || loweredName == "element" || loweredName == "atom_types" || loweredName == "species") standardType = ParticlesObject::TypeProperty;
	else if(loweredName == "mass") standardType = ParticlesObject::MassProperty;
	else if(loweredName == "radius") standardType = ParticlesObject::RadiusProperty;
	else if(loweredName == "color") standardType = ParticlesObject::ColorProperty;
	else if(loweredName == "c_cna" || loweredName == "pattern") standardType = ParticlesObject::StructureTypeProperty;
	else if(loweredName == "c_epot") standardType = ParticlesObject::PotentialEnergyProperty;
	else if(loweredName == "c_kpot") standardType = ParticlesObject::KineticEnergyProperty;
	else if(loweredName == "selection") standardType = ParticlesObject::SelectionProperty;
	else if(loweredName == "forces" || loweredName == "force") standardType = ParticlesObject::ForceProperty;

	// Try to directly map variable name to a standard property name.
	if(standardType == ParticlesObject::UserProperty)
		standardType = (ParticlesObject::Type)ParticlesObject::OOClass().standardPropertyTypeId(name);

	InputColumnInfo column;
	column.columnName = name;

	// Only map to standard property if data layout matches.
	if(standardType != ParticlesObject::UserProperty) {
		if(componentCount == ParticlesObject::OOClass().standardPropertyComponentCount(standardType)) {
			column.mapStandardColumn(standardType);
			return column;
		}
	}

	column.mapCustomColumn(name, dataType);
	return column;
}

/******************************************************************************
 * Saves the class' contents to the given stream.
 *****************************************************************************/
void AMBERNetCDFImporter::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	ParticleImporter::saveToStream(stream, excludeRecomputableData);

	stream.beginChunk(0x01);
	_customColumnMapping.saveToStream(stream);
	stream.endChunk();
}

/******************************************************************************
 * Loads the class' contents from the given stream.
 *****************************************************************************/
void AMBERNetCDFImporter::loadFromStream(ObjectLoadStream& stream)
{
	ParticleImporter::loadFromStream(stream);

	stream.expectChunk(0x01);
	_customColumnMapping.loadFromStream(stream);
	stream.closeChunk();
}

/******************************************************************************
 * Creates a copy of this object.
 *****************************************************************************/
OORef<RefTarget> AMBERNetCDFImporter::clone(bool deepCopy, CloneHelper& cloneHelper) const
{
	// Let the base class create an instance of this class.
	OORef<AMBERNetCDFImporter> clone = static_object_cast<AMBERNetCDFImporter>(ParticleImporter::clone(deepCopy, cloneHelper));
	clone->_customColumnMapping = this->_customColumnMapping;
	return clone;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
