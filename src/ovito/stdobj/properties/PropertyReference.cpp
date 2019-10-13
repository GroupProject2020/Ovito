////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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

#include <ovito/stdobj/StdObj.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include "PropertyObject.h"
#include "PropertyReference.h"
#include "PropertyContainer.h"

namespace Ovito { namespace StdObj {

/******************************************************************************
* Constructs a reference to a standard property.
******************************************************************************/
PropertyReference::PropertyReference(PropertyContainerClassPtr pclass, int typeId, int vectorComponent) :
	_containerClass(pclass), _type(typeId),
	_name(pclass->standardPropertyName(typeId)), _vectorComponent(vectorComponent)
{
}

/******************************************************************************
* Constructs a reference based on an existing PropertyObject.
******************************************************************************/
PropertyReference::PropertyReference(PropertyContainerClassPtr pclass, const PropertyObject* property, int vectorComponent) :
	_containerClass(pclass),
	_type(property->type()), _name(property->name()), _vectorComponent(vectorComponent)
{
}

/******************************************************************************
* Returns the display name of the referenced property including the
* optional vector component.
******************************************************************************/
QString PropertyReference::nameWithComponent() const
{
	if(type() != 0) {
		if(vectorComponent() < 0 || containerClass()->standardPropertyComponentCount(type()) <= 1) {
			return name();
		}
		else {
			const QStringList& names = containerClass()->standardPropertyComponentNames(type());
			if(vectorComponent() < names.size())
				return QString("%1.%2").arg(name()).arg(names[vectorComponent()]);
		}
	}
	if(vectorComponent() < 0)
		return name();
	else
		return QString("%1.%2").arg(name()).arg(vectorComponent() + 1);
}

/******************************************************************************
* Returns a new property reference that uses the same name as the current one,
* but with a different property container class.
******************************************************************************/
PropertyReference PropertyReference::convertToContainerClass(PropertyContainerClassPtr containerClass) const
{
	if(containerClass) {
		PropertyReference newref = *this;
		if(containerClass != this->containerClass()) {
			newref._containerClass = containerClass;
			newref._type = containerClass->standardPropertyTypeId(name());
		}
		return newref;
	}
	else {
		return {};
	}
}

/// Writes a PropertyReference to an output stream.
/// \relates PropertyReference
SaveStream& operator<<(SaveStream& stream, const PropertyReference& r)
{
	stream.beginChunk(0x02);
	stream << r.containerClass();
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
	stream.expectChunk(0x02);
	stream >> r._containerClass;
	stream >> r._type;
	stream >> r._name;
	stream >> r._vectorComponent;
	if(!r._containerClass)
		r = PropertyReference();
	stream.closeChunk();
	return stream;
}

/******************************************************************************
* Finds the referenced property in the given property container object.
******************************************************************************/
const PropertyObject* PropertyReference::findInContainer(const PropertyContainer* container) const
{
	if(isNull())
		return nullptr;

	OVITO_ASSERT(container != nullptr);
	OVITO_ASSERT(containerClass()->isMember(container));

	if(type() != 0)
		return container->getProperty(type());
	else
		return container->getProperty(name());
}

}	// End of namespace
}	// End of namespace
