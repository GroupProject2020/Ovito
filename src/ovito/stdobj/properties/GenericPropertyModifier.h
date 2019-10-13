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


#include <ovito/stdobj/StdObj.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/stdobj/properties/PropertyContainer.h>

namespace Ovito { namespace StdObj {

/**
 * \brief Base class for modifiers that operate on properties and which have no
 *        specific behavior that depends on the type of property it is (e.g. particle property, bond property, etc).
 */
class OVITO_STDOBJ_EXPORT GenericPropertyModifier : public Modifier
{
	/// Give this modifier class its own metaclass.
	class OVITO_STDOBJ_EXPORT GenericPropertyModifierClass : public ModifierClass
	{
	public:

		/// Inherit constructor from base class.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(GenericPropertyModifier, GenericPropertyModifierClass)

protected:

	/// Constructor.
	GenericPropertyModifier(DataSet* dataset);

	/// Sets the subject property container.
	void setDefaultSubject(const QString& pluginId, const QString& containerClassName);

private:

	/// The property container the modifier will operate on.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(PropertyContainerReference, subject, setSubject);
};

}	// End of namespace
}	// End of namespace
