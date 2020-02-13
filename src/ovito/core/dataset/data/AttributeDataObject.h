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


#include <ovito/core/Core.h>
#include <ovito/core/dataset/data/DataObject.h>

namespace Ovito {

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

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

private:

	/// The stored attribute value.
	DECLARE_RUNTIME_PROPERTY_FIELD(QVariant, value, setValue);
};

}	// End of namespace
