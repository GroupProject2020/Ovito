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

	/// \brief Returns whether this object, when returned as an editable sub-object by another object,
	///        should be displayed in the modification stack.
	///
	/// Return false because this object cannot be edited.
	virtual bool isSubObjectEditable() const override { return false; }


protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;
		
private:

	/// The shape of the grid (i.e. number of voxels in each dimension).
	DECLARE_RUNTIME_PROPERTY_FIELD(std::vector<size_t>, shape, setShape);
};

}	// End of namespace
}	// End of namespace
