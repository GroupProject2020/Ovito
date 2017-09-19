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

#pragma once


#include <plugins/grid/Grid.h>
#include <core/dataset/data/properties/PropertyObject.h>
#include <core/dataset/data/properties/PropertyClass.h>
#include <core/dataset/data/properties/PropertyReference.h>

namespace Ovito { namespace Grid {

/**
 * \brief Property type for voxel grids.
 */
class OVITO_GRID_EXPORT VoxelProperty : public PropertyObject
{
public:

	/// Define a new property metaclass for voxel properties.
	class OVITO_GRID_EXPORT VoxelPropertyClass : public PropertyClass 
	{
	public:

		/// Inherit constructor from base class.
		using PropertyClass::PropertyClass;

		/// Returns the number of elements in a property for the given data state.
		virtual size_t elementCount(const PipelineFlowState& state) const override;

		/// Determines if the data elements which this property class applies to are present for the given data state.
		virtual bool isDataPresent(const PipelineFlowState& state) const override;
	
	protected:

		/// Is called by the system after construction of the meta-class instance.
		virtual void initialize() override {
			PropertyClass::initialize();
			setPropertyClassDisplayName(tr("Voxel data"));
			setElementDescriptionName(QStringLiteral("voxels"));
			setPythonName(QStringLiteral("voxels"));
		}
	};

	Q_OBJECT
	OVITO_CLASS_META(VoxelProperty, VoxelPropertyClass)
	
public:

	/// \brief Creates a voxel property object.
	Q_INVOKABLE VoxelProperty(DataSet* dataset) : PropertyObject(dataset) {}	
};

/**
 * Encapsulates a reference to a voxel property. 
 */
using VoxelPropertyReference = TypedPropertyReference<VoxelProperty>;

}	// End of namespace
}	// End of namespace
