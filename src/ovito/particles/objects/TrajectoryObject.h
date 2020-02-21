////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
	class OOMetaClass : public PropertyContainerClass
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
	OVITO_CLASS_META(TrajectoryObject, OOMetaClass);
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
