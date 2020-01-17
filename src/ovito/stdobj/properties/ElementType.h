////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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


#include <ovito/stdobj/StdObj.h>
#include <ovito/core/dataset/data/DataObject.h>

namespace Ovito { namespace StdObj {

/**
 * \brief Describes the basic properties (unique ID, name & color) of a "type" of elements stored in a PropertyObject.
 *        This serves as generic base class for particle types, bond types, structural types, etc.
 */
class OVITO_STDOBJ_EXPORT ElementType : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(ElementType)

public:

	/// \brief Constructs a new type.
	Q_INVOKABLE ElementType(DataSet* dataset);

	/// \brief Returns the name of this type, or a dynamically generated string representing the
	///        numeric ID if the type has no assigned name.
	QString nameOrNumericId() const {
		if(!name().isEmpty())
			return name();
		else
			return generateDefaultTypeName(numericId());
	}

	/// \brief Returns an automatically generated name for a type based on its numeric ID.
	static QString generateDefaultTypeName(int id) {
		return tr("Type %1").arg(id);
	}

	/// \brief Returns the title of this object. Same as nameOrNumericId().
	virtual QString objectTitle() const override { return nameOrNumericId(); }

	/// Returns the default color for the element type with the given ID.
	static const Color& getDefaultColorForId(int typeClass, int typeId);

	/// Returns the default color for a named element type.
	static Color getDefaultColor(int typeClass, const QString& typeName, int typeId, bool useUserDefaults = true);

	/// Changes the default color for a named element type.
	static void setDefaultColor(int typeClass, const QString& typeName, const Color& color);

protected:

	/// Stores the unique numeric identifier of the type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, numericId, setNumericId);

	/// The human-readable name assigned to this type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, name, setName);

	/// Stores the visualization color of the type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Color, color, setColor, PROPERTY_FIELD_MEMORIZE);

	/// Stores whether this type is "enabled" or "disabled".
	/// This makes only sense in some sorts of types. For example, structure identification modifiers
	/// use this field to determine which structural types they should look for.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, enabled, setEnabled);
};

}	// End of namespace
}	// End of namespace
