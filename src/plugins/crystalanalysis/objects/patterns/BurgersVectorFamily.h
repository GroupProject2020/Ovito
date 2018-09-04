///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/stdobj/properties/ElementType.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief represents a dislocation type.
 */
class OVITO_CRYSTALANALYSIS_EXPORT BurgersVectorFamily : public ElementType
{
	Q_OBJECT
	OVITO_CLASS(BurgersVectorFamily)

public:

	/// \brief Constructs a new BurgersVectorFamily.
	Q_INVOKABLE BurgersVectorFamily(DataSet* dataset, int id = 0, const QString& name = QString(), const Vector3& burgersVector = Vector3::Zero(), const Color& color = Color(0,0,0));

	/// Checks if the given Burgers vector is a member of this family.
	bool isMember(const Vector3& v, const StructurePattern* latticeStructure) const;

private:

	/// This prototype Burgers vector of this family.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Vector3, burgersVector, setBurgersVector);
};

}	// End of namespace
}	// End of namespace
}	// End of namespace
