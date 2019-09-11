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

#include <ovito/core/Core.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/dataset/DataSet.h>
#include "AsynchronousDelegatingModifier.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(AsynchronousModifierDelegate);

IMPLEMENT_OVITO_CLASS(AsynchronousDelegatingModifier);
DEFINE_REFERENCE_FIELD(AsynchronousDelegatingModifier, delegate);

/******************************************************************************
* Returns the modifier to which this delegate belongs.
******************************************************************************/
AsynchronousDelegatingModifier* AsynchronousModifierDelegate::modifier() const
{
	for(RefMaker* dependent : this->dependents()) {
		if(AsynchronousDelegatingModifier* modifier = dynamic_object_cast<AsynchronousDelegatingModifier>(dependent)) {
			if(modifier->delegate() == this) return modifier;
		}
	}

	return nullptr;
}

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AsynchronousDelegatingModifier::AsynchronousDelegatingModifier(DataSet* dataset) : AsynchronousModifier(dataset)
{
}

/******************************************************************************
* Creates a default delegate for this modifier.
******************************************************************************/
void AsynchronousDelegatingModifier::createDefaultModifierDelegate(const OvitoClass& delegateType, const QString& defaultDelegateTypeName)
{
	OVITO_ASSERT(delegateType.isDerivedFrom(AsynchronousModifierDelegate::OOClass()));

	// Find the delegate type that corresponds to the given name string.
	for(OvitoClassPtr clazz : PluginManager::instance().listClasses(delegateType)) {
		if(clazz->name() == defaultDelegateTypeName) {
			OORef<AsynchronousModifierDelegate> delegate = static_object_cast<AsynchronousModifierDelegate>(clazz->createInstance(dataset()));
			setDelegate(delegate);
			break;
		}
	}
	OVITO_ASSERT_MSG(delegate(), "AsynchronousDelegatingModifier::createDefaultModifierDelegate", qPrintable(QStringLiteral("There is no delegate class named '%1' inheriting from %2.").arg(defaultDelegateTypeName).arg(delegateType.name())));
}

/******************************************************************************
* Asks the metaclass whether the modifier can be applied to the given input data.
******************************************************************************/
bool AsynchronousDelegatingModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	if(!AsynchronousModifier::OOMetaClass::isApplicableTo(input)) return false;

	// Check if there is any modifier delegate that could handle the input data.
	for(const AsynchronousModifierDelegate::OOMetaClass* clazz : PluginManager::instance().metaclassMembers<AsynchronousModifierDelegate>(delegateMetaclass())) {
		if(clazz->isApplicableTo(input))
			return true;
	}

	return false;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
