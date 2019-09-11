///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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


#include <ovito/particles/Particles.h>
#include <ovito/stdobj/properties/PropertyContainer.h>

namespace Ovito { namespace Particles {

/**
 * \brief Stores trajectory lines of a particles dataset.
 */
class OVITO_PARTICLES_EXPORT TrajectoryObject : public PropertyContainer
{
	/// Define a new property metaclass for this property container type.
	class TrajectoryObjectClass : public PropertyContainerClass
	{
	public:

		/// Inherit constructor from base class.
		using PropertyContainerClass::PropertyContainerClass;

		/// Creates a storage object for standard properties.
		virtual PropertyPtr createStandardStorage(size_t elementCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath = {}) const override;

	protected:

		/// Is called by the system after construction of the meta-class instance.
		virtual void initialize() override;
	};

	Q_OBJECT
	OVITO_CLASS_META(TrajectoryObject, TrajectoryObjectClass);
	Q_CLASSINFO("DisplayName", "Particle trajectories");

public:

	/// \brief The list of standard properties.
	enum Type {
		PositionProperty = PropertyStorage::FirstSpecificProperty,
		SampleTimeProperty,
		ParticleIdentifierProperty
	};

	/// \brief Constructor.
	Q_INVOKABLE TrajectoryObject(DataSet* dataset);
};

}	// End of namespace
}	// End of namespace
