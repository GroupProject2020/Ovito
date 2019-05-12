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

#pragma once


#include <plugins/mesh/Mesh.h>
#include <plugins/stdobj/properties/PropertyContainer.h>

namespace Ovito { namespace Mesh {

/**
 * \brief Stores all voluemtric region-related properties of a SurfaceMesh.
 */
class OVITO_MESH_EXPORT SurfaceMeshRegions : public PropertyContainer
{
	/// Define a new property metaclass for this container type.
	class SurfaceMeshRegionsClass : public PropertyContainerClass
	{
	public:

		/// Inherit constructor from base class.
		using PropertyContainerClass::PropertyContainerClass;

		/// \brief Create a storage object for standard region properties.
		virtual PropertyPtr createStandardStorage(size_t regionCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath = {}) const override;

	protected:

		/// Is called by the system after construction of the meta-class instance.
		virtual void initialize() override;
	};

	Q_OBJECT
	OVITO_CLASS_META(SurfaceMeshRegions, SurfaceMeshRegionsClass);
	Q_CLASSINFO("DisplayName", "Mesh Regions");

public:

	/// \brief The list of standard region properties.
	enum Type {
		UserProperty = PropertyStorage::GenericUserProperty,	//< This is reserved for user-defined properties.
		ColorProperty = PropertyStorage::GenericColorProperty,
		PhaseProperty = PropertyStorage::FirstSpecificProperty,
		VolumeProperty,
		SurfaceAreaProperty,
		LatticeCorrespondenceProperty
	};

	/// \brief Constructor.
	Q_INVOKABLE SurfaceMeshRegions(DataSet* dataset) : PropertyContainer(dataset) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("Mesh Regions"); }
};

}	// End of namespace
}	// End of namespace
