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

#include <plugins/grid/Grid.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include "VoxelProperty.h"

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(VoxelProperty);

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void VoxelProperty::OOMetaClass::initialize()
{
	PropertyClass::initialize();

	// Enable automatic conversion of a VoxelPropertyReference to a generic PropertyReference and vice versa.
	QMetaType::registerConverter<VoxelPropertyReference, PropertyReference>();
	QMetaType::registerConverter<PropertyReference, VoxelPropertyReference>();		

	setPropertyClassDisplayName(tr("Voxel data"));
	setElementDescriptionName(QStringLiteral("voxels"));
	setPythonName(QStringLiteral("voxels"));
}

/******************************************************************************
* Returns the data grid size in the given data state.
******************************************************************************/
size_t VoxelProperty::OOMetaClass::elementCount(const PipelineFlowState& state) const
{
	for(DataObject* obj : state.objects()) {
		if(VoxelProperty* property = dynamic_object_cast<VoxelProperty>(obj)) {
			return property->size();
		}
	}
	return 0;
}

/******************************************************************************
* Determines if the data elements which this property class applies to are 
* present for the given data state.
******************************************************************************/
bool VoxelProperty::OOMetaClass::isDataPresent(const PipelineFlowState& state) const
{
	return state.findObject<VoxelProperty>() != nullptr;
}

}	// End of namespace
}	// End of namespace
