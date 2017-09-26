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
#include <core/dataset/pipeline/PipelineFlowState.h>
#include "PropertyObject.h"
#include "PropertyReference.h"

namespace Ovito { namespace StdObj {

/******************************************************************************
* Constructs a reference to a standard property.
******************************************************************************/
PropertyReference::PropertyReference(PropertyClassPtr pclass, int typeId, int vectorComponent) : 
	_propertyClass(pclass), _type(typeId), 
	_name(pclass->standardPropertyName(typeId)), _vectorComponent(vectorComponent) 
{
}

/******************************************************************************
* Constructs a reference based on an existing PropertyObject.
******************************************************************************/
PropertyReference::PropertyReference(PropertyObject* property, int vectorComponent) : 
	_propertyClass(&property->getOOMetaClass()), _type(property->type()), _name(property->name()), _vectorComponent(vectorComponent) 
{ 
}

/******************************************************************************
* Returns the display name of the referenced property including the 
* optional vector component.
******************************************************************************/
QString PropertyReference::nameWithComponent() const 
{
	if(type() != 0) {
		if(vectorComponent() < 0 || propertyClass()->standardPropertyComponentCount(type()) <= 1) {
			return name();
		}
		else {
			const QStringList& names = propertyClass()->standardPropertyComponentNames(type());
			if(vectorComponent() < names.size())
				return QString("%1.%2").arg(name()).arg(names[vectorComponent()]);
		}
	}
	if(vectorComponent() < 0)
		return name();
	else
		return QString("%1.%2").arg(name()).arg(vectorComponent() + 1);
}

/// Writes a PropertyReference to an output stream.
/// \relates PropertyReference
SaveStream& operator<<(SaveStream& stream, const PropertyReference& r)
{
	stream.beginChunk(0x01);	
	OvitoClass::serializeRTTI(stream, r.propertyClass());
	stream << r.type();
	stream << r.name();
	stream << r.vectorComponent();
	stream.endChunk();
	return stream;
}

/// Reads a PropertyReference from an input stream.
/// \relates PropertyReference
LoadStream& operator>>(LoadStream& stream, PropertyReference& r)
{
	stream.expectChunk(0x01);
	r._propertyClass = static_cast<PropertyClassPtr>(OvitoClass::deserializeRTTI(stream));
	stream >> r._type;
	stream >> r._name;
	stream >> r._vectorComponent;
	if(!r._propertyClass)
		r = PropertyReference();
	stream.closeChunk();
	return stream;
}
	
/******************************************************************************
* Finds the referenced property in the given pipeline state.
******************************************************************************/
PropertyObject* PropertyReference::findInState(const PipelineFlowState& state) const
{
	if(isNull())
		return nullptr;

	for(DataObject* o : state.objects()) {
		PropertyObject* prop = dynamic_object_cast<PropertyObject>(o);
		if(prop && propertyClass()->isMember(prop)) {
			if((this->type() == 0 && prop->name() == this->name()) ||
					(this->type() != 0 && prop->type() == this->type())) {
				return prop;
			}
		}
	}
	
	return nullptr;
}

}	// End of namespace
}	// End of namespace
