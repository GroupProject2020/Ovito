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

#include <ovito/mesh/Mesh.h>
#include <ovito/mesh/tri/TriMeshObject.h>
#include <ovito/mesh/tri/TriMeshVis.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/dataset/io/FileSource.h>
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
	triMeshObj->setMesh(_mesh);

	return output;
}

}	// End of namespace
}	// End of namespace
