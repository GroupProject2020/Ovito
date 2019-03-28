///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include <plugins/mesh/tri/TriMeshObject.h>
#include <plugins/mesh/tri/TriMeshVis.h>
#include <core/app/Application.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include <core/dataset/io/FileSource.h>
#include "TriMeshFrameData.h"

namespace Ovito { namespace Mesh {

/******************************************************************************
* Inserts the loaded data into the provided container object.
* This function is called by the system from the main thread after the
* asynchronous loading task has finished.
******************************************************************************/
OORef<DataCollection> TriMeshFrameData::handOver(const DataCollection* existing, bool isNewFile, FileSource* fileSource)
{
	OORef<DataCollection> output = new DataCollection(fileSource->dataset());

	// Create a TriMeshObject or reuse existing.
	TriMeshObject* triMeshObj = const_cast<TriMeshObject*>(existing ? existing->getObject<TriMeshObject>() : nullptr);
	if(triMeshObj) {
		output->addObject(triMeshObj);
	}
	else {
		triMeshObj = output->createObject<TriMeshObject>(fileSource);

		// Assign a TriMeshVis to the TriMeshObject.
		OORef<TriMeshVis> triMeshVis = new TriMeshVis(fileSource->dataset());
		if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
			triMeshVis->loadUserDefaults();
		triMeshObj->setVisElement(triMeshVis);
	}

	// Hand over the loaded mesh data.
	triMeshObj->mesh().swap(mesh());

	return output;
}

}	// End of namespace
}	// End of namespace
