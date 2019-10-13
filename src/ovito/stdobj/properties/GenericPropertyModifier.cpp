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

#include <ovito/stdobj/StdObj.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/core/app/PluginManager.h>
#include "GenericPropertyModifier.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(GenericPropertyModifier);
DEFINE_PROPERTY_FIELD(GenericPropertyModifier, subject);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
GenericPropertyModifier::GenericPropertyModifier(DataSet* dataset) : Modifier(dataset)
{
}

/******************************************************************************
* Sets the subject property container.
******************************************************************************/
void GenericPropertyModifier::setDefaultSubject(const QString& pluginId, const QString& containerClassName)
{
	if(OvitoClassPtr containerClass = PluginManager::instance().findClass(pluginId, containerClassName)) {
		OVITO_ASSERT(containerClass->isDerivedFrom(PropertyContainer::OOClass()));
		setSubject(static_cast<PropertyContainerClassPtr>(containerClass));
	}
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool GenericPropertyModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	if(!ModifierClass::isApplicableTo(input)) return false;

	// Modifier is applicable if there is at least one property container in the input data.
	// Subclasses can override this behavior.
	return input.containsObjectRecursive(PropertyContainer::OOClass());
}

}	// End of namespace
}	// End of namespace
