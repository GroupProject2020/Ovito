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


#include <ovito/stdmod/StdMod.h>
#include <ovito/core/dataset/pipeline/DelegatingModifier.h>

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
