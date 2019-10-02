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

#include <ovito/grid/Grid.h>
#include <ovito/grid/objects/VoxelGrid.h>
#include "VoxelGridColorCodingModifierDelegate.h"

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(VoxelGridColorCodingModifierDelegate);

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier 
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> VoxelGridColorCodingModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const 
{
	// Gather list of all VoxelGrid objects in the input data collection.
	QVector<DataObjectReference> objects;
	for(const ConstDataObjectPath& path : input.getObjectsRecursive(VoxelGrid::OOClass())) {
		objects.push_back(path);
	}
	return objects;
}

}	// End of namespace
}	// End of namespace
