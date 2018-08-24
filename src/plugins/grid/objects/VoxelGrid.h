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
#include <plugins/stdobj/simcell/PeriodicDomainDataObject.h>
#include <plugins/stdobj/properties/PropertyObject.h>

namespace Ovito { namespace Grid {

/**
 * \brief This object stores the spatial dimensions of a data grid made of voxels.
 */
class OVITO_MESH_EXPORT VoxelGrid : public PeriodicDomainDataObject
{
	Q_OBJECT
	OVITO_CLASS(VoxelGrid)
	
public:

	/// \brief Constructor.
	Q_INVOKABLE VoxelGrid(DataSet* dataset);

	/// Returns the title of this object.
	virtual QString objectTitle() override { return tr("Voxel grid"); }

	/// Appends a property to the list of properties.
	void addProperty(PropertyObject* property) {
		OVITO_ASSERT(properties().contains(property) == false);
		_properties.push_back(this, PROPERTY_FIELD(properties), property);
	}

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;
		
private:

	/// The shape of the grid (i.e. number of voxels in each dimension).
	DECLARE_RUNTIME_PROPERTY_FIELD(std::vector<size_t>, shape, setShape);

	/// Holds the list of voxel properties.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(PropertyObject, properties, setProperties);
};

}	// End of namespace
}	// End of namespace
