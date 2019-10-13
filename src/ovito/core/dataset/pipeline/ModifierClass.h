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


#include <ovito/core/Core.h>
#include <ovito/core/oo/RefTarget.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene) OVITO_BEGIN_INLINE_NAMESPACE(StdObj)

/**
 * \brief A meta-class for modifiers (i.e. classes derived from Modifier).
 */
class OVITO_CORE_EXPORT ModifierClass : public RefTarget::OOMetaClass
{
public:

	/// Inherit standard constructor from base meta class.
	using RefTarget::OOMetaClass::OOMetaClass;

	/// \brief Asks the modifier metaclass whether the modifier class can be applied to the given input data.
	/// \param input The data collection to operate on.
	/// \return true if the modifier can operate on the provided input data; false otherwise.
	///
	/// This method is used to filter the list of available modifiers. The default implementation returns true.
	virtual bool isApplicableTo(const DataCollection& input) const { return true; }

	/// \brief Returns the category under which the modifier will be displayed in the modifier list box.
	virtual QString modifierCategory() const {
		if(qtMetaObject()) {
			int infoIndex = qtMetaObject()->indexOfClassInfo("ModifierCategory");
			if(infoIndex != -1) {
				return QString::fromLocal8Bit(qtMetaObject()->classInfo(infoIndex).value());
			}
		}
		return {};
	}
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::ModifierClassPtr);
Q_DECLARE_TYPEINFO(Ovito::ModifierClassPtr, Q_PRIMITIVE_TYPE);
