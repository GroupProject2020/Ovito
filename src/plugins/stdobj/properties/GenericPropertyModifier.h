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


#include <plugins/stdobj/StdObj.h>
#include <core/dataset/pipeline/Modifier.h>
#include <plugins/stdobj/properties/PropertyContainer.h>

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
