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

#include <ovito/mesh/Mesh.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/mesh/surface/SurfaceMeshData.h>
#include "SurfaceMeshDeleteSelectedModifierDelegate.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshRegionsDeleteSelectedModifierDelegate);

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> SurfaceMeshRegionsDeleteSelectedModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
	// Gather list of all surface mesh regions in the input data collection.
	QVector<DataObjectReference> objects;
	for(const ConstDataObjectPath& path : input.getObjectsRecursive(SurfaceMeshRegions::OOClass())) {
		objects.push_back(path);
	}
	return objects;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus SurfaceMeshRegionsDeleteSelectedModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	size_t numRegions = 0;
	size_t numSelected = 0;

	for(const DataObject* obj : state.data()->objects()) {
		if(const SurfaceMesh* existingSurface = dynamic_object_cast<SurfaceMesh>(obj)) {
			// Make sure the input mesh data structure is valid. 
			existingSurface->verifyMeshIntegrity();

			// Check if there is a region selection set.
			const PropertyObject* selectionProperty = existingSurface->regions()->getProperty(SurfaceMeshRegions::SelectionProperty);
			if(!selectionProperty) continue; // Nothing to do if there is no selection.

			// Check if at least one mesh region is currently selected.
			if(boost::algorithm::all_of(selectionProperty->crange<int>(), [](auto s) { return s == 0; }))
				continue;

			// Mesh faces must have the "Region" property.
			const PropertyObject* regionProperty = existingSurface->faces()->getProperty(SurfaceMeshFaces::RegionProperty);
			if(!regionProperty) continue; // Nothing to do if there is no face region information.

			// Create a work data structure for modifying the mesh.
			SurfaceMeshData mesh(existingSurface);
			OVITO_ASSERT(mesh.hasFaceRegions());
			numRegions += mesh.regionCount();

			// Make the topology and property arrays mutable.
			mesh.makeTopologyMutable();
			mesh.makeFacesMutable();
			mesh.makeRegionsMutable();

			// Delete all faces that belong to one of the selected mesh regions.
			for(SurfaceMeshData::face_index face = mesh.faceCount() - 1; face >= 0; face--) {
				SurfaceMeshData::region_index region = mesh.faceRegion(face);
				if(region >= 0 && region < mesh.regionCount() && selectionProperty->get<int>(region)) {
					if(mesh.hasOppositeFace(face))
						mesh.topology()->unlinkFromOppositeFace(face);
					mesh.deleteFace(face);
				}
			}

			// Delete the selected regions.
			for(SurfaceMeshData::region_index region = mesh.regionCount() - 1; region >= 0; region--) {
				if(selectionProperty->get<int>(region)) {
					mesh.deleteRegion(region);
					numSelected++;
				}
			}

			// Create a mutable copy of the SurfaceMesh.
			SurfaceMesh* newSurface = state.makeMutable(existingSurface);
			// Write the modified mesh back to the output object.
			mesh.transferTo(newSurface);

			// Remove selection property.
			newSurface->makeRegionsMutable()->removeProperty(newSurface->regions()->getProperty(SurfaceMeshRegions::SelectionProperty));
		}
	}

	// Report some statistics:
	QString statusMessage = tr("%n input regions", 0, numRegions);
	statusMessage += tr("\n%n regions deleted (%1%)", 0, numSelected).arg(numSelected * 100 / std::max(numRegions, (size_t)1));

	return PipelineStatus(PipelineStatus::Success, std::move(statusMessage));
}

}	// End of namespace
}	// End of namespace
