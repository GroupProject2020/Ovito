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

#include <core/Core.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/data/AttributeDataObject.h>
#include "PipelineOutputHelper.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/******************************************************************************
* Returns a new unique data object identifier that does not collide with the 
* identifiers of any existing data object of the given type in the same data 
* collection.
******************************************************************************/
QString PipelineOutputHelper::generateUniqueIdentifier(const QString& baseName, const OvitoClass& dataObjectClass) const
{
	// This helper function checks if the given id is already used by another data object of the
	// given type in the this data collection.
	auto doesExist = [this,&dataObjectClass](const QString& id) {
		for(DataObject* obj : output().objects()) {
			if(dataObjectClass.isMember(obj)) {
				if(obj->identifier() == id)
					return true;
			}
		}
		return false;
	};

	if(!doesExist(baseName)) {
		return baseName;
	}
	else {
		// Append consecutive indices to the base ID name.
		for(int i = 2; ; i++) {
			QString uniqueId = baseName + QChar('.') + QString::number(i);
			if(!doesExist(uniqueId)) {
				return uniqueId;
			}
		}
	}
	OVITO_ASSERT(false);
}

/******************************************************************************
* Emits a new global attribute to the pipeline.
******************************************************************************/
void PipelineOutputHelper::outputAttribute(const QString& key, QVariant value)
{
	// Undo recording should never be active during pipeline evaluation.
	OVITO_ASSERT(!dataset()->undoStack().isRecording());

	// Create the data object storing the attribute's value.
	AttributeDataObject* attrObj = new AttributeDataObject(dataset(), std::move(value));

	// Generate a unique attribute name.
	OVITO_ASSERT(!key.isEmpty());
	attrObj->setIdentifier(generateUniqueIdentifier<AttributeDataObject>(key)); 

	// Put the attribute object into the output data collection.
	outputObject(attrObj);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
