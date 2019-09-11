///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <ovito/grid/Grid.h>
#include "VTKVoxelGridExporter.h"

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(VTKVoxelGridExporter);

/******************************************************************************
 * This is called once for every output file to be written and before
 * exportData() is called.
 *****************************************************************************/
bool VTKVoxelGridExporter::openOutputFile(const QString& filePath, int numberOfFrames, AsyncOperation& operation)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	OVITO_ASSERT(!_outputStream);

	_outputFile.setFileName(filePath);
	_outputStream.reset(new CompressedTextWriter(_outputFile, dataset()));

	return true;
}

/******************************************************************************
 * This is called once for every output file written after exportData()
 * has been called.
 *****************************************************************************/
void VTKVoxelGridExporter::closeOutputFile(bool exportCompleted)
{
	_outputStream.reset();
	if(_outputFile.isOpen())
		_outputFile.close();

	if(!exportCompleted)
		_outputFile.remove();
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool VTKVoxelGridExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation)
{
	// Evaluate pipeline.
	const PipelineFlowState& state = getPipelineDataToBeExported(time, operation);
	if(operation.isCanceled())
		return false;

	// Look up the VoxelGrid to be exported in the pipeline state.
	DataObjectReference objectRef(&VoxelGrid::OOClass(), dataObjectToExport().dataPath());
	const VoxelGrid* voxelGrid = static_object_cast<VoxelGrid>(state.getLeafObject(objectRef));
	if(!voxelGrid) {
		throwException(tr("The pipeline output does not contain the voxel grid to be exported (animation frame: %1; object key: %2). Available grid keys: (%3)")
			.arg(frameNumber).arg(objectRef.dataPath()).arg(getAvailableDataObjectList(state, VoxelGrid::OOClass())));
	}

	// Make sure the data structure to be exported is consistent.
	voxelGrid->verifyIntegrity();

	operation.setProgressText(tr("Writing file %1").arg(filePath));

	auto dims = voxelGrid->shape();
	textStream() << "# vtk DataFile Version 3.0\n";
	textStream() << "# Voxel grid data - written by " << QCoreApplication::applicationName() << " " << QCoreApplication::applicationVersion() << "\n";
	textStream() << "ASCII\n";
	textStream() << "DATASET STRUCTURED_POINTS\n";
	textStream() << "DIMENSIONS " << dims[0] << " " << dims[1] << " " << dims[2] << "\n";
	if(const SimulationCellObject* domain = voxelGrid->domain()) {
		textStream() << "ORIGIN " << domain->cellOrigin().x() << " " << domain->cellOrigin().y() << " " << domain->cellOrigin().z() << "\n";
		textStream() << "SPACING";
		textStream() << " " << domain->cellVector1().length() / std::max(dims[0], (size_t)1);
		textStream() << " " << domain->cellVector2().length() / std::max(dims[1], (size_t)1);
		textStream() << " " << domain->cellVector3().length() / std::max(dims[2], (size_t)1);
		textStream() << "\n";
	}
	else {
		textStream() << "ORIGIN 0 0 0\n";
		textStream() << "SPACING 1 1 1\n";
	}
	textStream() << "POINT_DATA " << voxelGrid->elementCount() << "\n";

	for(const PropertyObject* prop : voxelGrid->properties()) {
		if(prop->dataType() == PropertyStorage::Int || prop->dataType() == PropertyStorage::Int64 || prop->dataType() == PropertyStorage::Float) {

			// Write header of data field.
			QString dataName = prop->name();
			dataName.remove(QChar(' '));
			if(prop->dataType() == PropertyStorage::Float && prop->componentCount() == 3) {
				textStream() << "\nVECTORS " << dataName << " double\n";
			}
			else if(prop->componentCount() <= 4) {
				if(prop->dataType() == PropertyStorage::Int)
					textStream() << "\nSCALARS " << dataName << " int " << prop->componentCount() << "\n";
				else if(prop->dataType() == PropertyStorage::Int64)
					textStream() << "\nSCALARS " << dataName << " long " << prop->componentCount() << "\n";
				else
					textStream() << "\nSCALARS " << dataName << " double " << prop->componentCount() << "\n";
				textStream() << "LOOKUP_TABLE default\n";
			}
			else continue; // VTK format supports only between 1 and 4 vector components. Skipping properties with more components during export.

			// Write payload data.
			size_t cmpnts = prop->componentCount();
			OVITO_ASSERT(prop->stride() == prop->dataTypeSize() * cmpnts);
			auto data_f = (prop->dataType() == PropertyStorage::Float) ? prop->constDataFloat() : nullptr;
			auto data_i = (prop->dataType() == PropertyStorage::Int) ? prop->constDataInt() : nullptr;
			auto data_i64 = (prop->dataType() == PropertyStorage::Int64) ? prop->constDataInt64() : nullptr;
			for(size_t row = 0; row < dims[1]*dims[2]; row++) {
				if(operation.isCanceled())
					return false;
				for(size_t col = 0; col < dims[0]; col++) {
					for(size_t c = 0; c < cmpnts; c++) {
						if(data_f)
							textStream() << *data_f++ << " ";
						else if(data_i)
							textStream() << *data_i++ << " ";
						else if(data_i64)
							textStream() << *data_i64++ << " ";
					}
				}
				textStream() << "\n";
			}
		}
	}

	return !operation.isCanceled();
}

}	// End of namespace
}	// End of namespace
