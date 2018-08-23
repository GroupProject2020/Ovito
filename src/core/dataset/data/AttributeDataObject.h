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


#include <core/Core.h>
#include <core/dataset/data/DataObject.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief A data object holding a primitive value (e.g. a number or a string).
 */
class OVITO_CORE_EXPORT AttributeDataObject : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(AttributeDataObject)

public:

	/// Constructor.
	Q_INVOKABLE AttributeDataObject(DataSet* dataset, QVariant value = {}) 
		: DataObject(dataset), _value(std::move(value)) {}

	/// Returns whether this object, when returned as an editable sub-object by another object,
	/// should be displayed in the pipeline editor.
	virtual bool isSubObjectEditable() const override { return false; }

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

private:

	/// The stored attribute value.
	DECLARE_RUNTIME_PROPERTY_FIELD(QVariant, value, setValue);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
