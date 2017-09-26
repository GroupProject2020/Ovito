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


#include <plugins/stdmod/StdMod.h>
#include <core/dataset/pipeline/DelegatingModifier.h>

namespace Ovito { namespace StdMod {

/**
 * \brief Base class for DeleteSelectedModifier delegates that operate on different kinds of data.
 */
class OVITO_STDMOD_EXPORT DeleteSelectedModifierDelegate : public ModifierDelegate
{
	OVITO_CLASS(DeleteSelectedModifierDelegate)
	Q_OBJECT
	
protected:

	/// Abstract class constructor.
	using ModifierDelegate::ModifierDelegate;
};

/**
 * \brief This modifier deletes the currently selected elements.
 */
class OVITO_STDMOD_EXPORT DeleteSelectedModifier : public MultiDelegatingModifier
{
	/// Give this modifier class its own metaclass.
	class DeleteSelectedModifierClass : public MultiDelegatingModifier::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using MultiDelegatingModifier::OOMetaClass::OOMetaClass;

		/// Return the metaclass of delegates for this modifier type. 
		virtual const ModifierDelegate::OOMetaClass& delegateMetaclass() const override { return DeleteSelectedModifierDelegate::OOClass(); }
	};
	
	Q_OBJECT
	OVITO_CLASS_META(DeleteSelectedModifier, DeleteSelectedModifierClass)
	Q_CLASSINFO("DisplayName", "Delete selected");
	Q_CLASSINFO("ModifierCategory", "Modification");
	
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE DeleteSelectedModifier(DataSet* dataset) : MultiDelegatingModifier(dataset) {
		// Generate the list of delegate objects.
		createModifierDelegates(DeleteSelectedModifierDelegate::OOClass());		
	}
};

}	// End of namespace
}	// End of namespace
