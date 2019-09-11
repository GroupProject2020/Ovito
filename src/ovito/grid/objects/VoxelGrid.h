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


#include <ovito/grid/Grid.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/stdobj/properties/PropertyReference.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>

namespace Ovito { namespace Grid {

/**
 * \brief This object stores a data grid made of voxels.
 */
class OVITO_GRID_EXPORT VoxelGrid : public PropertyContainer
{
	/// Define a new property metaclass for voxel property containers.
	class VoxelGridClass : public PropertyContainerClass
	{
	public:

		/// Inherit constructor from base class.
		using PropertyContainerClass::PropertyContainerClass;

	protected:

		/// Is called by the system after construction of the meta-class instance.
		virtual void initialize() override;
	};

	Q_OBJECT
	OVITO_CLASS_META(VoxelGrid, VoxelGridClass);

public:

	/// Data type used to store the number of cells of the voxel grid in each dimension.
	using GridDimensions = std::array<size_t,3>;

	/// \brief Constructor.
	Q_INVOKABLE VoxelGrid(DataSet* dataset, const QString& title = QString());

	/// Returns the title of this object.
	virtual QString objectTitle() const override {
		if(!title().isEmpty()) return title();
		else if(!identifier().isEmpty()) return identifier();
		else return PropertyContainer::objectTitle();
	}

	/// Returns the spatial domain this voxel grid is embedded in after making sure it
	/// can safely be modified.
	SimulationCellObject* mutableDomain() {
		return makeMutable(domain());
	}

	/// Makes sure that all property arrays in this container have a consistent length.
	/// If this is not the case, the method throws an exception.
	void verifyIntegrity() const;

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

private:

	/// The title of the grid, which is shown in the user interface.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, title, setTitle);

	/// The shape of the grid (i.e. number of voxels in each dimension).
	DECLARE_RUNTIME_PROPERTY_FIELD(GridDimensions, shape, setShape);

	/// The domain the object is embedded in.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(SimulationCellObject, domain, setDomain, PROPERTY_FIELD_ALWAYS_DEEP_COPY | PROPERTY_FIELD_NO_SUB_ANIM);
};

/**
 * Encapsulates a reference to a voxel grid property.
 */
using VoxelPropertyReference = TypedPropertyReference<VoxelGrid>;


}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Grid::VoxelPropertyReference);