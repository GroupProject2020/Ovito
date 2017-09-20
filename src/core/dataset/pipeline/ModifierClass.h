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


#include <core/Core.h>
#include <core/oo/RefTarget.h>

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
	/// \param input The pipeline state at the point of the pipeline where the modifier is going to be inserted.
	/// \return true if the modifier can operate on the provided input data; false otherwise.
	///
	/// This method is used to filter the list of available modifiers. The default implementation returns true.
	virtual bool isApplicableTo(const PipelineFlowState& input) const { return true; }

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
