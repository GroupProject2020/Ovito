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


#include <plugins/stdobj/StdObj.h>
#include <core/oo/RefTarget.h>

namespace Ovito { namespace StdObj {

/**
 * \brief Describes the basic properties (unique ID, name & color) of a "type" of elements stored in a PropertyObject.
 *        This serves as generic base class for particle types, bond types, structural types, etc.
 */
class OVITO_STDOBJ_EXPORT ElementType : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(ElementType)
	
public:

	/// \brief Constructs a new type.
	Q_INVOKABLE ElementType(DataSet* dataset);

	/// Returns the title of this object.
	virtual QString objectTitle() override { return name(); }

protected:

	/// Stores the unique identifier of the type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, id, setId);

	/// The human-readable name of this type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, name, setName);

	/// Stores the visualization color of the type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, color, setColor);

	/// Stores whether this type is "enabled" or "disabled".
	/// This makes only sense in some sorts of types. For example, structure identification modifiers
	/// use this field to determine which structural types they should look for.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, enabled, setEnabled);
};

}	// End of namespace
}	// End of namespace
