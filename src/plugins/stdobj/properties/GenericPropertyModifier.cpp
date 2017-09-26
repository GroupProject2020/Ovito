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

#include <plugins/stdobj/StdObj.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include "GenericPropertyModifier.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(GenericPropertyModifier);
DEFINE_PROPERTY_FIELD(GenericPropertyModifier, propertyClass);		

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
GenericPropertyModifier::GenericPropertyModifier(DataSet* dataset) : Modifier(dataset),
	_propertyClass(nullptr)
{
}

/******************************************************************************
* Saves the class' contents to an output stream.
******************************************************************************/
void GenericPropertyModifier::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	Modifier::saveToStream(stream, excludeRecomputableData);
	stream.beginChunk(0x01);
	OvitoClass::serializeRTTI(stream, propertyClass());
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from an input stream.
******************************************************************************/
void GenericPropertyModifier::loadFromStream(ObjectLoadStream& stream)
{
	Modifier::loadFromStream(stream);
	stream.expectChunk(0x01);
	setPropertyClass(static_cast<PropertyClassPtr>(OvitoClass::deserializeRTTI(stream)));
	stream.closeChunk();
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool GenericPropertyModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	if(!ModifierClass::isApplicableTo(input)) return false;

	// Modifier is applicable if there is at least one property in the input data.
	// Subclasses can override this behavior.
	return input.findObject<PropertyObject>() != nullptr;
}

}	// End of namespace
}	// End of namespace
